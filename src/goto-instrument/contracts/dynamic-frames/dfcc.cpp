/*******************************************************************\

Module: Dynamic frame condition checking

Author: Remi Delmas, delmarsd@amazon.com

\*******************************************************************/

// TODO havoc global statics
// TODO move disable assigns clause pragma into utils
// TODO prune includes
#include "dfcc.h"

#include <util/config.h>
#include <util/expr_util.h>
#include <util/format_expr.h>
#include <util/format_type.h>
#include <util/fresh_symbol.h>
#include <util/mathematical_expr.h>
#include <util/mathematical_types.h>
#include <util/namespace.h>
#include <util/optional.h>
#include <util/pointer_expr.h>
#include <util/pointer_offset_size.h>
#include <util/pointer_predicates.h>
#include <util/std_expr.h>

#include <goto-programs/goto_functions.h>
#include <goto-programs/goto_inline.h>
#include <goto-programs/goto_model.h>
#include <goto-programs/link_to_library.h>
#include <goto-programs/remove_skip.h>
#include <goto-programs/remove_unused_functions.h>

#include <ansi-c/c_expr.h>
#include <ansi-c/cprover_library.h>
#include <goto-instrument/contracts/cfg_info.h>
#include <goto-instrument/contracts/instrument_spec_assigns.h>
#include <goto-instrument/contracts/utils.h>
#include <goto-instrument/nondet_static.h>
#include <linking/static_lifetime_init.h>

void dfcc(
  goto_modelt &goto_model,
  const std::string &harness_id,
  const std::set<std::string> &to_check,
  const bool allow_recursive_calls,
  const std::set<std::string> &to_replace,
  const bool apply_loop_contracts,
  const std::set<std::string> &to_exclude_from_nondet_static,
  messaget &log)
{
  std::map<std::string, std::string> to_check_map;
  for(const auto &function_id : to_check)
    to_check_map.insert({function_id, function_id});

  std::map<std::string, std::string> to_replace_map;
  for(const auto &function_id : to_replace)
    to_replace_map.insert({function_id, function_id});

  dfcct{goto_model, log}.transform_goto_model(
    harness_id,
    to_check_map,
    allow_recursive_calls,
    to_replace_map,
    apply_loop_contracts,
    to_exclude_from_nondet_static);
}

void dfcc(
  goto_modelt &goto_model,
  const std::string &harness_id,
  const std::map<std::string, std::string> &to_check,
  const bool allow_recursive_calls,
  const std::map<std::string, std::string> &to_replace,
  const bool apply_loop_contracts,
  const std::set<std::string> &to_exclude_from_nondet_static,
  messaget &log)
{
  dfcct{goto_model, log}.transform_goto_model(
    harness_id,
    to_check,
    allow_recursive_calls,
    to_replace,
    apply_loop_contracts,
    to_exclude_from_nondet_static);
}

std::map<irep_idt, irep_idt> dfcct::wrapped_functions_cache;

dfcct::dfcct(goto_modelt &goto_model, messaget &log)
  : goto_model(goto_model),
    log(log),
    message_handler(log.get_message_handler()),
    utils(goto_model, log),
    library(goto_model, utils, log),
    ns(goto_model.symbol_table),
    instrument(goto_model, log, utils, library),
    spec_functions(goto_model, log, utils, library, instrument),
    dsl_contract_handler(
      goto_model,
      log,
      utils,
      library,
      instrument,
      spec_functions),
    swap_and_wrap(
      goto_model,
      log,
      utils,
      library,
      instrument,
      spec_functions,
      dsl_contract_handler),
    max_assigns_clause_size(-1)
{
}

