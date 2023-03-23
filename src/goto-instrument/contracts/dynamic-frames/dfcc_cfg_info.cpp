/*******************************************************************\

Module: Dynamic frame condition checking for function and loop contracts.

Author: Qinheping Hu, qinhh@amazon.com
Author: Remi Delmas delmasrd@amazon.com

Date: March 2023

\*******************************************************************/

#include "dfcc_cfg_info.h"

#include <util/c_types.h>
#include <util/format_expr.h>
#include <util/fresh_symbol.h>
#include <util/pointer_expr.h>

#include <analyses/local_may_alias.h>
#include <analyses/natural_loops.h>
#include <goto-instrument/contracts/utils.h>
#include <langapi/language_util.h>

#include "dfcc_check_loop_normal_form.h"
#include "dfcc_infer_loop_assigns.h"
#include "dfcc_is_cprover_symbol.h"
#include "dfcc_library.h"
#include "dfcc_loop_nesting_graph.h"
#include "dfcc_loop_tags.h"
#include "dfcc_root_object.h"
#include "dfcc_utils.h"

/// Extracts the assigns clause expression from the latch condition
const exprt &get_assigns(const goto_programt::targett &latch_target)
{
  return static_cast<const exprt &>(
    latch_target->condition().find(ID_C_spec_assigns));
}

/// Extracts the invariant clause expression from the latch condition
const exprt &get_invariant(const goto_programt::targett &latch_target)
{
  return static_cast<const exprt &>(
    latch_target->condition().find(ID_C_spec_loop_invariant));
}

/// Extracts the decreases clause expression from the latch condition
const exprt &get_decreases(const goto_programt::targett &latch_target)
{
  return static_cast<const exprt &>(
    latch_target->condition().find(ID_C_spec_decreases));
}

/// Returns true iff some contract clause expression is attached
/// to the latch condition of this loop
bool has_contract(const goto_programt::targett &latch_target)
{
  return !get_assigns(latch_target).is_nil() ||
         !get_invariant(latch_target).is_nil() ||
         !get_decreases(latch_target).is_nil();
}

void dfcc_loop_infot::output(
  std::ostream &out,
  namespacet &ns,
  const irep_idt &mode) const
{
  out << "// dfcc_loop_id: " << loop_id << "\n";
  out << "// cbmc_loop_id: " << cbmc_loop_id << "\n";
  out << "// local: {";
  for(auto &id : local)
  {
    out << id << ", ";
  }
  out << "}\n";

  out << "// tracked: {";
  for(auto &id : tracked)
  {
    out << id << ", ";
  }
  out << "}\n";

  out << "// write_set: " << from_expr_using_mode(ns, mode, write_set_var)
      << "\n";

  out << "// addr_of_write_set: "
      << from_expr_using_mode(ns, mode, addr_of_write_set_var) << "\n";

  out << "// assigns: {";
  for(auto &expr : assigns)
  {
    out << from_expr_using_mode(ns, mode, expr) << ", ";
  }
  out << "}\n";

  out << "// invariant: " << from_expr_using_mode(ns, mode, invariant) << "\n";

  out << "// decreases: " << from_expr_using_mode(ns, mode, decreases) << "\n";

  out << "// inner loops: {"
      << "\n";
  for(auto &id : inner_loops)
  {
    out << id << ", ";
  }
  out << "}"
      << "\n";

  out << "// outer loops: {"
      << "\n";
  for(auto &id : outer_loops)
  {
    out << id << ", ";
  }
  out << "}"
      << "\n";
}

optionalt<goto_programt::instructiont::targett>
dfcc_loop_infot::find_head(goto_programt &goto_program) const
{
  for(auto target = goto_program.instructions.begin();
      target != goto_program.instructions.end();
      target++)
  {
    if(dfcc_is_loop_head(target) && dfcc_has_loop_id(target, loop_id))
    {
      return target;
    }
  }
  return {};
}

