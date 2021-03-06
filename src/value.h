#ifndef FUR_VALUE_H
#define FUR_VALUE_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct Obj Obj;
typedef struct ObjString ObjString;

typedef struct {
  enum {
    TYPE_NIL,
    TYPE_BOOLEAN,
    TYPE_INTEGER,
    TYPE_OBJ,
  } is_a;

  union {
    bool boolean;
    int32_t integer;
    Obj* obj;
  } as;
} Value;

#define isNil(v)      (v.is_a == TYPE_NIL)
#define isInteger(v)  (v.is_a == TYPE_INTEGER)
#define isObj(v)      (v.is_a == TYPE_OBJ)

static bool isTrue(Value v) {
  return v.is_a == TYPE_BOOLEAN && v.as.boolean;
}

static bool isFalse(Value v) {
  return v.is_a == TYPE_BOOLEAN && !(v.as.boolean);
}

/*
 * TODO Profile storing a bool in the union instead. It requires more
 * comparisons to do equality, but I suspect the simplicity of branching
 * on type and then reading the union for true/false will outweigh that.
 */
inline static bool isBoolean(Value v) {
  return isTrue(v) || isFalse(v);
}

inline static bool Value_toBool(Value v) {
  assert(isBoolean(v));
  return isTrue(v);
}

inline static Value Value_fromBool(bool b) {
  Value result;
  result.is_a = TYPE_BOOLEAN;
  result.as.boolean = b;
  return result;
}

inline static Value Value_fromInt32(int32_t i) {
  Value result;
  result.is_a = TYPE_INTEGER;
  result.as.integer = i;
  return result;
}

inline static Value Value_fromObj(Obj* o) {
  Value result;
  result.is_a = TYPE_OBJ;
  result.as.obj = o;
  return result;
}

void Value_printRepr(Value);

#endif
