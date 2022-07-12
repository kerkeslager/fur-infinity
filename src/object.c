#include "object.h"
#include <stdio.h>

inline static void Obj_init(Obj* self, ObjType type) {
  self->next = NULL;
  self->type = type;
}

void ObjString_init(ObjString* self, ObjType type, size_t length, char* characters) {
  Obj_init(&(self->obj), type);
  self->length = length;
  self->characters = characters;
}

void ObjString_free(ObjString* self) {
  free(self->characters);
}

void Obj_free(Obj* self) {
  switch(self->type) {
    case OBJ_STRING:
      ObjString_free((ObjString*)self);
      break;

    default:
      assert(false);
  }

  free(self);
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
    case OBJ_STRING:
      return ObjString_printRepr((ObjString*) self);

    default:
      assert(false);
  }
}