optionalt<goto_programt::instructiont::targett>
dfcc_loop_infot::find_latch(goto_programt &goto_program) const
{
  optionalt<goto_programt::instructiont::targett> result = nullopt;
  for(auto target = goto_program.instructions.begin();
      target != goto_program.instructions.end();
      target++)
  {
    // go until the end because we want to find the very last occurrence
    if(dfcc_is_loop_latch(target) && dfcc_has_loop_id(target, loop_id))
    {
      result = target;
    }
  }
  return result;
}

/// Throws an analysis exception if there exists a loop with a contract
/// with a nested loop without a contract.
static void check_inner_loop_have_contracts(
  const dfcc_loop_nesting_grapht &loop_nesting_graph)
{
  std::list<size_t> must_have_contracts;
  for(std::size_t idx = 0; idx < loop_nesting_graph.size(); idx++)
  {
    // keep track of which loops have contracts
    const auto &node = loop_nesting_graph[idx];
    if(has_contract(node.latch))
    {
      for(const auto pred_idx : loop_nesting_graph.get_predecessors(idx))
        must_have_contracts.push_back(pred_idx);
    }
  }

  while(!must_have_contracts.empty())
  {
    const auto idx = must_have_contracts.front();
    must_have_contracts.pop_front();
    const auto &node = loop_nesting_graph[idx];
    if(!has_contract(node.latch))
    {
      throw invalid_source_file_exceptiont(
        "Found loop without contract nested in a loop with a contract.\nPlease "
        "provide a contract or unwind this loop before applying loop "
        "contracts.",
        node.head->source_location());
    }

    for(const auto pred_idx : loop_nesting_graph.get_predecessors(idx))
      must_have_contracts.push_back(pred_idx);
  }
}

/// Tags instructions of loops found in \p loop_nesting_graph with the loop
/// identifier of the innermost loop it belongs to, and the loop instruction
/// type : head, body, exiting or latch.
static void tag_loop_instructions(
  goto_programt &goto_program,
  dfcc_loop_nesting_grapht &loop_nesting_graph)
{
  for(const auto &idx : loop_nesting_graph.topsort())
  {
    auto &node = loop_nesting_graph[idx];
    auto &head = node.head;
    auto &latch = node.latch;
    auto &instructions = node.instructions;

    dfcc_set_loop_head(head);
    dfcc_set_loop_latch(latch);

    for(auto t : instructions)
    {
      // Skip instructions that are already tagged and belong to inner loops.
      auto loop_id_opt = dfcc_get_loop_id(t);
      if(loop_id_opt.has_value() && loop_id_opt.value() != idx)
        continue;

      dfcc_set_loop_id(t, idx);

      if(t != head && t != latch)
        dfcc_set_loop_body(t);

      if(t->is_goto() && !instructions.contains(t->get_target()))
      {
        dfcc_set_loop_exiting(t);
      }
    }
  }

  auto top_level_id = loop_nesting_graph.size();

  // tag remaining instructions as top level
  for(auto target = goto_program.instructions.begin();
      target != goto_program.instructions.end();
      target++)
  {
    // Skip instructions that are already tagged (belong to inner loops).
    auto loop_id_opt = dfcc_get_loop_id(target);
    if(loop_id_opt.has_value() && loop_id_opt.value() != top_level_id)
    {
      continue;
    }
    dfcc_set_loop_id(target, top_level_id);
    dfcc_set_loop_top_level(target);
  }
}

