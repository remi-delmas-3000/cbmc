/*******************************************************************\

Module: Dynamic frame condition checking

Author: Remi Delmas, delmasrd@amazon.com

\*******************************************************************/

/// \file
/// Dynamic frame condition checking

#ifndef CPROVER_GOTO_INSTRUMENT_CONTRACTS_DFCC_H
#define CPROVER_GOTO_INSTRUMENT_CONTRACTS_DFCC_H

#include <util/std_expr.h>

#include <goto-programs/goto_convert_functions.h>

//#include <list>
#include <map>
#include <set>
//#include <unordered_set>

class goto_modelt;
class messaget;
class message_handlert;
class symbolt;

#define FLAG_DFCC "dfcc"
#define HELP_DFCC                                                              \
  "--dfcc                       use dynamic frame condition checking for "     \
  "contracts"

/// Library types used in the instrumentation
enum class dfcc_typet
{
  CAR,
  CAR_LIST,
  OBJ_LIST
};

/// Library functions used in the instrumentation
enum class dfcc_funt
{
  CAR_CREATE,
  CAR_LIST_EMPTY,
  CAR_LIST_ADD,
  CAR_LIST_INVALIDATE_SAME_OBJECT,
  CAR_CONTAINS_CAR,
  CAR_LIST_CONTAINS_CAR,
  CAR_LIST_HAVOC,
  OBJ_LIST_EMPTY,
  OBJ_LIST_ADD,
  OBJ_MAP_EMPTY,
  OBJ_MAP_COPY,
  OBJ_MAP_ADD,
  OBJ_MAP_REMOVE,
  CHECK_ASSIGNMENT,
  CHECK_ARRAY_SET,
  CHECK_ARRAY_COPY,
  CHECK_ARRAY_REPLACE,
  CHECK_HAVOC_OBJECT,
  CHECK_DEALLOCATE,
  CHECK_ASSIGNS_CLAUSE_INCLUSION,
  CHECK_FREES_CLAUSE_INCLUSION,
  OBJ_LIST_DEALLOCATE
};

class dfcct
{
public:
  dfcct(goto_modelt &goto_model, messaget &log);

  // enum to type name mapping
  static const std::map<dfcc_typet, irep_idt> dfcc_type_name;

  // enum to function name mapping
  static const std::map<dfcc_funt, irep_idt> dfcc_fun_name;

  /// Applies the dfcc transformation to all functions found in the goto model.
  /// Except for the harness_function used as the entry point for contract
  /// checking.
  void transform_goto_model(const irep_idt &harness_function);

protected:
  goto_modelt &goto_model;
  messaget &log;
  message_handlert &message_handler;
  goto_convertt converter;

  /// The `bool[__CPROVER_constant_infinity_uint]` type
  typet object_map_type;

  /// enum-to-symbol mapping (dynamically loaded)
  std::map<dfcc_typet, symbolt> dfcc_type_symbol;

  /// enum-to-symbol mapping (dynamically loaded)
  std::map<dfcc_funt, symbolt> dfcc_fun_symbol;

  /// Collects the names of all missing dfcc functions
  void get_missing_dfcc_funs(std::set<irep_idt> &missing);

  /// Adds all the `cprover_dfcc.c` library types and functions to the model
  void load_dfcc_library();

  /// Transforms a goto function.
  ///
  /// The transformation distinguishes three types of functions:
  ///
  /// 1. spec-assigns functions: user-defined and return __CPROVER_assignable_t.
  /// 2. spec-frees functions: user-defined and return __CPROVER_freeable_t.
  /// 3. User defined or library functions that are not spec-* functions.
  ///    They get instrumented to make them checkable against a frame spec.
  ///
  /// Functions with assigns and frees contract clauses are used to generate
  /// spec-assigns and spec-frees functions.
  void transform_goto_function(const irep_idt &function_id);

  /// Transforms the proof harness function.
  void transform_harness_function(const irep_idt &function_id);

  /// Transforms a function that returns __CPROVER_assignable_t into a function
  /// - that accepts an extra __CPROVER_assignable_car_list_t ** parameter
  /// - populates the list with targets specified by the function
  void to_dfcc_spec_assigns_function(const irep_idt &function_id);

  /// Transforms the assigns clause of a contract into a function that
  /// - takes the same parameters as the function
  /// - accepts an extra __CPROVER_assignable_car_list_t ** parameter
  /// - populates the list with targets specified by the clause
  void dfcc_spec_assigns_function_from_contract(const irep_idt &function_id);

  /// Transforms a function that returns __CPROVER_freeable_t into a function
  /// - takes an extra __CPROVER_bool * parameter (unbounded object ID map)
  /// - populates the map with objects specified by the function
  void to_dfcc_spec_frees_function(const irep_idt &function_id);

  /// From the 'frees' clause of a contract, generates a function that
  /// - takes the same parameters as the function
  /// - takes a __CPROVER_bool * parameter (unbounded object ID map)
  /// - populates the map with objects specified by the clause
  void dfcc_spec_frees_function_from_contract(const irep_idt &function_id);

  /// Transforms a function into a function
  /// - that accepts extra parameters specifying assignable and freeable memory
  ///   locations
  /// - checks assignments, deallocations, etc. against specified locations
  /// - tracks heap deallocations and new heap allocations
  /// - tracks dirty stack allocated locations
  void to_dfcc_checked_function(const irep_idt &function_id);
};

/// Applies the dfcc transformation to the model
void dfcc(goto_modelt &goto_model, messaget &log, irep_idt &harness_function);

#endif