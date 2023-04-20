/*******************************************************************\

Module: Dynamic frame condition checking for function and loop contracts.

Author: Qinheping Hu, qinhh@amazon.com
Author: Remi Delmas delmasrd@amazon.com

Date: March 2023

\*******************************************************************/

/// \file
/// Class that computes CFG information about the loop structure
/// of a GOTO function for the purpose of dynamic frame conditions checking
/// and loop contract instrumentation.

#ifndef CPROVER_GOTO_INSTRUMENT_CONTRACTS_DYNAMIC_FRAMES_DFCC_CFG_INFO_H
#define CPROVER_GOTO_INSTRUMENT_CONTRACTS_DYNAMIC_FRAMES_DFCC_CFG_INFO_H

#include <util/message.h>
#include <util/namespace.h>
#include <util/std_expr.h>

#include <goto-programs/goto_program.h>

#include "dfcc_loop_contract_mode.h"

#include <map>
#include <set>

class dfcc_utilst;
class dfcc_libraryt;
class goto_modelt;
class goto_functiont;

/// \brief Describes a single loop for the purpose of DFCC loop contract
/// instrumentation.
class dfcc_loop_infot
{
public:
  dfcc_loop_infot(
    const std::size_t loop_id,
    const std::size_t cbmc_loop_id,
    const std::set<exprt> assigns,
    const exprt invariant,
    const exprt decreases,
    const std::set<irep_idt> local,
    const std::set<irep_idt> tracked,
    const std::vector<std::size_t> inner_loops,
    const std::vector<std::size_t> outer_loops,
    const symbol_exprt write_set_var,
    const symbol_exprt addr_of_write_set_var)
    : loop_id(loop_id),
      cbmc_loop_id(cbmc_loop_id),
      assigns(assigns),
      invariant(invariant),
      decreases(decreases),
      local(local),
      tracked(tracked),
      inner_loops(inner_loops),
      outer_loops(outer_loops),
      write_set_var(write_set_var),
      addr_of_write_set_var(addr_of_write_set_var)
  {
  }

  /// Prints a textual representation of the struct to \p out.
  void output(std::ostream &out, namespacet &ns, const irep_idt &mode) const;

  // Finds the instruction tagged as loop head and having the same loop
  // identifier as this struct in the given program.
  optionalt<goto_programt::instructiont::targett>
  find_head(goto_programt &goto_program) const;

  // Finds the last instruction tagged as loop latch hand having the same loop
  // loop identifier as this struct in the given program.
  optionalt<goto_programt::instructiont::targett>
  find_latch(goto_programt &goto_program) const;

  /// Loop identifier assigned by DFCC to this loop.
  const std::size_t loop_id;

  /// Loop identifier assigned to this loop by traditional CBMC loop numbering.
  const std::size_t cbmc_loop_id;

  /// Set of targets assigned by the loop, either user-provided or inferred.
  const std::set<exprt> assigns;

  /// Loop invariant expression.
  const exprt invariant;

  /// Decreases clause expression.
  const exprt decreases;

  /// Set of local identifiers locally DECL in loop instructions, excluding
  /// identifiers declared in nested loops.
  const std::set<irep_idt> local;

  /// Subset of locals that must be tracked in the loop's write set.
  /// A local must be tracked if it is dirty or might be assigned by one of
  /// the inner loops of that loop.
  const std::set<irep_idt> tracked;

  /// Integer identifiers of inner loops of that loop.
  const std::vector<std::size_t> inner_loops;

  /// Integer identifier of the outer loop(s) if it exists.
  const std::vector<std::size_t> outer_loops;

  /// Symbol representing the stack-allocated write set object for this loop.
  const symbol_exprt write_set_var;

  /// Symbol representing pointer to the stack allocated write set object for
  /// this loop. This is the one that must be passed as argument to write set
  /// functions.
  const symbol_exprt addr_of_write_set_var;
};