/// Utility class that generates a \ref dfcc_loop_infot struct for each loop
/// of the given \p loop_nesting_graph and inserts them into \p loop_info_map.
/// This class is used by the constructor of \ref dfcc_cfg_infot.
class gen_dfcc_loop_infot
{
public:
  gen_dfcc_loop_infot(
    const irep_idt &function_id,
    goto_functiont &goto_function,
    goto_modelt &goto_model,
    dfcc_utilst &utils,
    dfcc_libraryt &library,
    message_handlert &message_handler,
    dfcc_loop_nesting_grapht &loop_nesting_graph,
    std::map<std::size_t, dfcc_loop_infot> &loop_info_map,
    std::map<irep_idt, std::size_t> &ident_to_loop_id_map,
    std::vector<std::size_t> &top_level_loops,
    std::set<irep_idt> &top_level_local,
    std::set<irep_idt> &top_level_tracked)
    : function_id(function_id),
      goto_function(goto_function),
      goto_model(goto_model),
      utils(utils),
      library(library),
      log(message_handler),
      loop_nesting_graph(loop_nesting_graph),
      loop_info_map(loop_info_map),
      ident_to_loop_id_map(ident_to_loop_id_map),
      top_level_loops(top_level_loops),
      top_level_local(top_level_local),
      top_level_tracked(top_level_tracked),
      local_may_alias(goto_function),
      dirty(goto_function),
      ns(goto_model.symbol_table)
  {
    apply();
  }

private:
  const irep_idt &function_id;
  goto_functiont &goto_function;
  goto_modelt &goto_model;
  dfcc_utilst &utils;
  dfcc_libraryt &library;
  messaget log;
  dfcc_loop_nesting_grapht &loop_nesting_graph;
  std::map<std::size_t, dfcc_loop_infot> &loop_info_map;
  std::map<irep_idt, std::size_t> &ident_to_loop_id_map;
  std::vector<std::size_t> &top_level_loops;
  std::set<irep_idt> &top_level_local;
  std::set<irep_idt> &top_level_tracked;
  local_may_aliast local_may_alias;
  dirtyt dirty;
  const namespacet ns;

  bool is_assigned(const irep_idt &ident, assignst assigns)
  {
    PRECONDITION(!dirty(ident));
    // For each assigns clause target
    for(const auto &expr : assigns)
    {
      auto root_objects = dfcc_root_objects(expr);
      for(const auto &root_object : root_objects)
      {
        if(
          root_object.id() == ID_symbol &&
          to_symbol_expr(root_object).get_identifier() == ident)
        {
          return true;
        }
        // If `root_object` is not a symbol, then it contains a combination of
        // address-of and dereference operators that cannot be statically
        // resolved to a symbol.
        // Since we know `ident` is not dirty, we know that dereference
        // operations cannot land on that `ident`. So the root_object cannot
        // describe a memory location within the object backing that ident.
        // We conclude that ident is not assigned by this target and move on to
        // the next target.
      }
    }
    return false;
  }

  void apply()
  {
    std::size_t top_level_id = loop_nesting_graph.size();

    // initialize the DECL ident -> loop id map with top level id
    for(auto target = goto_function.body.instructions.begin();
        target != goto_function.body.instructions.end();
        target++)
    {
      if(target->is_decl())
      {
        ident_to_loop_id_map[target->decl_symbol().get_identifier()] =
          top_level_id;
      }
    }

    for(std::size_t loop_id = 0; loop_id < loop_nesting_graph.size(); loop_id++)
    {
      // Recursive traversal from each top level loop
      // (top level loops do not have successors)
      if(loop_nesting_graph.get_successors(loop_id).empty())
      {
        top_level_loops.push_back(loop_id);
        apply_rec(loop_id);
      }
    }

    // Compute set of locals at function level
    top_level_local.insert(
      goto_function.parameter_identifiers.begin(),
      goto_function.parameter_identifiers.end());

    for(const auto &entry : ident_to_loop_id_map)
    {
      if(entry.second == loop_info_map.size())
      {
        top_level_local.insert(entry.first);
      }
    }

    // Compute subset of function locals that must be tracked in the function's
    // write set. A local must be tracked if it is dirty or if it may be
    // assigned by one of the top level loops.
    for(const auto &ident : top_level_local)
    {
      if(dirty(ident))
      {
        top_level_tracked.insert(ident);
      }
      else
      {
        // Check if this ident is touched by one of the top level loops
        for(const auto loop_id : top_level_loops)
        {
          if(is_assigned(ident, loop_info_map.at(loop_id).assigns))
          {
            top_level_tracked.insert(ident);
          }
        }
      }
    }
  }

