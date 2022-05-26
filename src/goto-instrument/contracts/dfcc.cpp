#include "dfcc.h"

#include <util/c_types.h>
#include <util/config.h>
#include <util/namespace.h>
#include <util/std_expr.h>
// #include <util/message.h>
// #include <util/namespace.h>
// #include <util/optional.h>
// #include <util/pointer_expr.h>

#include <goto-programs/goto_convert_class.h>

#include <util/config.h>

#include <goto-programs/goto_functions.h>
#include <goto-programs/goto_model.h>
#include <goto-programs/instrument_preconditions.h>
#include <goto-programs/link_to_library.h>

#include <ansi-c/cprover_library.h>
// #include <goto-instrument/loop_utils.h>

#include <iostream>

// initialise static class data out of line
const std::map<dfcc_typet, irep_idt> dfcct::dfcc_type_name = {
  {dfcc_typet::CAR, CPROVER_PREFIX "assignable_car_t"},
  {dfcc_typet::CAR_LIST, CPROVER_PREFIX "assignable_car_list_t"},
  {dfcc_typet::OBJ_LIST, CPROVER_PREFIX "assignable_obj_list_t"}};

const std::map<dfcc_funt, irep_idt> dfcct::dfcc_fun_name = {
  {dfcc_funt::CAR_CREATE, CPROVER_PREFIX "assignable_car_create"},
  {dfcc_funt::CAR_LIST_EMPTY, CPROVER_PREFIX "assignable_car_list_empty"},
  {dfcc_funt::CAR_LIST_ADD, CPROVER_PREFIX "assignable_car_list_add"},
  {dfcc_funt::CAR_LIST_INVALIDATE_SAME_OBJECT,
   CPROVER_PREFIX "assignable_car_list_invalidate_same_object"},
  {dfcc_funt::CAR_CONTAINS_CAR, CPROVER_PREFIX "assignable_car_contains_car"},
  {dfcc_funt::CAR_LIST_CONTAINS_CAR,
   CPROVER_PREFIX "assignable_car_list_contains_car"},
  {dfcc_funt::CAR_LIST_HAVOC, CPROVER_PREFIX "assignable_car_list_havoc"},
  {dfcc_funt::OBJ_LIST_EMPTY, CPROVER_PREFIX "assignable_obj_list_empty"},
  {dfcc_funt::OBJ_LIST_ADD, CPROVER_PREFIX "assignable_obj_list_add"},
  {dfcc_funt::OBJ_MAP_EMPTY, CPROVER_PREFIX "assignable_obj_map_empty"},
  {dfcc_funt::OBJ_MAP_COPY, CPROVER_PREFIX "assignable_obj_map_copy"},
  {dfcc_funt::OBJ_MAP_ADD, CPROVER_PREFIX "assignable_obj_map_add"},
  {dfcc_funt::OBJ_MAP_REMOVE, CPROVER_PREFIX "assignable_obj_map_remove"},
  {dfcc_funt::CHECK_ASSIGNMENT, CPROVER_PREFIX "assignable_check_assignment"},
  {dfcc_funt::CHECK_ARRAY_SET, CPROVER_PREFIX "assignable_check_array_set"},
  {dfcc_funt::CHECK_ARRAY_COPY, CPROVER_PREFIX "assignable_check_array_copy"},
  {dfcc_funt::CHECK_ARRAY_REPLACE,
   CPROVER_PREFIX "assignable_check_array_replace"},
  {dfcc_funt::CHECK_HAVOC_OBJECT,
   CPROVER_PREFIX "assignable_check_havoc_object"},
  {dfcc_funt::CHECK_DEALLOCATE, CPROVER_PREFIX "assignable_check_deallocate"},
  {dfcc_funt::CHECK_ASSIGNS_CLAUSE_INCLUSION,
   CPROVER_PREFIX "assignable_check_assigns_clause_inclusion"},
  {dfcc_funt::CHECK_FREES_CLAUSE_INCLUSION,
   CPROVER_PREFIX "assignable_check_frees_clause_inclusion"},
  {dfcc_funt::OBJ_LIST_DEALLOCATE,
   CPROVER_PREFIX "assignable_obj_list_deallocate"}};

/// True if 's1' contains 's2'.
static bool string_contains(std::string s1, std::string s2)
{
  return s1.find(s2) != std::string::npos;
}

