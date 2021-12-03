#include <stdlib.h>

int MAX_SIZE = __CPROVER_max_malloc_size;
int nondet_int();

// returns the index at which the write was performed if any
// -1 otherwise
int foo(char *a, int size)
  // clang-format off
__CPROVER_requires(0 <= size && size <= MAX_SIZE)
__CPROVER_requires(a == NULL || __CPROVER_rw_ok(a, size))
__CPROVER_assigns(__CPROVER_POINTER_OBJECT(a))
__CPROVER_ensures(
    a && __CPROVER_return_value >= 0 ==> a[__CPROVER_return_value] == 0)
// clang-format on
{
  if(!a)
    return -1;
  int i = nondet_int();
  if(0 <= i && i < size)
  {
    a[i] = 0;
    return i;
  }
  return -1;
}

int main()
{
  int size = nondet_int();
  if(size < 0)
    size = 0;
  if(size > MAX_SIZE)
    size = MAX_SIZE;
  char *a = malloc(size * sizeof(*a));
  int res = foo(a, size);
  if(res >= 0)
    __CPROVER_assert(a[res] == 0, "expecting SUCCESS");
  return 0;
}