  void apply_rec(const std::size_t loop_id)
  {
    // update the DECL ident -> loop id map on the way down
    for(auto target : loop_nesting_graph[loop_id].instructions)
    {
      if(target->is_decl())
      {
        ident_to_loop_id_map[target->decl_symbol().get_identifier()] = loop_id;
      }
    }

    // Process inner loops
    for(const auto inner_loop_id : loop_nesting_graph.get_predecessors(loop_id))
    {
      apply_rec(inner_loop_id);
    }

    auto &loop = loop_nesting_graph[loop_id];

    // Collect identifiers that are local to this loop only
    // (excluding nested loop).
    std::set<irep_idt> loop_local;
    for(const auto &target : loop.instructions)
    {
      if(target->is_decl())
      {
        auto &ident = target->decl_symbol().get_identifier();
        if(ident_to_loop_id_map[ident] == loop_id)
          loop_local.insert(ident);
      }
    }

    // Compute subset of locals that must be tracked in the loop's write set.
    // A local must be tracked if it is dirty or if it may be assigned by one
    // of the inner loops.
    std::set<irep_idt> loop_tracked;
    for(const auto &ident : loop_local)
    {
      if(dirty(ident))
      {
        loop_tracked.insert(ident);
      }
      else
      {
        // Check if this ident is touched by one of the inner loops
        for(const auto inner_loop_id :
            loop_nesting_graph.get_predecessors(loop_id))
        {
          if(is_assigned(ident, loop_info_map.at(inner_loop_id).assigns))
            loop_tracked.insert(ident);
        }
      }
    }

    // Process loop contract clauses
    exprt invariant_expr = get_invariant(loop.latch);
    exprt decreases_expr = get_decreases(loop.latch);
    exprt assigns_expr = get_assigns(loop.latch);
    std::set<exprt> assigns;

    // Generate defaults for all clauses if some clause is defined
    if(
      invariant_expr.is_not_nil() || decreases_expr.is_not_nil() ||
      assigns_expr.is_not_nil())
    {
      if(invariant_expr.is_nil())
      {
        // use a default invariant if none given.
        invariant_expr = true_exprt{};
        // assigns clause is missing; we will try to automatic inference
        log.warning() << "No invariant provided for loop " << function_id << "."
                      << loop.latch->loop_number << " at "
                      << loop.head->source_location()
                      << ". Using 'true' as a sound default invariant. Please "
                         "provide an invariant the default is too weak."
                      << messaget::eom;
      }
      else
      {
        // conjoin all invariant clauses
        invariant_expr = conjunction(invariant_expr.operands());
      }
      // unpack assigns clause targets
      auto &loop_assigns = get_assigns(loop.latch);
      if(loop_assigns.is_not_nil())
      {
        for(auto &operand : loop_assigns.operands())
        {
          assigns.insert(operand);
        }
      }
      else
      {
        // infer assigns clause targets if none given
        auto inferred = dfcc_infer_loop_assigns(
          local_may_alias, loop.instructions, loop.head->source_location(), ns);
        log.warning() << "No assigns clause provided for loop " << function_id
                      << "." << loop.latch->loop_number << " at "
                      << loop.head->source_location() << ". The inferred set {";
        bool first = true;
        for(const auto &expr : inferred)
        {
          if(!first)
          {
            log.status() << ", ";
          }
          first = false;
          log.status() << from_expr(ns, function_id, expr);
        }
        log.status() << "} might be incomplete or imprecise, please provide an "
                        "assigns clause if the analysis fails."
                     << messaget::eom;
        assigns.swap(inferred);
      }

      if(decreases_expr.is_nil())
      {
        log.warning() << "No decrease clause provided for loop " << function_id
                      << "." << loop.latch->loop_number << " at "
                      << loop.head->source_location()
                      << ". Termination will not be checked." << messaget::eom;
      }
    }

    std::vector<std::size_t> inner_loops;
    for(auto pred_idx : loop_nesting_graph.get_predecessors(loop_id))
    {
      inner_loops.push_back(pred_idx);
    }

    std::vector<std::size_t> outer_loops;
    for(auto succ_idx : loop_nesting_graph.get_successors(loop_id))
    {
      outer_loops.push_back(succ_idx);
    }

    const auto cbmc_loop_number = loop.latch->loop_number;
    const auto &language_mode = utils.get_function_symbol(function_id).mode;

    // Generate "write set" variable
    const auto &write_set_var =
      get_fresh_aux_symbol(
        library.dfcc_type[dfcc_typet::WRITE_SET],
        id2string(function_id),
        "__write_set_loop_" + std::to_string(cbmc_loop_number),
        loop.head->source_location(),
        language_mode,
        goto_model.symbol_table)
        .symbol_expr();

    // Generate "address of write set" variable
    const auto &addr_of_write_set_var =
      get_fresh_aux_symbol(
        library.dfcc_type[dfcc_typet::WRITE_SET_PTR],
        id2string(function_id),
        "__address_of_write_set_loop_" + std::to_string(cbmc_loop_number),
        loop.head->source_location(),
        language_mode,
        goto_model.symbol_table)
        .symbol_expr();

    // Insert entry in map and we're done
    loop_info_map.insert(
      {loop_id,
       {loop_id,
        cbmc_loop_number,
        assigns,
        invariant_expr,
        decreases_expr,
        loop_local,
        loop_tracked,
        inner_loops,
        outer_loops,
        write_set_var,
        addr_of_write_set_var}});
  }
};

