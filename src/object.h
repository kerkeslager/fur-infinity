#ifndef FUR_OBJECT_H
#define FUR_OBJECT_H

#include <stdlib.h>

#include "value.h"

typedef enum {
  OBJ_STRING
} ObjType;

struct Obj {
  Obj* next;
  ObjType type;
};

struct ObjString {
  Obj obj;
  size_t length;
  char* characters;
};

inline static void Obj_init(Obj*, ObjType);
void Obj_free(Obj*);
void Obj_printRepr(Obj*);

void ObjString_init(ObjString*, size_t, char*);
void ObjString_free(ObjString*);

#endif
