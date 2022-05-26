/* FUNCTION: __CPROVER_assignable_car_create */

#ifndef __CPROVER_assignable_car_t_defined
#  define __CPROVER_assignable_car_t_defined
// a conditional address range
typedef struct __CPROVER_assignable_car_t
{
  __CPROVER_bool is_writable;
  __CPROVER_bool is_whole_object;
  __CPROVER_bool is_pointer_to_pointer;
  __CPROVER_size_t size;
  __CPROVER_size_t object_id;
  void *lb;
  void *ub;
} __CPROVER_assignable_car_t;
#endif

// Creates a conditional address range from a pointer and a size.
// Checks that the pointer is either NULL or
// points to a valid writable region of at least 'size' bytes.
__CPROVER_assignable_car_t __CPROVER_assignable_car_create(
  // Base address of the target location
  void *ptr,
  // Size of the target location in bytes
  __CPROVER_size_t size)
{
#pragma CPROVER check push
#pragma CPROVER check disable "pointer"
#pragma CPROVER check disable "pointer-primitive"
#pragma CPROVER check disable "unsigned-overflow"
#pragma CPROVER check disable "signed-overflow"
#pragma CPROVER check disable "undefined-shift"
#pragma CPROVER check disable "conversion"
  __CPROVER_assert(
    (!ptr || __CPROVER_rw_ok(ptr, size)),
    "car_create: ptr is NULL or writable up to size");
  __CPROVER_assignable_car_t car;
  car.is_writable = ptr;
  car.is_whole_object = 0;
  car.is_pointer_to_pointer = 0;
  car.size = size;
  car.object_id = __CPROVER_POINTER_OBJECT(ptr);
  car.lb = ptr;
  car.ub = (void *)((char *)ptr + size);
  __CPROVER_assert(
    __CPROVER_same_object(car.lb, car.ub),
    "car_create: no overflow on CAR upper bound");
  return car;
#pragma CPROVER check pop
}

/* FUNCTION: __CPROVER_assignable_car_list_empty */

#ifndef __CPROVER_assignable_car_t_defined
#  define __CPROVER_assignable_car_t_defined
// a conditional address range
typedef struct __CPROVER_assignable_car_t
{
  __CPROVER_bool is_writable;
  __CPROVER_bool is_whole_object;
  __CPROVER_bool is_pointer_to_pointer;
  __CPROVER_size_t size;
  __CPROVER_size_t object_id;
  void *lb;
  void *ub;
} __CPROVER_assignable_car_t;
#endif

#ifndef __CPROVER_assignable_car_list_t_defined
#  define __CPROVER_assignable_car_list_t_defined
// a list of conditional address ranges
typedef struct __CPROVER_assignable_car_list_t
{
  struct __CPROVER_assignable_car_list_t *next;
  __CPROVER_assignable_car_t car;
} __CPROVER_assignable_car_list_t;
#endif

// Initialises list to the empty list.
void __CPROVER_assignable_car_list_empty(__CPROVER_assignable_car_list_t **list)
{
  __CPROVER_assert(list, "list is not NULL");
  *list = 0;
}

/* FUNCTION: __CPROVER_assignable_car_list_add */

#ifndef __CPROVER_assignable_car_t_defined
#  define __CPROVER_assignable_car_t_defined
// a conditional address range
typedef struct __CPROVER_assignable_car_t
{
  __CPROVER_bool is_writable;
  __CPROVER_bool is_whole_object;
  __CPROVER_bool is_pointer_to_pointer;
  __CPROVER_size_t size;
  __CPROVER_size_t object_id;
  void *lb;
  void *ub;
} __CPROVER_assignable_car_t;
#endif

#ifndef __CPROVER_assignable_car_list_t_defined
#  define __CPROVER_assignable_car_list_t_defined
// a list of conditional address ranges
typedef struct __CPROVER_assignable_car_list_t
{
  struct __CPROVER_assignable_car_list_t *next;
  __CPROVER_assignable_car_t car;
} __CPROVER_assignable_car_list_t;
#endif

__CPROVER_assignable_car_t
__CPROVER_assignable_car_create(void *, __CPROVER_size_t);