dfcc_cfg_infot::dfcc_cfg_infot(
  const irep_idt &function_id,
  goto_functiont &goto_function,
  const exprt &top_level_write_set,
  const dfcc_loop_contract_modet loop_contract_mode,
  goto_modelt &goto_model,
  message_handlert &message_handler,
  dfcc_utilst &utils,
  dfcc_libraryt &library)
  : function_id(function_id),
    goto_function(goto_function),
    top_level_write_set(top_level_write_set),
    log(message_handler),
    ns(goto_model.symbol_table)
{
  local_may_aliast local_may_alias(goto_function);
  dfcc_loop_nesting_grapht loop_nesting_graph;
  goto_programt &goto_program = goto_function.body;
  messaget log(message_handler);
  if(loop_contract_mode != dfcc_loop_contract_modet::NONE)
  {
    dfcc_check_loop_normal_form(goto_program, log);
    loop_nesting_graph = build_loop_nesting_graph(goto_program, log);
    check_inner_loop_have_contracts(loop_nesting_graph);

    auto topsorted = loop_nesting_graph.topsort();
    for(const auto idx : topsorted)
    {
      topsorted_loops.push_back(idx);
    }
  }

  tag_loop_instructions(goto_program, loop_nesting_graph);

  // initialize attributes using auxiliary class
  gen_dfcc_loop_infot(
    function_id,
    goto_function,
    goto_model,
    utils,
    library,
    message_handler,
    loop_nesting_graph,
    loop_info_map,
    ident_to_loop_id_map,
    top_level_loops,
    top_level_local,
    top_level_tracked);
}

void dfcc_cfg_infot::output(
  std::ostream &out,
  namespacet &ns,
  const irep_idt &mode) const
{
  out << "// dfcc_cfg_infot for:  " << function_id << "\n";
  out << "// top_level_local: {";
  for(auto &id : top_level_local)
  {
    out << id << ", ";
  }
  out << "}\n";

  out << "// top_level_tracked: {";
  for(auto &id : top_level_tracked)
  {
    out << id << ", ";
  }
  out << "}\n";

  out << "// loop:\n";
  for(auto &loop : loop_info_map)
  {
    out << "// dfcc-loop_id:" << loop.first << "\n";
    auto head = loop.second.find_head(goto_function.body);
    auto latch = loop.second.find_latch(goto_function.body);
    out << "// head:\n";
    head.value()->output(out);
    out << "// latch:\n";
    latch.value()->output(out);
    loop.second.output(out, ns, mode);
  }
  out << "// program:\n";
  Forall_goto_program_instructions(target, goto_function.body)
  {
    out << "// dfcc-loop-id:" << dfcc_get_loop_id(target).value();
    out << " cbmc-loop-number:" << target->loop_number;
    out << " top-level:" << dfcc_is_loop_top_level(target);
    out << " head:" << dfcc_is_loop_head(target);
    out << " body:" << dfcc_is_loop_body(target);
    out << " exiting:" << dfcc_is_loop_exiting(target);
    out << " latch:" << dfcc_is_loop_latch(target);
    out << "\n";
    target->output(out);
  }
}

