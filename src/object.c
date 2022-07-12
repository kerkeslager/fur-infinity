#include "object.h"

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