// Adds a sized target to the list if the target is writable, no-op otherwise.
// The is_whole_object and is_pointer_to_pointer define how this CAR will be 
// havoced.
void __CPROVER_assignable_car_list_add(
  // List of CARs to extend
  __CPROVER_assignable_car_list_t **list,

  // Base address of the target location
  void *ptr,

  // Size of the target location in bytes
  __CPROVER_size_t size,

  // True iff the object pointed to by ptr contains a pointer
  __CPROVER_bool is_pointer_to_pointer)
{
  __CPROVER_assert(list, "list is not NULL");
  __CPROVER_assignable_car_t car = __CPROVER_assignable_car_create(ptr, size);
  if(car.is_writable)
  {
    struct __CPROVER_assignable_car_list_t *result =
      __CPROVER_allocate(sizeof(*result), 1);
    car.is_whole_object =
      __CPROVER_POINTER_OFFSET(ptr) == 0 && size == __CPROVER_OBJECT_SIZE(ptr);
    car.is_pointer_to_pointer = is_pointer_to_pointer;
    result->car = car;
    result->next = *list;
    *list = result;
  }
}

/* FUNCTION: __CPROVER_assignable_car_list_invalidate_same_object */

#ifndef __CPROVER_assignable_car_t_defined
#  define __CPROVER_assignable_car_t_defined
// a conditional address range
typedef struct __CPROVER_assignable_car_t
{
  __CPROVER_bool is_writable;
  __CPROVER_bool is_whole_object;
  __CPROVER_bool is_pointer_to_pointer;
  __CPROVER_size_t size;
  __CPROVER_size_t object_id;
  void *lb;
  void *ub;
} __CPROVER_assignable_car_t;
#endif

#ifndef __CPROVER_assignable_car_list_t_defined
#  define __CPROVER_assignable_car_list_t_defined
// a list of conditional address ranges
typedef struct __CPROVER_assignable_car_list_t
{
  struct __CPROVER_assignable_car_list_t *next;
  __CPROVER_assignable_car_t car;
} __CPROVER_assignable_car_list_t;
#endif

void __CPROVER_assignable_car_list_invalidate_same_object(
  __CPROVER_assignable_car_list_t *list,
  __CPROVER_size_t object_id)
{
  while(list)
  {
    if(list->car.is_writable && (object_id == list->car.object_id))
      list->car.is_writable = 0;
    list = list->next;
  }
}

/* FUNCTION: __CPROVER_assignable_car_contains_car */

#ifndef __CPROVER_assignable_car_t_defined
#  define __CPROVER_assignable_car_t_defined
// a conditional address range
typedef struct __CPROVER_assignable_car_t
{
  __CPROVER_bool is_writable;
  __CPROVER_bool is_whole_object;
  __CPROVER_bool is_pointer_to_pointer;
  __CPROVER_size_t size;
  __CPROVER_size_t object_id;
  void *lb;
  void *ub;
} __CPROVER_assignable_car_t;
#endif

// True iff 'car' and 'candidate' are writable and 'car' contains 'candidate'.
__CPROVER_bool __CPROVER_assignable_car_contains_car(
  __CPROVER_assignable_car_t car,
  __CPROVER_assignable_car_t candidate)
{
  __CPROVER_bool incl =
    car.is_writable & candidate.is_writable &
    __CPROVER_same_object(car.lb, candidate.lb) &
    __CPROVER_POINTER_OFFSET(car.lb) <= __CPROVER_POINTER_OFFSET(candidate.lb) &
    __CPROVER_POINTER_OFFSET(candidate.ub) <= __CPROVER_POINTER_OFFSET(car.ub);
  return incl;
}

/* FUNCTION: __CPROVER_assignable_car_list_contains_car */

#ifndef __CPROVER_assignable_car_t_defined
#  define __CPROVER_assignable_car_t_defined
// a conditional address range
typedef struct __CPROVER_assignable_car_t
{
  __CPROVER_bool is_writable;
  __CPROVER_bool is_whole_object;
  __CPROVER_bool is_pointer_to_pointer;
  __CPROVER_size_t size;
  __CPROVER_size_t object_id;
  void *lb;
  void *ub;
} __CPROVER_assignable_car_t;
#endif

#ifndef __CPROVER_assignable_car_list_t_defined
#  define __CPROVER_assignable_car_list_t_defined
// a list of conditional address ranges
typedef struct __CPROVER_assignable_car_list_t
{
  struct __CPROVER_assignable_car_list_t *next;
  __CPROVER_assignable_car_t car;
} __CPROVER_assignable_car_list_t;
#endif

__CPROVER_bool __CPROVER_assignable_car_contains_car(
  __CPROVER_assignable_car_t,
  __CPROVER_assignable_car_t);

// True iff a car_t of the given 'list' contains the given 'candidate'.
__CPROVER_bool __CPROVER_assignable_car_list_contains_car(
  // List to check inclusion against
  __CPROVER_assignable_car_list_t *list,
  // Candidate CAR
  __CPROVER_assignable_car_t candidate)
{
  __CPROVER_bool incl = 0;
  while(list)
  {
    incl |= __CPROVER_assignable_car_contains_car(list->car, candidate);
    list = list->next;
  }
  return incl;
}