void dfcc(goto_modelt &goto_model, messaget &log, irep_idt &harness_function)
{
  dfcct{goto_model, log}.transform_goto_model(harness_function);
}

dfcct::dfcct(goto_modelt &goto_model, messaget &log)
  : goto_model(goto_model),
    log(log),
    message_handler(log.get_message_handler()),
    converter(goto_model.symbol_table, log.get_message_handler())
{
  object_map_type = array_typet(c_bool_typet(8), infinity_exprt(size_type()));
}

void dfcct::get_missing_dfcc_funs(std::set<irep_idt> &missing)
{
  missing.clear();
  for(const auto &pair : dfcct::dfcc_fun_name)
  {
    symbol_tablet::symbolst::const_iterator found =
      goto_model.symbol_table.symbols.find(pair.second);

    if(
      found != goto_model.symbol_table.symbols.end() &&
      found->second.value.is_nil())
      missing.insert(pair.second);
  }
}

void dfcct::load_dfcc_library()
{
  std::set<irep_idt> missing;
  get_missing_dfcc_funs(missing);

  symbol_tablet tmp_symbol_table;
  cprover_c_library_factory_force_load(
    missing, tmp_symbol_table, message_handler);

  // copy all missing symbols to the main symbol table
  for(const auto &symbol_pair : tmp_symbol_table.symbols)
  {
    if(
      string_contains(
        id2string(symbol_pair.first), CPROVER_PREFIX "assignable") ||
      id2string(symbol_pair.first) == CPROVER_PREFIX "assert")
    {
      goto_model.symbol_table.insert(symbol_pair.second);
    }
  }

  // compile all missing symbols to goto
  for(const auto &id : missing)
  {
    goto_convert(
      id, goto_model.symbol_table, goto_model.goto_functions, message_handler);
  }

  // check that all symbols have a goto_implementation
  // and populate symbol maps
  namespacet ns(goto_model.symbol_table);
  for(const auto &pair : dfcc_fun_name)
  {
    const auto &found =
      goto_model.goto_functions.function_map.find(pair.second);
    if(!(found != goto_model.goto_functions.function_map.end() &&
         found->second.body_available()))
    {
      log.error() << "dfcc: GOTO body of dfcc function " << pair.second
                  << " was not found" << messaget::eom;
      throw 0;
    }
    dfcc_fun_symbol[pair.first] = ns.lookup(pair.second);
  }

  // populate symbol maps for easy access to symbols during translation
  for(const auto &pair : dfcct::dfcc_type_name)
  {
    dfcc_type_symbol[pair.first] = ns.lookup(pair.second);
  }
}

void dfcct::transform_goto_model(const irep_idt &harness_function)
{
  // load the cprover library to make sure the model is complete
  log.status() << "Adding CPROVER library (" << config.ansi_c.arch << ")"
               << messaget::eom;
  link_to_library(goto_model, message_handler, cprover_c_library_factory);

  // load the dfcc library before instrumentation starts
  load_dfcc_library();

  // shortcuts
  goto_functionst &goto_functions = goto_model.goto_functions;
  symbol_tablet &symbol_table = goto_model.symbol_table;

  // iterate over all previously known symbols as the body of the loop modifies
  // the symbol table and can thus invalidate iterators
  for(auto &sym_name : symbol_table.sorted_symbol_names())
  {
    const typet &type = symbol_table.lookup_ref(sym_name).type;

    // only process functions
    if(type.id() != ID_code)
      continue;

    goto_functionst::function_mapt::iterator fct_entry =
      goto_functions.function_map.find(sym_name);

    // process the function
    if(fct_entry != goto_functions.function_map.end())
    {
      // the function was found
      code_typet code_type = to_code_type(type);
      typet return_type = code_type.return_type();
      code_with_contract_typet code_with_contract_type =
        to_code_with_contract_type(code_type);

      // true iff function returns __CPROVER_assignable_t
      bool is_spec_assigns_function =
        return_type.id() == ID_empty &&
        return_type.get(ID_C_typedef) == CPROVER_PREFIX "assignable_t";
      ;

      // true iff function returns __CPROVER_freeable_t
      bool is_spec_frees_function =
        return_type.id() == ID_empty &&
        return_type.get(ID_C_typedef) == CPROVER_PREFIX "freeable_t";

      if(sym_name == harness_function)
      {
        transform_harness_function(sym_name);
      }
      else if(is_spec_assigns_function)
      {
        // convert to spec version
        to_dfcc_spec_assigns_function(sym_name);
      }
      else if(is_spec_frees_function)
      {
        // convert to spec version
        to_dfcc_spec_frees_function(sym_name);
      }
      else
      {
        // generate spec function from assigns clause if any
        if(!code_with_contract_type.assigns().empty())
        {
          dfcc_spec_assigns_function_from_contract(sym_name);
        }

        // generate spec function from frees clause if any
        // if(!code_with_contract_type.frees().empty())
        // {
        //   dfcc_spec_frees_function_from_contract(sym_name);
        // }

        // convert to checked function
        to_dfcc_checked_function(sym_name);
      }
    }
    else
    {
      // abort on missing functions since if they ever get called/used
      // they would not be transformed/available
      log.error() << "dfcc: GOTO function " << sym_name << " was not found"
                  << messaget::eom;
      throw 0;
    }
  }
  /////////////
}