void dfcct::check_transform_goto_model_preconditions(
  const std::string &harness_id,
  const std::map<std::string, std::string> &to_check,
  const bool allow_recursive_calls,
  const std::map<std::string, std::string> &to_replace,
  const bool apply_loop_contracts,
  const std::set<std::string> &to_exclude_from_nondet_static)
{
  log.debug() << "harness_id " << harness_id << messaget::eom;

  for(const auto &pair : to_check)
  {
    log.debug() << "to check " << pair.first << "->" << pair.second
                << messaget::eom;
  }

  for(const auto &pair : to_replace)
  {
    log.debug() << "to replace " << pair.first << "->" << pair.second
                << messaget::eom;
  }

  // TODO reactivate this once I understand how entry points work
  // Check that the goto_model entry point matches the given harness_id
  // if(!config.main.has_value())
  // {
  //   log.error() << "dfcct::transform_goto_model: no entry point found in the"
  //                  "goto model, expected '"
  //               << harness_id << "', aborting contracts transformations."
  //               << messaget::eom;
  //   throw 0;
  // }

  // if(config.main.value() != harness_id)
  // {
  //   log.error() << "dfcct::transform_goto_model: entry point '"
  //               << config.main.value()
  //               << "' differs from given harness function '" << harness_id
  //               << "', aborting contracts transformations." << messaget::eom;
  //   throw 0;
  // }

  // check there is at most one function to check
  PRECONDITION_WITH_DIAGNOSTICS(
    to_check.size() <= 1,
    "Only a single (function, contract) pair can be checked at a time, "
    "aborting");

  // check that harness function exists
  PRECONDITION_WITH_DIAGNOSTICS(
    utils.function_symbol_with_body_exists(harness_id),
    "Harness function '" + harness_id + "' either not found or has no body");

  for(const auto &pair : to_check)
  {
    PRECONDITION_WITH_DIAGNOSTICS(
      utils.function_symbol_with_body_exists(pair.first),
      "Function to check '" + pair.first +
        "' either not found or has no body");

    PRECONDITION_WITH_DIAGNOSTICS(
      dsl_contract_handler.pure_contract_symbol_exists(pair.second),
      "Contract to check '" + pair.second + "' not found");

    PRECONDITION_WITH_DIAGNOSTICS(
      pair.first != harness_id,
      "Function '" + pair.first +
        "' cannot be both be checked against a contract and be the harness");

    PRECONDITION_WITH_DIAGNOSTICS(
      pair.second != harness_id,
      "Function '" + pair.first +
        "' cannot be both the contract to check and be the harness");

    PRECONDITION_WITH_DIAGNOSTICS(
      to_replace.find(pair.first) == to_replace.end(),
      "Function '" + pair.first +
        "' cannot be both checked against contract and replaced by a contract");

    PRECONDITION_WITH_DIAGNOSTICS(
      !instrument.do_not_instrument(pair.first),
      "CPROVER function or builtin '" + pair.first +
        "' cannot be checked against a contract");
  }

  for(const auto &pair : to_replace)
  {
    // for functions to replace with contracts we don't require the replaced
    // function to have a body because only the contract is actually used
    PRECONDITION_WITH_DIAGNOSTICS(
      utils.function_symbol_exists(pair.first),
      "Function to replace '" + pair.first + "' not found");

    PRECONDITION_WITH_DIAGNOSTICS(
      dsl_contract_handler.pure_contract_symbol_exists(pair.second),
      "Contract to replace '" + pair.second + "' not found");

    PRECONDITION_WITH_DIAGNOSTICS(
      pair.first != harness_id,
      "Function '" + pair.first +
        "' cannot both be replaced with a contract and be the harness");

    PRECONDITION_WITH_DIAGNOSTICS(
      pair.second != harness_id,
      "Function '" + pair.first +
        "' cannot both be the contract to use for replacement and be the "
        "harness");
  }
}

void dfcct::partition_function_symbols(
  const std::vector<irep_idt> &symbols,
  std::set<irep_idt> &contract_symbols,
  std::set<irep_idt> &assignable_t_symbols,
  std::set<irep_idt> &freeable_t_symbols,
  std::set<irep_idt> &other_symbols)
{
  auto &symbol_table = goto_model.symbol_table;

  for(auto &sym_name : symbols)
  {
    const auto &symbol = symbol_table.lookup_ref(sym_name);

    if(symbol.type.id() != ID_code)
      continue;

    // is it a pure contract ?
    if(symbol.is_property && (id2string(sym_name).rfind("contract::", 0) == 0))
    {
      contract_symbols.insert(sym_name);
    }
    else if(spec_functions.is_assignable_t_function(sym_name))
    {
      assignable_t_symbols.insert(sym_name);
    }
    else if(spec_functions.is_freeable_t_function(sym_name))
    {
      freeable_t_symbols.insert(sym_name);
    }
    else
    {
      other_symbols.insert(sym_name);
    }
  }
}