/* FUNCTION: __CPROVER_assignable_car_list_havoc */

#ifndef __CPROVER_assignable_car_t_defined
#  define __CPROVER_assignable_car_t_defined
// a conditional address range
typedef struct __CPROVER_assignable_car_t
{
  __CPROVER_bool is_writable;
  __CPROVER_bool is_whole_object;
  __CPROVER_bool is_pointer_to_pointer;
  __CPROVER_size_t size;
  __CPROVER_size_t object_id;
  void *lb;
  void *ub;
} __CPROVER_assignable_car_t;
#endif

#ifndef __CPROVER_assignable_car_list_t_defined
#  define __CPROVER_assignable_car_list_t_defined
// a list of conditional address ranges
typedef struct __CPROVER_assignable_car_list_t
{
  struct __CPROVER_assignable_car_list_t *next;
  __CPROVER_assignable_car_t car;
} __CPROVER_assignable_car_list_t;
#endif

char *__CPROVER_assignable_nondet_ptr();

// Havocs all currently writable cars of the given list.
void __CPROVER_assignable_car_list_havoc(__CPROVER_assignable_car_list_t *list)
{
  while(list)
  {
    if(list->car.is_writable)
    {
      if(list->car.is_whole_object && list->car.is_pointer_to_pointer)
      {
        *((char **)list->car.lb) = __CPROVER_assignable_nondet_ptr();
      }
      else if(list->car.is_whole_object)
      {
        __CPROVER_havoc_object(list->car.lb);
      }
      else
      {
        __CPROVER_havoc_slice(list->car.lb, list->car.size);
      }
    }
    list = list->next;
  }
}

/* FUNCTION: __CPROVER_assignable_obj_list_empty */

#ifndef __CPROVER_assignable_obj_list_t_defined
#  define __CPROVER_assignable_obj_list_t_defined
// a list of object IDs
typedef struct __CPROVER_assignable_obj_list_t
{
  struct __CPROVER_assignable_obj_list_t *next;
  __CPROVER_size_t object_id;
} __CPROVER_assignable_obj_list_t;
#endif

// Initialises 'list' to the empty list.
void __CPROVER_assignable_obj_list_empty(__CPROVER_assignable_obj_list_t **list)
{
  __CPROVER_assert(list, "list is not NULL");
  *list = 0;
}

/* FUNCTION: __CPROVER_assignable_obj_list_add */

#ifndef __CPROVER_assignable_obj_list_t_defined
#  define __CPROVER_assignable_obj_list_t_defined
// a list of object IDs
typedef struct __CPROVER_assignable_obj_list_t
{
  struct __CPROVER_assignable_obj_list_t *next;
  __CPROVER_size_t object_id;
} __CPROVER_assignable_obj_list_t;
#endif

// Adds the object pointed to by 'ptr' to the list
// if 'ptr' is valid and not NULL.
void __CPROVER_assignable_obj_list_add(
  __CPROVER_assignable_obj_list_t **list,
  void *ptr)
{
  __CPROVER_assert(list, "list is not NULL");
  __CPROVER_ssize_t offset = __CPROVER_POINTER_OFFSET(ptr);
  __CPROVER_ssize_t ssize = (__CPROVER_ssize_t)__CPROVER_OBJECT_SIZE(ptr);
  __CPROVER_assert(
    !ptr || (0 <= offset && offset <= ssize &&
             __CPROVER_rw_ok(ptr, (__CPROVER_size_t)(ssize - offset))),
    "obj_list_add: pointer must be NULL or pointing into some valid object");

  if(ptr)
  {
    struct __CPROVER_assignable_obj_list_t *result =
      __CPROVER_allocate(sizeof(*result), 1);
    result->object_id = __CPROVER_POINTER_OBJECT(ptr);
    result->next = *list;
    *list = result;
  }
}

/* FUNCTION: __CPROVER_assignable_obj_map_empty */

// Initialises 'map' to the empty map.
void __CPROVER_assignable_obj_map_empty(__CPROVER_bool *dest)
{
  __CPROVER_assert(dest, "dest is not NULL");
  __CPROVER_array_set(dest, 0);
}

/* FUNCTION: __CPROVER_assignable_obj_map_copy */

// Copies the full contents of 'src' to 'dest'.
void __CPROVER_assignable_obj_map_copy(
  __CPROVER_bool *dest,
  __CPROVER_bool *src)
{
  __CPROVER_assert(dest, "dest is not NULL");
  __CPROVER_assert(src, "src is not NULL");
  __CPROVER_array_copy(dest, src);
}

