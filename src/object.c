#include "object.h"
#include <stdio.h>

inline static void Obj_init(Obj* self, ObjType type) {
  self->next = NULL;
  self->type = type;
}

void ObjConcat_init(ObjConcat* self, Obj* o0, Obj* o1) {
  Obj_init(&(self->obj), OBJ_CONCAT);

  /*
   * We only want concatenations to concatenate strings. Currently
   * the only forms of objects that exist *are* strings, but that
   * won't always be the case, so let's future-proof it.
   */
  assert(o0->type == OBJ_CONCAT || o0->type == OBJ_STRING);
  assert(o1->type == OBJ_CONCAT || o1->type == OBJ_STRING);

  self->o0 = o0;
  self->o1 = o1;
}

void ObjConcat_free(ObjConcat* self) {
  assert(self->obj.type == OBJ_CONCAT);
  /*
   * Currently we don't do anything, because all the possible children
   * are freed in other places:
   * - Strings are all interned, so they are freed by Code.
   * - Concatenations are on the stack and will be freed by the garbage
   *   collector.
   */
}

void ObjString_init(ObjString* self, size_t length, char* characters) {
  Obj_init(&(self->obj), OBJ_STRING);
  self->length = length;
  self->characters = characters;
}

void ObjString_free(ObjString* self) {
  assert(self->obj.type = OBJ_STRING);
  free(self->characters);
}

void Obj_free(Obj* self) {
  switch(self->type) {
    case OBJ_CONCAT:
      ObjConcat_free((ObjConcat*)self);
      break;

    case OBJ_STRING:
      ObjString_free((ObjString*)self);
      break;

    default:
      assert(false);
  }

  free(self);
}

void Obj_printReprNoQuote(Obj* self);

void ObjConcat_printReprNoQuote(ObjConcat* self) {
  assert(self->obj.type == OBJ_CONCAT);
  Obj_printReprNoQuote(self->o0);
  Obj_printReprNoQuote(self->o1);
}

void ObjString_printReprNoQuote(ObjString* self) {
  assert(self->obj.type == OBJ_STRING);

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
        printf("\\\"");
        break;
      case '\n':
        printf("\\n");
        break;
      case '\r':
        printf("\\r");
        break;
      case '\t':
        printf("\\t");
        break;
      default:
        printf("%c", self->characters[i]);
    }
  }
}

void Obj_printReprNoQuote(Obj* self) {
  switch(self->type) {
    case OBJ_CONCAT:
      return ObjConcat_printReprNoQuote((ObjConcat*) self);

    case OBJ_STRING:
      return ObjString_printReprNoQuote((ObjString*) self);

    default:
      assert(false);
  }
}

void Obj_printRepr(Obj* self) {
  printf("\"");
  Obj_printReprNoQuote(self);
  printf("\"");
}
