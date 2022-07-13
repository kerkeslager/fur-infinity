#ifndef FUR_OBJECT_H
#define FUR_OBJECT_H

#include <stdlib.h>

#include "value.h"

typedef enum {
  OBJ_CONCAT,
  OBJ_STRING
} ObjType;

struct Obj {
  Obj* next;
  ObjType type;
};

struct ObjConcat {
  Obj obj;
  Obj* o0;
  Obj* o1;
};

struct ObjString {
  Obj obj;
  size_t length;
  char* characters;
};

inline static void Obj_init(Obj*, ObjType);
void Obj_free(Obj*);
void Obj_printRepr(Obj*);

void ObjConcat_init(ObjConcat*, Obj*, Obj*);
void ObjConcat_free(ObjConcat*);
void ObjString_init(ObjString*, size_t, char*);
void ObjString_free(ObjString*);

#endif
