#ifndef FUR_OBJECT_H
#define FUR_OBJECT_H

#include <stdlib.h>

#include "code.h"
#include "memory.h"
#include "symbol.h"
#include "value.h"

typedef enum {
  OBJ_CLOSURE,
  OBJ_NATIVE,
  OBJ_STRING
} ObjType;

struct Obj {
  Obj* next;
  ObjType type;
};

typedef struct {
  Obj obj;
  Code* code;
  Symbol* name;
  uint8_t arity;
} ObjClosure;

typedef struct {
  Obj obj;
  Value (*call)(uint8_t argc, Value* argv);
} ObjNative;

struct ObjString {
  Obj obj;
  size_t length;
  char* characters;
};

inline static void Obj_init(Obj*, ObjType);
void Obj_free(Obj*);
void Obj_printRepr(Obj*);
bool Obj_equals(Obj*, Obj*);

ALLOCATE_ONE_DECL(ObjClosure);
void ObjClosure_init(ObjClosure*, Symbol*, uint8_t, Code*);
void ObjClosure_free(ObjClosure*);

ALLOCATE_ONE_DECL(ObjNative);
void ObjNative_init(ObjNative*, Value (*call)(uint8_t, Value*));

ALLOCATE_ONE_DECL(ObjString);
ALLOCATOR_DECL(ObjString);
void ObjString_init(ObjString*, size_t, char*);
void ObjString_free(ObjString*);
bool ObjString_equals(ObjString*, ObjString*);

Value nativeInput(uint8_t argc, Value* argv);
Value nativePrint(uint8_t argc, Value* argv);

typedef struct {
  const char* name;
  Value (*call)(uint8_t, Value*);
} NamedNative;

#define NATIVE_COUNT 2

static const NamedNative NATIVE[NATIVE_COUNT] = {
  { .name="input", .call=nativeInput },
  { .name="print", .call=nativePrint }
};

#endif
