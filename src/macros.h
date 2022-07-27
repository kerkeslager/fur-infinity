#ifndef FUR_MACROS_H
#define FUR_MACROS_H

#define LIST_DECL(name, member_type) \
  typedef struct { \
    size_t length; \
    size_t capacity; \
    member_type* items; \
  } name

#define LIST_APPEND_IMPL(name, member_type, init_size) \
  void name##_append(name* self, member_type item) { \
    if(self->length == self->capacity) { \
      if(self->capacity == 0) { \
        self->capacity = init_size; \
        self->items = malloc(sizeof(member_type) * init_size); \
        assert(self->items != NULL); /* TODO Handle this */ \
      } else { \
        self->capacity = self->capacity * 2; \
        member_type* tmp = realloc(self->items, sizeof(member_type) * self->capacity); \
        assert(tmp != NULL); /* TODO Handle this */\
        self->items = tmp; \
      } \
    } \
    \
    self->items[self->length] = item; \
    self->length++; \
  }

#endif
