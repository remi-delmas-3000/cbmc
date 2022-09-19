#include <stdbool.h>

int foo(bool a, char *x, char *y)
  // clang-format off
__CPROVER_assigns(
   a : __CPROVER_whole_object(x), __CPROVER_whole_object(y)
)
// clang-format on
{
  if(a)
  {
    x[0] = 0; // must pass
    y[0] = 0; // must pass
  }
  else
  {
    x[0] = 0; // must fail
    y[0] = 0; // must fail
  }

  x[0] = 0; // must fail
  y[0] = 0; // must fail
  return 0;
}

int main()
{
  bool a;
  char x[2];
  char y[2];

  foo(a, &x, &y);
  return 0;
}
