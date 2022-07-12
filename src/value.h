#ifndef FUR_VALUE_H
#define FUR_VALUE_H

#include <assert.h>
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

static const Value VALUE_NIL = {
  TYPE_NIL
};

inline static bool isBoolean(Value v) {
  return isTrue(v) || isFalse(v);
}

inline static bool Value_toBool(Value v) {
  assert(isBoolean(v));
  return isTrue(v);
}

inline static Value Value_fromBool(bool b) {
  Value result;
  result.is_a = b ? TYPE_TRUE : TYPE_FALSE;
  return result;
}

inline static Value Value_fromInt32(int32_t i) {
  Value result;
  result.is_a = TYPE_INTEGER;
  result.as.integer = i;
  return result;
}

void Value_printRepr(Value);
int Value_asSuccess(Value);

#endif
