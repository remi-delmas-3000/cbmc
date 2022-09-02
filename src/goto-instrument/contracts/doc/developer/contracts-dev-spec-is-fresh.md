# Rewriting Calls to __CPROVER_is_fresh and __CPROVER_is_freshr Predicates {#contracts-dev-spec-is-fresh}

Back to top @ref contracts-dev-spec

@tableofcontents

In goto programs encoding pre or post conditions (generated from the contract clauses) and in all user-defined functions, we simply replace calls to `__CPROVER_is_fresh` and `__CPROVER_is_fresh` with calls to the library implementation

```c
__CPROVER_contracts_is_freshr(void **ptr, size_t size, __CPROVER_contracts_write_set_ptr_t write_set);
```

This function implements the is_fresh behaviour in all possible contexts (contract checking vs replacement, requires vs ensures clause context), and receives the context information from the write set it receives as parameter. The context flags are set on the write set upon creation depending on the context.

---
 Prev | Next
:-----|:------
 @ref contracts-dev | @ref contracts-dev-spec-reminder