/* FUNCTION: __CPROVER_assignable_obj_map_add */

// Adds the object pointed to by 'ptr' to 'map' if 'ptr' is not NULL.
// 'ptr' must be NULL or valid.
void __CPROVER_assignable_obj_map_add(__CPROVER_bool *map, void *ptr)
{
  __CPROVER_assert(map, "map is not null");
  __CPROVER_ssize_t offset = __CPROVER_POINTER_OFFSET(ptr);
  __CPROVER_ssize_t ssize = (__CPROVER_ssize_t)__CPROVER_OBJECT_SIZE(ptr);
  __CPROVER_assert(
    !ptr || (0 <= offset && offset <= ssize &&
             __CPROVER_rw_ok(ptr, (__CPROVER_size_t)(ssize - offset))),
    "obj_map_add: pointer must be NULL or pointing into some valid object");
  if(ptr)
  {
    map[__CPROVER_POINTER_OBJECT(ptr)] = 1;
  }
}

/* FUNCTION: __CPROVER_assignable_obj_map_remove */

// Removes the object pointed to by 'ptr' from 'map' if 'ptr' is not NULL.
// 'ptr' must be NULL or valid.
void __CPROVER_assignable_obj_map_remove(__CPROVER_bool *map, void *ptr)
{
  __CPROVER_assert(map, "map is not NULL");
  __CPROVER_ssize_t offset = __CPROVER_POINTER_OFFSET(ptr);
  __CPROVER_ssize_t ssize = (__CPROVER_ssize_t)__CPROVER_OBJECT_SIZE(ptr);
  __CPROVER_assert(
    !ptr || (0 <= offset && offset <= ssize &&
             __CPROVER_rw_ok(ptr, (__CPROVER_size_t)(ssize - offset))),
    "obj_map_remove: pointer must be NULL or pointing into some valid object");
  if(ptr)
  {
    map[__CPROVER_POINTER_OBJECT(ptr)] = 0;
  }
}

/* FUNCTION: __CPROVER_assignable_check_assignment */
#ifndef __CPROVER_assignable_car_t_defined
#  define __CPROVER_assignable_car_t_defined
// a conditional address range
typedef struct __CPROVER_assignable_car_t
{
  __CPROVER_bool is_writable;
  __CPROVER_bool is_whole_object;
  __CPROVER_bool is_pointer_to_pointer;
  __CPROVER_size_t size;
  __CPROVER_size_t object_id;
  void *lb;
  void *ub;
} __CPROVER_assignable_car_t;
#endif

#ifndef __CPROVER_assignable_car_list_t_defined
#  define __CPROVER_assignable_car_list_t_defined
// a list of conditional address ranges
typedef struct __CPROVER_assignable_car_list_t
{
  struct __CPROVER_assignable_car_list_t *next;
  __CPROVER_assignable_car_t car;
} __CPROVER_assignable_car_list_t;
#endif

__CPROVER_assignable_car_t
__CPROVER_assignable_car_create(void *, __CPROVER_size_t);

__CPROVER_bool __CPROVER_assignable_car_list_contains_car(
  __CPROVER_assignable_car_list_t *,
  __CPROVER_assignable_car_t);

// Checks if the target of an assignment is included in
// contract_assigns union stack_allocated union heap_allocated.
__CPROVER_bool __CPROVER_assignable_check_assignment(

  // Start address of the assigned location
  void *ptr,

  // Size of the assigned location in bytes
  __CPROVER_size_t size,

  // Assignable CARs according to the top-level contract
  __CPROVER_assignable_car_list_t *contract_assigns,

  // Currently stack allocated objects
  __CPROVER_bool *stack_allocated,

  // Currently heap-allocated objects
  __CPROVER_bool *heap_allocated)
{
  __CPROVER_assert(stack_allocated, "stack_allocated is not NULL");
  __CPROVER_assert(heap_allocated, "heap_allocated is not NULL");
  __CPROVER_assignable_car_t car = __CPROVER_assignable_car_create(ptr, size);
  if(!car.is_writable)
    return 0;
  return stack_allocated[car.object_id] || heap_allocated[car.object_id] ||
         __CPROVER_assignable_car_list_contains_car(contract_assigns, car);
}

/* FUNCTION: __CPROVER_assignable_check_array_set */

