#include <stdbool.h>
#include <stdlib.h>

// A function defining freeable pointers
void foo_frees(char *arr, const size_t size, const size_t new_size)
{
  __CPROVER_freeable(arr);
}

bool is_freeable(void *arr)
{
  bool is_dynamic_object = (arr == 0) | __CPROVER_DYNAMIC_OBJECT(arr);
  bool has_offset_zero = (arr == 0) | (__CPROVER_POINTER_OFFSET(arr) == 0);
  return is_dynamic_object & has_offset_zero;
}

// Function that resizes the array
char *foo(char *arr, const size_t size, const size_t new_size)
  // clang-format off
__CPROVER_requires(__CPROVER_is_freeable(arr))
__CPROVER_assigns(__CPROVER_object_whole(arr))
__CPROVER_frees(foo_frees(arr, size, new_size))
__CPROVER_ensures(
  (arr && new_size > size) ==>
    __CPROVER_is_fresh(__CPROVER_return_value, new_size))
__CPROVER_ensures(
  (arr && new_size > size) ==>
    __CPROVER_was_freed(__CPROVER_old(arr)))
__CPROVER_ensures(
    !(arr && new_size > size) ==>
      __CPROVER_return_value == __CPROVER_old(arr))
// clang-format on
{
  // The harness  allows to add a nondet offset to the pointer passed to foo.
  // Proving this assertion shows that the __CPROVER_is_freeable(arr) assumption
  // is in effect as expected for the verification
  __CPROVER_assert(is_freeable(arr), "arr is freeable");

  if(arr && new_size > size)
  {
    free(arr);
    arr = malloc(new_size);

    // write at some nondet i (should be always allowed since arr is fresh)
    size_t i;
    if(i < new_size)
      arr[i] = 0;

    return arr;
  }
  else
  {
    // write at some nondet i
    size_t i;
    if(i < size)
      arr[i] = 0;

    return arr;
  }
}

int main()
{
  size_t size;
  size_t new_size;
  __CPROVER_assume(size < __CPROVER_max_malloc_size);
  __CPROVER_assume(new_size < __CPROVER_max_malloc_size);
  char *arr = malloc(size);
  arr = foo(arr, size, new_size);
  return 0;
}
