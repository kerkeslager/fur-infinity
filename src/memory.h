#ifndef FUR_MEMORY_H
#define FUR_MEMORY_H

#include <assert.h>

/*
 * TODO Scan the code for unchecked usages of malloc, calloc, and realloc, and
 * update them to use these functions.
 */

/*
 * TODO Update the signatures of these functions to take the necessary args to
 * handle memory allocation failures in a graceful way. This will allow us to
 * verify that we're null-checking ALL allocations, by grepping the codebase to
 * ensure that we're only calling these functions, which the compiler will
 * verify are being called with args.
 */

inline static char* allocateChars(size_t n) {
  char* ptr = malloc(n);
  assert(ptr != NULL); /* TODO Implement handling */
  return ptr;
}

#define ALLOCATE_ONE_DECL(name) \
  name* name##_allocateOne()

#define ALLOCATE_ONE_IMPL(name) \
  name* name##_allocateOne() { \
    name* ptr = (name*)malloc(sizeof(name)); \
    assert(ptr != NULL); /* TODO Implement handling */ \
    return (name*)ptr; \
  }

#define ALLOCATOR_DECL(name) \
  name* name##_allocate(size_t)

#define ALLOCATOR_IMPL(name) \
  name* name##_allocate(size_t n) { \
    name* ptr = (name*)calloc(n, sizeof(name)); \
    assert(ptr != NULL); /* TODO Implement handling */ \
    return (name*)ptr; \
  }

#define RESIZER_DECL(name) \
  name* name##_resize(name*, size_t)

#define RESIZER_IMPL(name) \
  name* name##_resize(name* ptr, size_t n) { \
    name* ret = realloc(ptr, n * sizeof(name)); \
    assert(ret != NULL); /* TODO Implement handling */ \
    return ret; \
  }

#endif
