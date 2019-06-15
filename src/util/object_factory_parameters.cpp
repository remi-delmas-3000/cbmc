/*******************************************************************\

Module: Object Factory

Author: Diffblue Ltd

\*******************************************************************/

#include "object_factory_parameters.h"
#include "string2int.h"
#include "validate.h"

#include <regex>

#include <util/cmdline.h>
#include <util/options.h>

void object_factory_parameterst::set(const optionst &options)
{
  if(options.is_set("max-nondet-array-length"))
  {
    max_nondet_array_length =
      options.get_unsigned_int_option("max-nondet-array-length");
  }
  if(options.is_set("max-nondet-tree-depth"))
  {
    max_nondet_tree_depth =
      options.get_unsigned_int_option("max-nondet-tree-depth");
  }
  if(options.is_set("min-null-tree-depth"))
  {
    min_null_tree_depth =
      options.get_unsigned_int_option("min-null-tree-depth");
  }
  if(options.is_set("max-nondet-string-length"))
  {
    max_nondet_string_length =
      options.get_unsigned_int_option("max-nondet-string-length");
  }
  if(options.is_set("string-printable"))
  {
    string_printable = options.get_bool_option("string-printable");
  }
  if(options.is_set("string-input-value"))
  {
    string_input_values = options.get_list_option("string-input-value");
  }
  if(options.is_set("min-nondet-string-length"))
  {
    min_nondet_string_length =
      options.get_unsigned_int_option("min-nondet-string-length");
  }
  if(options.is_set("java-assume-inputs-interval"))
  {
    const auto &interval = options.get_option("java-assume-inputs-interval");
    const std::regex limits_regex("\\[(-\\d+|\\d*):(-\\d+|\\d*)\\]");
    std::smatch base_match;
    if(!std::regex_match(interval, base_match, limits_regex))
    {
      throw invalid_command_line_argument_exceptiont(
        "interval must be of the form [int:int], [int:] or [:int]",
        "--java-assume-inputs-interval");
    }
    assume_inputs_interval = [&]() -> integer_intervalt {
      integer_intervalt temp;
      if(base_match[1] != "")
        temp.make_ge_than(string2integer(base_match[1]));
      if(base_match[2] != "")
        temp.make_le_than(string2integer(base_match[2]));
      if(temp.is_top())
        throw invalid_command_line_argument_exceptiont(
          "at least one of the interval bounds must be given",
          "--java-assume-inputs-interval");
      if(temp.empty())
        throw invalid_command_line_argument_exceptiont(
          "interval is empty, lower limit cannot be bigger than upper limit",
          "--java-assume-inputs-interval");
      return temp;
    }();
  }
}

/// Parse the object factory parameters from a given command line
/// \param cmdline: Command line
/// \param [out] options: The options object that will be updated.
void parse_object_factory_options(const cmdlinet &cmdline, optionst &options)
{
  if(cmdline.isset("max-nondet-array-length"))
  {
    options.set_option(
      "max-nondet-array-length", cmdline.get_value("max-nondet-array-length"));
  }
  if(cmdline.isset("max-nondet-tree-depth"))
  {
    options.set_option(
      "max-nondet-tree-depth", cmdline.get_value("max-nondet-tree-depth"));
  }
  if(cmdline.isset("min-null-tree-depth"))
  {
    options.set_option(
      "min-null-tree-depth", cmdline.get_value("min-null-tree-depth"));
  }
  if(cmdline.isset("max-nondet-string-length"))
  {
    options.set_option(
      "max-nondet-string-length",
      cmdline.get_value("max-nondet-string-length"));
  }
  if(cmdline.isset("string-printable"))
  {
    options.set_option("string-printable", true);
  }
  if(cmdline.isset("string-non-empty"))
  {
    options.set_option("min-nondet-string-length", 1);
  }
  if(cmdline.isset("string-input-value"))
  {
    options.set_option(
      "string-input-value", cmdline.get_values("string-input-value"));
  }
  if(cmdline.isset("java-assume-inputs-interval"))
  {
    options.set_option(
      "java-assume-inputs-interval",
      cmdline.get_value("java-assume-inputs-interval"));
  }
}