void dfcct::transform_harness_function(const irep_idt &function_id)
{
  // TODO
}

void dfcct::to_dfcc_spec_assigns_function(const irep_idt &function_id)
{
  // add parameter __CPROVER_assignable_car_list_t **list
  // to symbol_table code representation and goto_function
  // Rewrite calls:
  // ```
  // CALL __CPROVER_assignable(ptr, size, is_ptr_to_ptr); ~>
  // CALL __CPROVER_assignable_car_list_add(list, ptr, size, is_ptr_to_ptr);
  // ```
}

void dfcct::dfcc_spec_assigns_function_from_contract(
  const irep_idt &function_id)
{
  // create new goto function
  //
  // ```
  // __CPROVER_assignable_t function_id::assigns(
  //  function-params, __CPROVER_assignable_car_list_t **list)
  // ```
  // for each conditional target `cond: target` of the assigns clause
  // generate code
  //
  // ```
  // IF !cond GOTO SKIP_TARGET;
  // CALL __CPROVER_assignable_car_list_add(list, target, size, is_pointer);
  // SKIP_TARGET: SKIP;
  // ```
  //
  // for target expressions that are calls to built-ins
  // for __CPROVER_whole_object
  // for __CPROVER_object_from
  // for __CPROVER_object_upto
  // for __CPROVER_assignable
  //
  // ```
  // IF !cond GOTO SKIP_TARGET;
  // CALL __CPROVER_assignable_car_list_add(list, target, size, is_pointer);
  // SKIP_TARGET: SKIP;
  // ```
  //
  // for target expressions that are calls to user-defined
  // __CPROVER_assignable_t functions `cond: foo(params)`, rewrite to
  //
  // ```
  // IF !cond GOTO SKIP_TARGET;
  // CALL list = foo(target, list);
  // SKIP_TARGET: SKIP;
  // ```
}

void dfcct::to_dfcc_spec_frees_function(const irep_idt &function_id)
{
  // fetch symbol table entry and goto_function
  // add all extra parameters to symbol_table entry and goto_function
  // add parameter __CPROVER_assignable_obj_list_t **list
  // rewrite calls:
  // CALL __CPROVER_object_from(ptr); ~>
  // CALL __CPROVER_assignable_obj_list_add(list, ptr);
}

void dfcct::dfcc_spec_frees_function_from_contract(const irep_idt &function_id)
{
  // create new symbol and goto function
  // ```
  // __CPROVER_freeable_t
  //   function::frees(function-params, __CPROVER_assignable_obj_list_t **list)
  // ```
  //
  // for each conditional target `cond: ptr` of the frees clause
  // generate code
  //
  // ```
  // IF !cond GOTO SKIP_TARGET;
  // CALL list = __CPROVER_assignable_obj_list_add(list, ptr);
  // SKIP_TARGET: SKIP;
  // ```
  //
  // for calls to user-defined functions that return __CPROVER_freeable_t
  // ```
  // IF !cond GOTO SKIP_TARGET;
  // CALL foo(target, list);
  // SKIP_TARGET: SKIP;
  // ```
}

void dfcct::to_dfcc_checked_function(const irep_idt &function_id)
{
  // fetch symbol table entry and goto_function

  // add all extra parameters to symbol_table entry and goto_function

  // traverse instructions for instrumentation
}