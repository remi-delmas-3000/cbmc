# Command Line Interface for Code Contracts {#contracts-user-cli}

The program transformation takes the following parameters:

```
goto-instrument --dfcc harness [--enforce-contract f_top/c_top] (--replace-with-contract f/c)* [--apply-loop-contracts] in.gb out.gb
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
- `--apply-loop-contracts` is optional and specifies to apply loop contract checking and abstraction;