static int x = 0;

void foo() __CPROVER_assigns()
{
  int *y = &x;

  static int x = 0;

  // should pass (assigns local x)
  x++;

  // should fail (assigns global x)
  (*y)++;
}

int main()
{
  foo();
  return 0;
}
