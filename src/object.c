#include "object.h"
#include <stdio.h>

inline static void Obj_init(Obj* self, ObjType type) {
  self->next = NULL;
  self->type = type;
}

void ObjNative_init(ObjNative* self, Value (*call)(uint8_t, Value*)) {
  Obj_init(&(self->obj), OBJ_NATIVE);
  self->call = call;
}

void ObjString_init(ObjString* self, size_t length, char* characters) {
  Obj_init(&(self->obj), OBJ_STRING);
  self->length = length;
  self->characters = characters;
}

void ObjString_free(ObjString* self) {
  free(self->characters);
}

void Obj_free(Obj* self) {
  switch(self->type) {
    case OBJ_NATIVE:
      break;

    case OBJ_STRING:
      ObjString_free((ObjString*)self);
      break;

    default:
      assert(false);
  }

  free(self);
}

void ObjNative_printRepr(ObjNative* self) {
  printf("<native>");
}

void ObjString_printRepr(ObjString* self) {
  printf("\"");
  for(size_t i = 0; i < self->length; i++) {
    // This is the *representation*, not the actual string
    // so we need to unescape characters
    switch(self->characters[i]) {
      case '\\':
        printf("\\\\");
        break;
      case '\'':
        printf("\\\'");
        break;
      case '\"':
        printf("\\\'");
        break;
      case '\n':
        printf("\\\'");
        break;
      case '\r':
        printf("\\\'");
        break;
      case '\t':
        printf("\\\'");
        break;
      default:
        printf("%c", self->characters[i]);
    }
  }
  printf("\"");
}

void Obj_printRepr(Obj* self) {
  switch(self->type) {
    case OBJ_NATIVE:
      return ObjNative_printRepr((ObjNative*) self);

    case OBJ_STRING:
      return ObjString_printRepr((ObjString*) self);

    default:
      assert(false);
  }
}

Value nativePrint(uint8_t argc, Value* argv) {
  for(uint8_t i = 0; i < argc; i++) {
    assert(argv[i].is_a == TYPE_OBJ);
    assert(argv[i].as.obj->type == OBJ_STRING);

    ObjString* s = (ObjString*)(argv[i].as.obj);

    for(size_t c = 0; c < s->length; c++) {
      printf("%c", s->characters[c]);
    }
  }

  Value result;
  result.is_a = TYPE_NIL;
  return result;
}
