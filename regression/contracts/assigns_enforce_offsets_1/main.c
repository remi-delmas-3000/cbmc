#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

int foo(int *x) __CPROVER_assigns(*(x+1))
{ 
  // should fail because we write before *(x+1)
  *x = 0;
  return 0;
}

int main()
{
  int x;
  foo(&x);
  return 0;
}