/// Computes natural loops, enforces normal form conditions, computes the
/// nesting graph, tags each instruction of the function with the loop ID of the
/// innermost loop and loop instruction type. Loop identifiers range from 0 to
/// nof_loops-1. Instructions that are not part of any loop are given nof_loop
/// as id and the top-level instruction kind.
///
/// For example, the following function has three loops:
///
/// ```c
/// int foo(args, __write_set_ptr_t __write_set) __assigns(A)
/// {
///   do_something();
///   while (not_done1()) __assigns(A1) __invariant(I1) __decreases(D1)
///   {
///     do_something1();
///     while (not_done2()) __assigns(A2) __invariant(I2) __decreases(D2)
///     {
///       do_something2();
///     }
///
///     if (must_break1())
///     {
///       while (not_done3()) __assigns(A3) __invariant(I3) __decreases(D3)
///       {
///         do_something3();
///       }
///       break;
///     }
///   }
/// }
/// ```
///
/// Natural loops are computed at the GOTO instruction level and a loop
/// nesting graph is generated:
///
/// ```
/// foo(A)-------------+
///  |                 |
/// loop1(A1, I1, D1) loop3(A3, I3, D3)
///  |
/// loop2(A2, I2, D2)
/// ```
///
/// GOTO instructions are tagged as follows:
///
/// ```c
/// foo /* foo */
///    CALL do_something()                   // loop_id:3 tags:{top-level}
///    SKIP; // prehead                      // loop_id:3 tags:{top-level}
/// 1: DECL __not_done1 : signedbv[32]       // loop_id:0 tags:{head}
///    CALL __not_done1 := not_done1()       // loop_id:0 tags:{body}
///    IF ¬(__not_done1 ≠ 0) THEN GOTO 7     // loop_id:0 tags:{body,exiting}
///    CALL do_something1()                  // loop_id:0 tags:{body}
///    SKIP; // prehead                      // loop_id:0 tags:{body}
/// 2: DECL __not_done2 : signedbv[32]       // loop_id:1 tags:{head}
///    CALL __not_done2 := not_done2()       // loop_id:1 tags:{body}
///    IF ¬(__not_done2 ≠ 0) THEN GOTO 3     // loop_id:1 tags:{body,exiting}
///    CALL do_something2()                  // loop_id:1 tags:{body}
///    GOTO 2                                // loop_id:1 tags:{latch}
/// 3: SKIP                                  // loop_id:0 tags:{body}
///    DECL __must_break1 : signedbv[32]     // loop_id:0 tags:{body}
///    CALL __must_break1 := must_break1()   // loop_id:0 tags:{body}
///    IF ¬(__must_break1 ≠ 0) THEN GOTO 6   // loop_id:0 tags:{body}
///    SKIP // prehead                       // loop_id:0 tags:{body}
/// 4: DECL __not_done3 : signedbv[32]       // loop_id:2 tags:{head}
///    CALL __not_done3 := not_done3()       // loop_id:2 tags:{body}
///    IF ¬(__not_done3 ≠ 0) THEN GOTO 5     // loop_id:2 tags:{body,exiting}
///    CALL do_something3()                  // loop_id:2 tags:{body}
///    GOTO 4                                // loop_id:2 tags:{latch}
/// 5: SKIP                                  // loop_id:0 tags:{body}
///    GOTO 7                                // loop_id:0 tags:{body,exiting}
/// 6: SKIP                                  // loop_id:0 tags:{body}
///    DEAD __must_break1                    // loop_id:0 tags:{body}
///    DEAD __not_done2                      // loop_id:0 tags:{body}
///    GOTO 1                                // loop_id:0 tags:{latch}
/// 7: SKIP                                  // loop_id:3 tags:{top-level}
///    DEAD __not_done1                      // loop_id:3 tags:{top-level}
///    END_FUNCTION                          // loop_id:3 tags:{top-level}
/// ```
///
/// The tags allow to the dynamic frames instrumentation pass to select the
/// write set instance of the specific loop to instrument the instruction,
/// and allow the loop contracts instrumentation pass to robustly locate head,
/// exiting nodes and latch nodes when applying the loop contract
/// transformation.
class dfcc_cfg_infot
{
public:
  dfcc_cfg_infot(
    const irep_idt &function_id,
    goto_functiont &goto_function,
    const exprt &top_level_write_set,
    const dfcc_loop_contract_modet loop_contract_mode,
    goto_modelt &goto_model,
    message_handlert &message_handler,
    dfcc_utilst &utils,
    dfcc_libraryt &library);

  void output(std::ostream &out, namespacet &ns, const irep_idt &mode) const;

  /// \brief Returns the loop info for that loop_id.
  const dfcc_loop_infot &get_loop_info(const std::size_t loop_id) const;

  /// \brief Returns the write set variable for that instruction
  const exprt &get_write_set(goto_programt::targett target) const;

  /// \brief Returns the write set variable of that loop instruction
  const symbol_exprt &get_loop_write_set(goto_programt::targett target) const;

  /// \brief Returns the write set of the outer loop of that loop or the top
  /// level write set if that loop has no outer loop.
  const exprt &get_outer_write_set(std::size_t loop_id) const;

  const std::vector<std::size_t> &get_loops_toposorted() const
  {
    return topsorted_loops;
  }

  /// Finds the DFCC id of the loop that contains the given loop, returns
  /// nullopt when the loop has no outer loop.
  const optionalt<std::size_t>
  get_outer_loop_identifier(const std::size_t loop_id) const;

  /// True iff a DECL ident must be tracked in the write set of the loop
  /// that contains the DECL.
  bool must_track_decl_or_dead(goto_programt::targett target) const;

  /// True iff the \p lhs of an assignment must be checked against
  /// the ambient write set
  bool must_check_lhs(goto_programt::targett target) const;

  const exprt &get_top_level_write_set() const
  {
    return top_level_write_set;
  }

private:
  const irep_idt &function_id;
  goto_functiont &goto_function;
  const exprt &top_level_write_set;
  messaget log;
  const namespacet ns;

  /// True iff \p id is in the valid range for a loop id or is equal to
  /// the  top level id for this function.
  bool is_valid_loop_or_top_level_id(const std::size_t id) const;

  /// True iff \p id is in the valid range for a loop id for this function.
  bool is_valid_loop_id(const std::size_t id) const;

  /// True iff \p id is in the valid range for a loop id for this function.
  bool is_top_level_id(const std::size_t id) const;

  /// Maps the identifier of a DECL instruction to loop id that contains it or
  /// to or the the top level id.
  std::map<irep_idt, std::size_t> ident_to_loop_id_map;

  /// Loop identifiers sorted from most deeply nested to less deeply nested
  std::vector<std::size_t> topsorted_loops;

  /// Loop identifiers for top level loops (ie for loops that are not nested in
  /// in another loop).
  std::vector<std::size_t> top_level_loops;

  /// Set of identifiers DECL at top level
  std::set<irep_idt> top_level_local;

  /// Set of identifiers DECL at top level
  std::set<irep_idt> top_level_tracked;

  /// Map from loop identifier to loop info struct
  std::map<std::size_t, dfcc_loop_infot> loop_info_map;
};

#endif