const exprt &dfcc_cfg_infot::get_write_set(goto_programt::targett target) const
{
  auto loop_id_opt = dfcc_get_loop_id(target);
  PRECONDITION(
    loop_id_opt.has_value() &&
    is_valid_loop_or_top_level_id(loop_id_opt.value()));
  auto loop_id = loop_id_opt.value();
  if(is_top_level_id(loop_id))
  {
    return top_level_write_set;
  }
  else
  {
    return loop_info_map.at(loop_id).addr_of_write_set_var;
  }
}

const symbol_exprt &
dfcc_cfg_infot::get_loop_write_set(goto_programt::targett target) const
{
  auto loop_id_opt = dfcc_get_loop_id(target);
  PRECONDITION(
    loop_id_opt.has_value() && is_valid_loop_id(loop_id_opt.value()));
  auto loop_id = loop_id_opt.value();
  return loop_info_map.at(loop_id).addr_of_write_set_var;
}

const exprt &dfcc_cfg_infot::get_outer_write_set(std::size_t loop_id) const
{
  PRECONDITION(is_valid_loop_id(loop_id));
  auto outer_loop_opt = get_outer_loop_identifier(loop_id);
  return outer_loop_opt.has_value()
           ? get_loop_info(outer_loop_opt.value()).addr_of_write_set_var
           : top_level_write_set;
}

const dfcc_loop_infot &
dfcc_cfg_infot::get_loop_info(const std::size_t loop_id) const
{
  PRECONDITION(is_valid_loop_id(loop_id));
  return loop_info_map.at(loop_id);
}

// find the identifier or the immediately enclosing loop in topological order
const optionalt<std::size_t>
dfcc_cfg_infot::get_outer_loop_identifier(const std::size_t loop_id) const
{
  PRECONDITION(is_valid_loop_id(loop_id));
  auto outer_loops = get_loop_info(loop_id).outer_loops;

  // find the first loop in the topological order that is connected
  // to our node.
  for(const auto &idx : get_loops_toposorted())
  {
    if(
      std::find(outer_loops.begin(), outer_loops.end(), idx) !=
      outer_loops.end())
    {
      return idx;
    }
  }
  // return nullopt for loops that are not nested in other loops
  return nullopt;
}

bool dfcc_cfg_infot::is_valid_loop_or_top_level_id(const std::size_t id) const
{
  return id <= loop_info_map.size();
}

bool dfcc_cfg_infot::is_valid_loop_id(const std::size_t id) const
{
  return id < loop_info_map.size();
}

bool dfcc_cfg_infot::is_top_level_id(const std::size_t id) const
{
  return id == loop_info_map.size();
}

bool dfcc_cfg_infot::must_track_decl_or_dead(
  goto_programt::targett target) const
{
  PRECONDITION(target->is_decl() || target->is_dead());
  auto loop_id_opt = dfcc_get_loop_id(target);
  PRECONDITION(loop_id_opt.has_value());
  auto loop_id = loop_id_opt.value();
  auto &ident = target->is_decl() ? target->decl_symbol().get_identifier()
                                  : target->dead_symbol().get_identifier();
  if(is_top_level_id(loop_id))
  {
    return top_level_tracked.find(ident) != top_level_tracked.end();
  }
  else
  {
    auto &loop_info = loop_info_map.at(loop_id);
    return loop_info.tracked.find(ident) != loop_info.tracked.end();
  }
}

