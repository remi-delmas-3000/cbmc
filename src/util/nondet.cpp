/*******************************************************************\

Module: Non-deterministic object init and choice for CBMC

Author: Diffblue Ltd.

\*******************************************************************/

#include "nondet.h"

#include "allocate_objects.h"
#include "arith_tools.h"
#include "c_types.h"
#include "fresh_symbol.h"
#include "symbol.h"

symbol_exprt generate_nondet_int(
  const exprt &min_value_expr,
  const exprt &max_value_expr,
  const std::string &basename_prefix,
  const source_locationt &source_location,
  allocate_objectst &allocate_objects,
  code_blockt &instructions)
{
  PRECONDITION(min_value_expr.type() == max_value_expr.type());
  const typet &int_type = min_value_expr.type();

  // Declare a symbol for the non deterministic integer.
  const symbol_exprt &nondet_symbol =
    allocate_objects.allocate_automatic_local_object(int_type, basename_prefix);
  instructions.add(code_declt(nondet_symbol));

  // Assign the symbol any non deterministic integer value.
  //   int_type name_prefix::nondet_int = NONDET(int_type)
  instructions.add(code_assignt(
    nondet_symbol, side_effect_expr_nondett(int_type, source_location)));

  // Constrain the non deterministic integer with a lower bound of `min_value`.
  //   ASSUME(name_prefix::nondet_int >= min_value)
  instructions.add(
    code_assumet(binary_predicate_exprt(nondet_symbol, ID_ge, min_value_expr)));

  // Constrain the non deterministic integer with an upper bound of `max_value`.
  //   ASSUME(name_prefix::nondet_int <= max_value)
  instructions.add(
    code_assumet(binary_predicate_exprt(nondet_symbol, ID_le, max_value_expr)));

  return nondet_symbol;
}

symbol_exprt generate_nondet_int(
  const mp_integer &min_value,
  const mp_integer &max_value,
  const std::string &basename_prefix,
  const typet &int_type,
  const source_locationt &source_location,
  allocate_objectst &allocate_objects,
  code_blockt &instructions)
{
  PRECONDITION(min_value <= max_value);
  return generate_nondet_int(
    from_integer(min_value, int_type),
    from_integer(max_value, int_type),
    basename_prefix,
    source_location,
    allocate_objects,
    instructions);
}

code_blockt generate_nondet_switch(
  const irep_idt &name_prefix,
  const alternate_casest &switch_cases,
  const typet &int_type,
  const irep_idt &mode,
  const source_locationt &source_location,
  symbol_table_baset &symbol_table)
{
  PRECONDITION(!switch_cases.empty());

  if(switch_cases.size() == 1)
    return code_blockt({switch_cases[0]});

  code_blockt result_block;

  allocate_objectst allocate_objects{
    mode, source_location, name_prefix, symbol_table};

  const symbol_exprt &switch_value = generate_nondet_int(
    0,
    switch_cases.size() - 1,
    "nondet_int",
    int_type,
    source_location,
    allocate_objects,
    result_block);

  code_blockt switch_block;
  size_t case_number = 0;
  for(const auto &switch_case : switch_cases)
  {
    code_blockt this_block;
    this_block.add(switch_case);
    this_block.add(code_breakt());
    code_switch_caset this_case(
      from_integer(case_number, switch_value.type()), this_block);
    switch_block.add(std::move(this_case));
    ++case_number;
  }

  code_switcht result_switch(switch_value, switch_block);
  result_block.add(std::move(result_switch));
  return result_block;
}