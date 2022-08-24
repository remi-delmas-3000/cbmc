[CPROVER Manual TOC](../../)

# Frees Clause

### Syntax

```c
__CPROVER_frees(*pointer-typed-expression*, ...)
```

A _frees_ clause allows the user to specify a set of pointers that may be freed
by a function or the body of a loop.
A function or loop contract contract may have zero or more _frees_ clauses.

_Frees_ clause targets must be pointer-typed expressions.

_Frees_ clause targets can also be _conditional_ and written as follows:

```
condition: list-of-pointer-typed-expressions;
```

### Examples

In a function contract
```c
int foo(char *arr1, char *arr2, size_t size)
__CPROVER_frees(
    // arr1 freeable only if the condition holds
    size > 0 && arr1: arr1;
    // arr2 always freeable
    arr2
)
{
  if(size > 0 && arr1)
    free(arr1);
  free(arr2);
  return 0;
}
```

In a loop contract:

```c
int main()
{
  size_t size = 10;
  char *arr = malloc(size);

  for(size_t i = 0; i <= size; i++)
    // clang-format off
  __CPROVER_assigns(i, __CPROVER_POINTER_OBJECT(arr))
  __CPROVER_frees(arr)
    // clang-format on2
    {
      if(i < size)
        arr[i] = 0;
      else
        free(arr);
    }
  return 0;
}
```

### Semantics

The set of pointers specified by the frees clause of the contract is interpreted
at the function call-site for function contracts, and right before entering the
loop for loop contracts.

#### For contract checking
When checking a contract against a function or a loop, each pointer that the
function or loop body attempts to free gets checked for membership in the set of
pointers specified by the contract.

#### For replacement of function calls or loops by contracts
When replacing a function call or a loop by a contract, each pointer of the
_frees_ clause is non-deterministically freed after the function call
or after the loop.

# Using functions to specify parametric sets of freeable pointers

Users can define parametric sets of freeable pointers by writing functions that
return the built-in type `__CPROVER_freeable_t` and call the built-in function
`__CPROVER_freeable` or any user-defined function returning
`__CPROVER_freeable_t`:

```c
__CPROVER_freeable_t my_freeable_set(char **arr, size_t size)
{
  if (arr && size > 3) {
    __CPROVER_freeable(arr[0]);
    __CPROVER_freeable(arr[1]);
    __CPROVER_freeable(arr[2]);
  }
}
```

The built-in function:

```c
__CPROVER_freeable_t __CPROVER_freeable(void *ptr);
```
adds the given pointer to the freeable set described by the enclosing function.

Calls to functions returning `__CPROVER_freeable_t` can then be used as targets
in `__CPROVER_frees` clauses:

```c
void my_function(char **arr, size_t size)
__CPROVER_frees(my_freeable_set(arr, size))
{
  ...
}
```

# Frees clause related predicates

The predicate:

```c
__CPROVER_bool __CPROVER_is_freeable(void *ptr);
```
can only be used in pre and post conditions, in contract checking or replacement
modes. It returns `true` if and only if the pointer satisfies the preconditions
of the `free` function from `stdlib.h`, that is if and only if the pointer
points to a valid dynamically allocated object and has offset zero.

The predicate:

```c
__CPROVER_bool __CPROVER_is_freed(void *ptr);
```
can only be used in post conditions and returns `true` if and only if the
pointer was freed during the execution of the function or the loop under
analysis.