#ifndef __CPROVER_assignable_car_t_defined
#  define __CPROVER_assignable_car_t_defined
// a conditional address range
typedef struct __CPROVER_assignable_car_t
{
  __CPROVER_bool is_writable;
  __CPROVER_bool is_whole_object;
  __CPROVER_bool is_pointer_to_pointer;
  __CPROVER_size_t size;
  __CPROVER_size_t object_id;
  void *lb;
  void *ub;
} __CPROVER_assignable_car_t;
#endif

#ifndef __CPROVER_assignable_car_list_t_defined
#  define __CPROVER_assignable_car_list_t_defined
// a list of conditional address ranges
typedef struct __CPROVER_assignable_car_list_t
{
  struct __CPROVER_assignable_car_list_t *next;
  __CPROVER_assignable_car_t car;
} __CPROVER_assignable_car_list_t;
#endif

__CPROVER_assignable_car_t
__CPROVER_assignable_car_create(void *, __CPROVER_size_t);
__CPROVER_bool __CPROVER_assignable_car_list_contains_car(
  __CPROVER_assignable_car_list_t *,
  __CPROVER_assignable_car_t);

// Checks if the destination of an array_set operation
// is included in contract_assigns union stack_allocated union heap_allocated.
__CPROVER_bool __CPROVER_assignable_check_array_set(
  // The destination array
  void *dest,

  // Assignable CARs according to the top-level contract
  __CPROVER_assignable_car_list_t *contract_assigns,

  // Currently stack allocated objects
  __CPROVER_bool *stack_allocated,

  // Currently heap allocated objects
  __CPROVER_bool *heap_allocated)
{
  __CPROVER_assert(stack_allocated, "stack_allocated is not NULL");
  __CPROVER_assert(heap_allocated, "heap_allocated is not NULL");
  __CPROVER_size_t size =
    __CPROVER_OBJECT_SIZE(dest) - __CPROVER_POINTER_OFFSET(dest);
  __CPROVER_assignable_car_t car = __CPROVER_assignable_car_create(dest, size);
  if(!car.is_writable)
    return 0;
  return stack_allocated[car.object_id] || heap_allocated[car.object_id] ||
         __CPROVER_assignable_car_list_contains_car(contract_assigns, car);
}

/* FUNCTION: __CPROVER_assignable_check_array_copy */

#ifndef __CPROVER_assignable_car_t_defined
#  define __CPROVER_assignable_car_t_defined
// a conditional address range
typedef struct __CPROVER_assignable_car_t
{
  __CPROVER_bool is_writable;
  __CPROVER_bool is_whole_object;
  __CPROVER_bool is_pointer_to_pointer;
  __CPROVER_size_t size;
  __CPROVER_size_t object_id;
  void *lb;
  void *ub;
} __CPROVER_assignable_car_t;
#endif

#ifndef __CPROVER_assignable_car_list_t_defined
#  define __CPROVER_assignable_car_list_t_defined
// a list of conditional address ranges
typedef struct __CPROVER_assignable_car_list_t
{
  struct __CPROVER_assignable_car_list_t *next;
  __CPROVER_assignable_car_t car;
} __CPROVER_assignable_car_list_t;
#endif

__CPROVER_assignable_car_t
__CPROVER_assignable_car_create(void *, __CPROVER_size_t);

__CPROVER_bool __CPROVER_assignable_car_list_contains_car(
  __CPROVER_assignable_car_list_t *,
  __CPROVER_assignable_car_t);

// Checks if the destination of an array_copy operation
// is included in contract_assigns union stack_allocated union heap_allocated.
// array_copy overwrites all of *dest (possibly using nondet values), even
// if *src is smaller.
__CPROVER_bool __CPROVER_assignable_check_array_copy(
  // Destination pointer
  void *dest,

  // Assignable CARs according to the top-level contract
  __CPROVER_assignable_car_list_t *contract_assigns,

  // Currently stack allocated objects
  __CPROVER_bool *stack_allocated,

  // Currently heap allocated objects
  __CPROVER_bool *heap_allocated)
{
  __CPROVER_assert(stack_allocated, "stack_allocated is not NULL");
  __CPROVER_assert(heap_allocated, "heap_allocated is not NULL");
  __CPROVER_size_t size =
    __CPROVER_OBJECT_SIZE(dest) - __CPROVER_POINTER_OFFSET(dest);
  __CPROVER_assignable_car_t car = __CPROVER_assignable_car_create(dest, size);
  if(!car.is_writable)
    return 0;
  return stack_allocated[__CPROVER_POINTER_OBJECT(dest)] ||
         heap_allocated[__CPROVER_POINTER_OBJECT(dest)] ||
         __CPROVER_assignable_car_list_contains_car(contract_assigns, car);
}

/* FUNCTION: __CPROVER_assignable_check_array_replace */

