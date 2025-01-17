# Rewriting Calls to the __CPROVER_pointer_equals Predicate {#contracts-dev-spec-pointer-equals}

Back to top @ref contracts-dev-spec

@tableofcontents

In goto programs encoding pre or post conditions (generated from the contract
clauses) and in all user-defined functions, we simply replace calls to
`__CPROVER_pointer_equals` with calls to the library implementation.

```c
__CPROVER_contracts_pointer_equals(
    void **ptr1,
    void *ptr2,
    __CPROVER_contracts_write_set_ptr_t write_set);
```

This function implements the `__CPROVER_pointer_equals` behaviour in all
possible contexts (contract checking vs replacement, requires vs ensures clause
context, as described by the flags carried by the write set parameter).

---
 Prev | Next
:-----|:------
 @ref contracts-dev | @ref contracts-dev-spec-reminder