void dfcct::transform_goto_model(
  const std::string &harness_id,
  const std::map<std::string, std::string> &to_check,
  const bool allow_recursive_calls,
  const std::map<std::string, std::string> &to_replace,
  const bool apply_loop_contracts,
  const std::set<std::string> &to_exclude_from_nondet_static)
{
  check_transform_goto_model_preconditions(
    harness_id,
    to_check,
    allow_recursive_calls,
    to_replace,
    apply_loop_contracts,
    to_exclude_from_nondet_static);

  // load the cprover library to make sure the model is complete
  log.status() << "Loading CPROVER C library (" << config.ansi_c.arch << ")"
               << messaget::eom;
  link_to_library(
    goto_model, log.get_message_handler(), cprover_c_library_factory);

  // Make all statics nondet. This needs to be done before starting the program
  // transformation since the instrumentation adds static variables
  // which must keep their initial values to function as intended.
  log.status()
    << "Adding nondeterministic initialization of static/global variables."
    << messaget::eom;
  nondet_static(goto_model, to_exclude_from_nondet_static);

  // shortcuts
  symbol_tablet &symbol_table = goto_model.symbol_table;

  // partition the set of functions of the goto_model
  std::set<irep_idt> pure_contract_symbols;
  std::set<irep_idt> assignable_t_symbols;
  std::set<irep_idt> freeable_t_symbols;
  std::set<irep_idt> other_symbols;
  std::set<irep_idt> function_pointer_contracts;

  // this vector must be computed before loading the dfcc lib
  // so it does not contain dfcc library symbols
  const auto preexisting_symbols = symbol_table.sorted_symbol_names();

  partition_function_symbols(
    preexisting_symbols,
    pure_contract_symbols,
    assignable_t_symbols,
    freeable_t_symbols,
    other_symbols);

  // load the dfcc library before instrumentation starts
  library.load(other_symbols);

  // add C prover lib again to fetch any dependencies of the dfcc functions
  link_to_library(
    goto_model, log.get_message_handler(), cprover_c_library_factory);

  // instrument the harness function
  // load the cprover library to make sure the model is complete
  log.status() << "Instrumenting harness function '" << harness_id << "'"
               << messaget::eom;
  instrument.instrument_harness_function(harness_id);

  other_symbols.erase(harness_id);

  // swap-and-wrap checked functions with contracts
  for(const auto &pair : to_check)
  {
    const auto &wrapper_id = pair.first;
    const auto &contract_id = pair.second;
    log.status() << "Wrapping '" << wrapper_id << "' with contract '"
                 << contract_id << "' in CHECK mode" << messaget::eom;

    swap_and_wrap.swap_and_wrap_check(
      wrapper_id,
      contract_id,
      function_pointer_contracts,
      allow_recursive_calls);

    if(other_symbols.find(wrapper_id) != other_symbols.end())
      other_symbols.erase(wrapper_id);

    // upate max contract size
    const int assigns_clause_size =
      dsl_contract_handler.get_assigns_clause_size(contract_id);
    if(assigns_clause_size > max_assigns_clause_size)
      max_assigns_clause_size = assigns_clause_size;
  }

  // swap-and-wrap replaced functions with contracts
  for(const auto &pair : to_replace)
  {
    const auto &wrapper_id = pair.first;
    const auto &contract_id = pair.second;
    log.status() << "Wrapping '" << wrapper_id << "' with contract '"
                 << contract_id << "' in REPLACE mode" << messaget::eom;
    swap_and_wrap.swap_and_wrap_replace(
      wrapper_id,
      contract_id,
      function_pointer_contracts);
    if(other_symbols.find(wrapper_id) != other_symbols.end())
    {
      other_symbols.erase(wrapper_id);
    }
  }

  // swap-and-wrap function pointer contracts with themselves
  for(const auto &fp_contract : function_pointer_contracts)
  {
    log.status() << "Discovered function pointer contract '" << fp_contract
                 << "'" << messaget::eom;

    // contracts for function pointers must be replaced with themselves
    // so we need to check that:
    // - the symbol exists as a function symbol
    // - the symbol exists as a pure contract symbol
    // - the function symbol is not already swapped for contract checking
    // - the function symbol is not already swapped with another contract for
    // replacement

    const auto str = id2string(fp_contract);

    // Is it already swapped with another function for contract checking ?
    PRECONDITION_WITH_DIAGNOSTICS(
      to_check.find(str) == to_check.end(),
      "Function '" + str +
        "' used as contract for function pointer cannot be itself the object "
        "of a contract check.");


    // Is it already swapped with another function for contract checking ?
    auto found = to_replace.find(str);
    if(found != to_replace.end()) {
      PRECONDITION_WITH_DIAGNOSTICS(
        found->first == found->second,
        "Function '" + str +
          "' used as contract for function pointer already the object of a "
          "contract replacement with '" +
          found->second + "'");
      log.status() << "Function pointer contract '" << fp_contract
                   << "' already wrapped with itself in REPLACE mode"
                   << messaget::eom;
    }
    else
    {
      // we need to swap it with itself
      PRECONDITION_WITH_DIAGNOSTICS(
        utils.function_symbol_exists(str),
        "Function pointer contract '" + str + "' not found.");

      PRECONDITION_WITH_DIAGNOSTICS(
        dsl_contract_handler.pure_contract_symbol_exists(str),
        "Contract to replace '" + str + "' not found.");

      log.status() << "Wrapping function pointer contract '" << fp_contract
                   << "' with itself in REPLACE mode" << messaget::eom;

      swap_and_wrap.swap_and_wrap_replace(
        fp_contract, fp_contract, function_pointer_contracts);
      // remove it from the set of symbols to process
      if(other_symbols.find(fp_contract) != other_symbols.end())
        other_symbols.erase(fp_contract);
    }
  }

  // instrument all other remaining functions
  for(const auto &function_id : other_symbols)
  {
    // Don't instrument CPROVER and internal functions
    if(instrument.do_not_instrument(function_id))
    {
      continue;
    }

    log.status() << "Instrumenting '" << function_id << "'" << messaget::eom;

    instrument.instrument_function(function_id);
  }

  log.status() << "Updating goto functions" << messaget::eom;
  goto_model.goto_functions.update();

  if(!to_check.empty())
  {
    log.status() << "Specializing cprover_contracts functions for assigns "
                    "clauses of at most "
                 << max_assigns_clause_size << " targets" << messaget::eom;
    library.specialize(max_assigns_clause_size);
  }

  log.status() << "Inhibiting front-end functions" << messaget::eom;
  library.inhibit_front_end_builtins();
  // TODO utils.inhibit_unreachable_functions(harness);
  goto_model.goto_functions.update();

  log.status() << "Removing SKIP instructions" << messaget::eom;
  remove_skip(goto_model);
  goto_model.goto_functions.update();

  log.status() << "Removing unused functions" << messaget::eom;
  // This can prune too many functions if function pointers have not been
  // yet been removed or if the entry point is not defined.
  // Another solution would be to rewrite the bodies of functions that seem to
  // be unreachable into assert(false);assume(false)
  remove_unused_functions(goto_model, message_handler);
  goto_model.goto_functions.update();

  // collect set of functions which are now instrumented
  std::set<irep_idt> instrumented_functions;
  instrument.get_instrumented_functions(instrumented_functions);
  swap_and_wrap.get_swapped_functions(instrumented_functions);
  goto_model.goto_functions.update();

  // update statics and static function maps
  log.status() << "Updating CPROVER init function" << messaget::eom;
  library.create_initialize_function(instrumented_functions);
  goto_model.goto_functions.update();
}