#ifndef __CPROVER_assignable_car_t_defined
#  define __CPROVER_assignable_car_t_defined
// a conditional address range
typedef struct __CPROVER_assignable_car_t
{
  __CPROVER_bool is_writable;
  __CPROVER_bool is_whole_object;
  __CPROVER_bool is_pointer_to_pointer;
  __CPROVER_size_t size;
  __CPROVER_size_t object_id;
  void *lb;
  void *ub;
} __CPROVER_assignable_car_t;
#endif

#ifndef __CPROVER_assignable_car_list_t_defined
#  define __CPROVER_assignable_car_list_t_defined
// a list of conditional address ranges
typedef struct __CPROVER_assignable_car_list_t
{
  struct __CPROVER_assignable_car_list_t *next;
  __CPROVER_assignable_car_t car;
} __CPROVER_assignable_car_list_t;
#endif

__CPROVER_assignable_car_t
__CPROVER_assignable_car_create(void *, __CPROVER_size_t);

__CPROVER_bool __CPROVER_assignable_car_list_contains_car(
  __CPROVER_assignable_car_list_t *,
  __CPROVER_assignable_car_t);

// Checks if the destination of an array_replace operation
// is included in contract_assigns union stack_allocated union heap_allocated.
// array_replace replaces at most size-of-*src bytes in *dest
__CPROVER_bool __CPROVER_assignable_check_array_replace(
  // Destination pointer
  void *dest,

  // Source pointer
  void *src,

  // Assignable CARs according to the top-level contract
  __CPROVER_assignable_car_list_t *contract_assigns,

  // Currently stack allocated objects
  __CPROVER_bool *stack_allocated,

  // Currently heap allocated objects
  __CPROVER_bool *heap_allocated)
{
  __CPROVER_assert(stack_allocated, "stack_allocated is not NULL");
  __CPROVER_assert(heap_allocated, "heap_allocated is not NULL");
  __CPROVER_size_t src_size =
    __CPROVER_OBJECT_SIZE(src) - __CPROVER_POINTER_OFFSET(src);
  __CPROVER_size_t dest_size =
    __CPROVER_OBJECT_SIZE(dest) - __CPROVER_POINTER_OFFSET(dest);
  __CPROVER_size_t size = dest_size < src_size ? dest_size : src_size;
  __CPROVER_assignable_car_t car = __CPROVER_assignable_car_create(dest, size);
  if(!car.is_writable)
    return 0;
  return stack_allocated[__CPROVER_POINTER_OBJECT(dest)] ||
         heap_allocated[__CPROVER_POINTER_OBJECT(dest)] ||
         __CPROVER_assignable_car_list_contains_car(contract_assigns, car);
}

/* FUNCTION: __CPROVER_assignable_check_havoc_object */

#ifndef __CPROVER_assignable_car_t_defined
#  define __CPROVER_assignable_car_t_defined
// a conditional address range
typedef struct __CPROVER_assignable_car_t
{
  __CPROVER_bool is_writable;
  __CPROVER_bool is_whole_object;
  __CPROVER_bool is_pointer_to_pointer;
  __CPROVER_size_t size;
  __CPROVER_size_t object_id;
  void *lb;
  void *ub;
} __CPROVER_assignable_car_t;
#endif

#ifndef __CPROVER_assignable_car_list_t_defined
#  define __CPROVER_assignable_car_list_t_defined
// a list of conditional address ranges
typedef struct __CPROVER_assignable_car_list_t
{
  struct __CPROVER_assignable_car_list_t *next;
  __CPROVER_assignable_car_t car;
} __CPROVER_assignable_car_list_t;
#endif

__CPROVER_assignable_car_t
__CPROVER_assignable_car_create(void *, __CPROVER_size_t);

__CPROVER_bool __CPROVER_assignable_car_list_contains_car(
  __CPROVER_assignable_car_list_t *,
  __CPROVER_assignable_car_t);

// Checks if the target of a havoc_object operation
// is included in contract_assigns union stack_allocated union heap_allocated.
__CPROVER_bool __CPROVER_assignable_check_havoc_object(

  // pointer to the object being havoced
  void *ptr,

  // Assignable CARs according to the top-level contract
  __CPROVER_assignable_car_list_t *contract_assigns,

  // Currently stack allocated objects
  __CPROVER_bool *stack_allocated,

  // Currently heap allocated objects
  __CPROVER_bool *heap_allocated)
{
  __CPROVER_assert(stack_allocated, "stack_allocated is not NULL");
  __CPROVER_assert(heap_allocated, "heap_allocated is not NULL");
  ptr = (char *)ptr - __CPROVER_POINTER_OFFSET(ptr);
  __CPROVER_size_t size = __CPROVER_OBJECT_SIZE(ptr);
  __CPROVER_assignable_car_t car = __CPROVER_assignable_car_create(ptr, size);
  if(!car.is_writable)
    return 0;
  return stack_allocated[car.object_id] || heap_allocated[car.object_id] ||
         __CPROVER_assignable_car_list_contains_car(contract_assigns, car);
}

