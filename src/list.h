#ifndef FUR_MACROS_H
#define FUR_MACROS_H

#define LIST_DECL(name, member_type) \
  typedef struct { \
    size_t length; \
    size_t capacity; \
    member_type* items; \
  } name

#define LIST_IMPL_LENGTH(name) \
  inline static size_t name##_length(name* self) { \
    return self->length; \
  }

#define LIST_IMPL_INIT_PREALLOC(name, member_type, init_capacity) \
  void name##_init(name* self) { \
    self->length = 0; \
    self->capacity = init_capacity; \
    self->items = malloc(sizeof(member_type) * self->capacity); \
  }

#define LIST_IMPL_INIT_NO_PREALLOC(name) \
  void name##_init(name* self) { \
    self->length = 0; \
    self->capacity = 0; \
    self->items = NULL; \
  }

#define LIST_IMPL_FREE_WITH_ITEMS(name, member_free_function) \
  void name##_free(name* self) { \
    for(size_t i = 0; i < self->length; i++) { \
      member_free_function(self->items[i]); \
    } \
    \
    self->length = 0; \
    self->capacity = 0; \
    free(self->items); \
  }

#define LIST_IMPL_FREE_WITHOUT_ITEMS(name) \
  void name##_free(name* self) { \
    free(self->items); \
  }

#define LIST_IMPL_APPEND_PREALLOC(name, member_type) \
  void name##_append(name* self, member_type item) { \
    if(self->length == self->capacity) { \
      assert(self->capacity != 0); \
      self->capacity = self->capacity * 2; \
      member_type* tmp = realloc(self->items, sizeof(member_type) * self->capacity); \
      assert(tmp != NULL); /* TODO Handle this */\
      self->items = tmp; \
    } \
    \
    self->items[self->length] = item; \
    self->length++; \
  }

#define LIST_IMPL_APPEND_NO_PREALLOC(name, member_type, init_capacity) \
  void name##_append(name* self, member_type item) { \
    if(self->length == self->capacity) { \
      if(self->capacity == 0) { \
        self->capacity = init_capacity; \
        self->items = malloc(sizeof(member_type) * init_capacity); \
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
