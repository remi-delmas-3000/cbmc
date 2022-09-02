# Program Transformation Overview {#contracts-dev-spec-transfo-params}

Back to top @ref contracts-dev-spec

@tableofcontents

The program transformation takes the following parameters:

```
goto-instrument --dfcc harness [--enforce-contract f_top/c_top] (--replace-with-contract f/c)* in.gb out.gb
```

Where:
- `--dfcc harness` specifies the identifier of the proof harness;
- `--enforce-contract f_top/c_top` specifies is optional and specifies that
  the function `f_top` must be checked against the contract carried by function
  `c_top`, by the pure contract `contract::c_top`. The contract `/c_top` can be
  omitted in which case it is assumed that `f_top` needs to be checked against
  its own contract `contract::f_top`;
- `--replace-call-with-contract f/c` specifies that function `f` must be replaced
  with the contract carried by function `c`, i.e. by the pure contract `contract::c`.
  The contract `/c` can be omitted in which case it is assumed that `f` needs to
  be replaced by its own contract `contract::f`;

The program transformation steps are applied as follows:
1. @ref contracts-dev-spec-codegen is applied to all contracts to check or replace;
2. @ref contracts-dev-spec-dfcc is applied to all library or user-defined goto functions;
3. @ref contracts-dev-spec-harness is applied to the harness function;
4. @ref contracts-dev-spec-contract-checking is applied to the function to be checked against a contract;
5. @ref contracts-dev-spec-contract-replacement is applied to each function to be replaced by a contract;



---
 Prev | Next
:-----|:------
 @ref contracts-dev-spec-reminder | @ref contracts-dev-spec-codegen