/* FUNCTION: __CPROVER_assignable_check_deallocate */

#ifndef __CPROVER_assignable_car_t_defined
#  define __CPROVER_assignable_car_t_defined
// a conditional address range
typedef struct __CPROVER_assignable_car_t
{
  __CPROVER_bool is_writable;
  __CPROVER_bool is_whole_object;
  __CPROVER_bool is_pointer_to_pointer;
  __CPROVER_size_t size;
  __CPROVER_size_t object_id;
  void *lb;
  void *ub;
} __CPROVER_assignable_car_t;
#endif

#ifndef __CPROVER_assignable_car_list_t_defined
#  define __CPROVER_assignable_car_list_t_defined
// a list of conditional address ranges
typedef struct __CPROVER_assignable_car_list_t
{
  struct __CPROVER_assignable_car_list_t *next;
  __CPROVER_assignable_car_t car;
} __CPROVER_assignable_car_list_t;
#endif

// Checks if the target pointer of a deallocate operation is
// effectively freeable and included in contract_frees union heap_allocated.
__CPROVER_bool __CPROVER_assignable_check_deallocate(

  // Pointer that is going to be deallocated
  void *ptr,

  // Freeable objects according to the top level frees clause
  __CPROVER_bool *contract_frees,

  // Currently heap allocated objects
  __CPROVER_bool *heap_allocated)
{
  __CPROVER_assert(contract_frees, "contract_frees is not NULL");
  __CPROVER_assert(heap_allocated, "heap_allocated is not NULL");
  __CPROVER_assert(
    !ptr ||
      (__CPROVER_DYNAMIC_OBJECT(ptr) && __CPROVER_POINTER_OFFSET(ptr) == 0 &&
       __CPROVER_rw_ok(ptr, __CPROVER_OBJECT_SIZE(ptr))),
    "check_deallocate: check that the pointer is either NULL or is a valid "
    "pointer to a dynamic object and has a zero offset");

  // NULL pointers can always be deallocated
  if(!ptr)
    return 1;

  __CPROVER_size_t object_id = __CPROVER_POINTER_OBJECT(ptr);
  return contract_frees[object_id] || heap_allocated[object_id];
}

/* FUNCTION: __CPROVER_assignable_check_assigns_clause_inclusion */

#ifndef __CPROVER_assignable_car_t_defined
#  define __CPROVER_assignable_car_t_defined
// a conditional address range
typedef struct __CPROVER_assignable_car_t
{
  __CPROVER_bool is_writable;
  __CPROVER_bool is_whole_object;
  __CPROVER_bool is_pointer_to_pointer;
  __CPROVER_size_t size;
  __CPROVER_size_t object_id;
  void *lb;
  void *ub;
} __CPROVER_assignable_car_t;
#endif

#ifndef __CPROVER_assignable_car_list_t_defined
#  define __CPROVER_assignable_car_list_t_defined
// a list of conditional address ranges
typedef struct __CPROVER_assignable_car_list_t
{
  struct __CPROVER_assignable_car_list_t *next;
  __CPROVER_assignable_car_t car;
} __CPROVER_assignable_car_list_t;
#endif

__CPROVER_bool __CPROVER_assignable_car_list_contains_car(
  __CPROVER_assignable_car_list_t *,
  __CPROVER_assignable_car_t);

// Checks if all candidates are writable and
// included in contract_assigns union stack_allocated union heap_allocated.
__CPROVER_bool __CPROVER_assignable_check_assigns_clause_inclusion(
  // Candidate conditional address ranges
  __CPROVER_assignable_car_list_t *candidates,

  // Assignable address ranges from the top level contract
  __CPROVER_assignable_car_list_t *contract_assigns,

  // Currently stack allocated objects
  __CPROVER_bool *stack_allocated,

  // Currently heap allocated objects
  __CPROVER_bool *heap_allocated)
{
  __CPROVER_assert(stack_allocated, "stack_allocated is not NULL");
  __CPROVER_assert(heap_allocated, "heap_allocated is not NULL");
  __CPROVER_bool incl = 1;

  while(candidates)
  {
    __CPROVER_assignable_car_t car = candidates->car;
    incl &= car.is_writable &&
            (stack_allocated[car.object_id] || heap_allocated[car.object_id] ||
             __CPROVER_assignable_car_list_contains_car(contract_assigns, car));
    candidates = candidates->next;
  }
  return incl;
}