static bool __must_check_lhs(
  const exprt &lhs,
  const std::set<irep_idt> &local,
  const std::set<irep_idt> &tracked,
  messaget log,
  const namespacet &ns)
{
  if(can_cast_expr<symbol_exprt>(lhs))
  {
    const auto &id = to_symbol_expr(lhs).get_identifier();

    // direct assignments to local symbols can be skipped
    if(local.find(id) != local.end())
      return false;

    // this is a global symbol. Ignore if it is one of the CPROVER globals
    return !dfcc_is_cprover_symbol(id);
  }
  else
  {
    // This is a complex expression. Compute the set of root objects expressions
    // it can assign.
    auto root_objects = dfcc_root_objects(lhs);

    std::unordered_set<irep_idt> root_idents;

    // Check wether all root_objects can be resolved to actual identifiers.
    for(const auto &expr : root_objects)
    {
      if(expr.id() != ID_symbol)
      {
        // This means that the lhs contains either an address-of operation or a
        // dereference operation, and we cannot really know statically which
        // object it refers to without using the may_alias analysis.
        // Since the may_alias analysis is used to infer targets, for soundness
        // reasons we cannot also use it to skip checks.
        // We check the assignment.
        // If happens to assign to a mix of tracked and non-tracked
        // identifiers the check will fail but this is sound anyway.
        return true;
      }
      root_idents.insert(to_symbol_expr(expr).get_identifier());
    }

    // The root idents set is Non-empty.
    // true iff root_idents contains non-local idents
    bool some_non_local = false;
    // true iff root_idents contains some local that is not tracked
    bool some_local_not_tracked = false;
    // true iff root_idents contains only local that are not tracked
    bool all_local_not_tracked = true;
    // true iff root_idents contains only local that are tracked
    bool all_local_tracked = true;
    for(const auto &root_ident : root_idents)
    {
      bool loc = local.find(root_ident) != local.end();
      bool tra = tracked.find(root_ident) != tracked.end();
      bool local_tracked = loc && tra;
      bool local_not_tracked = loc && !tra;
      some_non_local |= !loc;
      some_local_not_tracked |= local_not_tracked;
      all_local_not_tracked &= local_not_tracked;
      all_local_tracked &= local_tracked;
    }

    // some based identifier is not local, must check
    if(some_non_local)
    {
      // if we also have a local that is not tracked, we know the check will
      // fail with the current algorithm, error out.
      if(some_local_not_tracked)
      {
        throw analysis_exceptiont(
          "LHS expression `" + from_expr(lhs) +
          "` in assignment mentions both explicitly and implicitly tracked "
          "memory locations. DFCC does not yet handle that case, please "
          "reformulate the assignment into separate assignments to either "
          "memory locations.");
      }
      return true;
    }
    else
    {
      // all base idents are local
      // if they are all not tracked, we *have* to skip the check
      // if they are all tracked, we *can* skip the check
      // in both cases it is sound to do so.
      if(all_local_not_tracked || all_local_tracked)
      {
        return false;
      }
      else
      {
        // we have a combination of tracked and not-tracked locals, we know
        // the check will fail with the current algorithm, error out.
        throw analysis_exceptiont(
          "LHS expression `" + from_expr(lhs) +
          "` in assignment mentions both explicitly and implicitly tracked "
          "memory locations. DFCC does not yet handle that case, please "
          "reformulate the assignment into separate assignments to either "
          "memory locations.");
      }
    }
  }
}

bool dfcc_cfg_infot::must_check_lhs(goto_programt::targett target) const
{
  PRECONDITION(target->is_assign() || target->is_function_call());

  const exprt &lhs =
    target->is_assign() ? target->assign_lhs() : target->call_lhs();

  if(lhs.is_nil())
    return false;

  auto loop_id_opt = dfcc_get_loop_id(target);
  PRECONDITION(
    loop_id_opt.has_value() &&
    is_valid_loop_or_top_level_id(loop_id_opt.value()));
  auto loop_id = loop_id_opt.value();

  if(is_top_level_id(loop_id))
  {
    return __must_check_lhs(lhs, top_level_local, top_level_tracked, log, ns);
  }
  else
  {
    auto &loop_info = loop_info_map.at(loop_id);
    return __must_check_lhs(lhs, loop_info.local, loop_info.tracked, log, ns);
  }
}
