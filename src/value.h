#ifndef FUR_VALUE_H
#define FUR_VALUE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  enum {
    TYPE_NIL,
    TYPE_TRUE,
    TYPE_FALSE,
    TYPE_INTEGER,
  } is_a;

  union {
    int32_t integer;
  } as;
} Value;

#define isNil(v)      v.is_a == TYPE_NIL
#define isTrue(v)     v.is_a == TYPE_TRUE
#define isFalse(v)    v.is_a == TYPE_FALSE
#define isInteger(v)  v.is_a == TYPE_INTEGER

inline static bool isBoolean(Value v) {
  return isTrue(v) || isFalse(v);
}

static const Value VALUE_NIL = {
  TYPE_NIL,
  42
};

static const Value VALUE_TRUE = {
  TYPE_TRUE,
  42
};

static const Value VALUE_FALSE = {
  TYPE_FALSE,
  42
};

inline static Value Value_fromInt32(int32_t i) {
  Value result;
  result.is_a = TYPE_INTEGER;
  result.as.integer = i;
  return result;
}

void Value_printRepr(Value);
int Value_asSuccess(Value);

#endif