/* FUNCTION: __CPROVER_assignable_check_frees_clause_inclusion */

#ifndef __CPROVER_assignable_obj_list_t_defined
#  define __CPROVER_assignable_obj_list_t_defined
// a list of object IDs
typedef struct __CPROVER_assignable_obj_list_t
{
  struct __CPROVER_assignable_obj_list_t *next;
  __CPROVER_size_t object_id;
} __CPROVER_assignable_obj_list_t;
#endif

// Checks if all candidate objects of a frees clause are included in
// contract_frees union heap_allocated.
__CPROVER_bool __CPROVER_assignable_check_frees_clause_inclusion(
  // Candidate objects from a frees clause
  __CPROVER_assignable_obj_list_t *candidates,

  // Objects from the top level frees clause
  __CPROVER_bool *contract_frees,

  // Currently heap allocated objects
  __CPROVER_bool *heap_allocated)
{
  __CPROVER_assert(contract_frees, "contract_frees is not NULL");
  __CPROVER_assert(heap_allocated, "heap_allocated is not NULL");
  __CPROVER_bool all_incl = 1;
  while(candidates)
  {
    __CPROVER_size_t object_id = candidates->object_id;
    all_incl &= contract_frees[object_id] || heap_allocated[object_id];
    candidates = candidates->next;
  }
  return all_incl;
}

/* FUNCTION: __CPROVER_assignable_obj_list_deallocate */

#ifndef __CPROVER_assignable_car_t_defined
#  define __CPROVER_assignable_car_t_defined
// a conditional address range
typedef struct __CPROVER_assignable_car_t
{
  __CPROVER_bool is_writable;
  __CPROVER_bool is_whole_object;
  __CPROVER_bool is_pointer_to_pointer;
  __CPROVER_size_t size;
  __CPROVER_size_t object_id;
  void *lb;
  void *ub;
} __CPROVER_assignable_car_t;
#endif

#ifndef __CPROVER_assignable_car_list_t_defined
#  define __CPROVER_assignable_car_list_t_defined
// a list of conditional address ranges
typedef struct __CPROVER_assignable_car_list_t
{
  struct __CPROVER_assignable_car_list_t *next;
  __CPROVER_assignable_car_t car;
} __CPROVER_assignable_car_list_t;
#endif

#ifndef __CPROVER_assignable_obj_list_t_defined
#  define __CPROVER_assignable_obj_list_t_defined
// a list of object IDs
typedef struct __CPROVER_assignable_obj_list_t
{
  struct __CPROVER_assignable_obj_list_t *next;
  __CPROVER_size_t object_id;
} __CPROVER_assignable_obj_list_t;
#endif

__CPROVER_bool __CPROVER_assignable_nondet_bool();

void __CPROVER_assignable_car_list_invalidate_same_object(
  __CPROVER_assignable_car_list_t *,
  __CPROVER_size_t);

// Non-deterministically deallocates objects of the given list,
// records deallocated objects in 'deallocated'
// invalidates objects and cars in the given lists and maps.
void __CPROVER_assignable_obj_list_deallocate(

  // List of objects to free
  __CPROVER_assignable_obj_list_t *list,

  // Destination map to record deallocated objects
  __CPROVER_bool *deallocated,

  // Emptiness flag for the deallocated map
  __CPROVER_bool *deallocated_is_empty,

  // Assignable objects according to the top level contract (for invalidation)
  __CPROVER_assignable_car_list_t *contract_assigns,

  // Freeable objects according to the top level contract (for invalidation)
  __CPROVER_bool *contract_frees,

  // Currently heap allocated objects (for invalidation)
  __CPROVER_bool *heap_allocated)
{
  __CPROVER_assert(deallocated, "deallocated is not NULL");
  __CPROVER_assert(deallocated_is_empty, "deallocated_is_empty is not NULL");
  __CPROVER_assert(contract_frees, "contract_frees is not NULL");
  __CPROVER_assert(heap_allocated, "heap_allocated is not NULL");
  while(list)
  {
    if(__CPROVER_assignable_nondet_bool())
    {
      __CPROVER_size_t object_id = list->object_id;
      deallocated[object_id] = 1;
      *deallocated_is_empty = 0;
      contract_frees[object_id] = 0;
      heap_allocated[object_id] = 0;
      __CPROVER_assignable_car_list_invalidate_same_object(
        contract_assigns, object_id);
    }
    list = list->next;
  }
}
