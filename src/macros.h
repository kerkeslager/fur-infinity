#ifndef FUR_MACROS_H
#define FUR_MACROS_H

#define LIST_DECL(name, member_type) \
  typedef struct { \
    size_t length; \
    size_t capacity; \
    member_type* items; \
  } name

#endif
