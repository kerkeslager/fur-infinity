#include <stdio.h>
#include <string.h>

#include "object.h"

inline static void Obj_init(Obj* self, ObjType type) {
  self->next = NULL;
  self->type = type;
}

ALLOCATE_ONE_IMPL(ObjNative);

void ObjNative_init(ObjNative* self, Value (*call)(uint8_t, Value*)) {
  Obj_init(&(self->obj), OBJ_NATIVE);
  self->call = call;
}

ALLOCATE_ONE_IMPL(ObjString);
ALLOCATOR_IMPL(ObjString);

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

bool ObjString_equals(ObjString* self, ObjString* other) {
  assert(self->obj.type == OBJ_STRING);
  assert(other->obj.type == OBJ_STRING);

  return self->length == other->length &&
    !strncmp(self->characters, other->characters, self->length);
}

bool Obj_equals(Obj* self, Obj* other) {
  if(self == other) return true;

  switch(self->type) {
    case OBJ_CLOSURE:
      assert(false);

    case OBJ_NATIVE:
      return other->type == OBJ_NATIVE &&
        ((ObjNative*)self)->call == ((ObjNative*)other)->call;

    case OBJ_STRING:
      return other->type == OBJ_STRING &&
        ObjString_equals((ObjString*)self, (ObjString*)other);
  }
}

void ObjNative_printRepr(ObjNative* self) {
  printf("<native>");
}

void ObjString_printRepr(ObjString* self) {
  /*
   * Choose whether to use single or double quotes to quote the string
   * representation, in a way that minimizes escapes. This isn't performant,
   * but it makes it look nicer in a way that's still pastable into code,
   * and since this is primarily a debugging tool, performance isn't a high
   * priority.
   */
  size_t singeQuoteCount = 0;
  size_t doubleQuoteCount = 0;

  for(size_t i = 0; i < self->length; i++) {
    switch(self->characters[i]) {
      case '\'':
        singeQuoteCount++;
        break;
      case '"':
        doubleQuoteCount++;
        break;
    }
  }

  char quoteChar = '\'';

  if(singeQuoteCount > doubleQuoteCount) {
    quoteChar = '"';
  }

  printf("%c", quoteChar);
  for(size_t i = 0; i < self->length; i++) {
    // This is the *representation*, not the actual string
    // so we need to unescape characters
    switch(self->characters[i]) {
      case '\'':
        switch(quoteChar) {
          case '\'':
            printf("\\\'");
            break;
          case '"':
            printf("'");
            break;
          default:
            assert(false);
        }
        break;

      case '\"':
        switch(quoteChar) {
          case '\'':
            printf("\"");
            break;
          case '"':
            printf("\\\"");
            break;
          default:
            assert(false);
        }
        break;

      case '\\':
        printf("\\\\");
        break;
      case '\n':
        printf("\\\n");
        break;
      case '\r':
        printf("\\\r");
        break;
      case '\t':
        printf("\\\t");
        break;
      default:
        printf("%c", self->characters[i]);
    }
  }
  printf("%c", quoteChar);
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
    switch(argv[i].is_a) {
      case TYPE_NIL:
        printf("nil");
        break;

      case TYPE_BOOLEAN:
        if(argv[i].as.boolean) {
          printf("true");
        } else {
          printf("false");
        } break;

      case TYPE_INTEGER:
        printf("%i", argv[i].as.integer);
        break;

      case TYPE_OBJ:
        {
          assert(argv[i].as.obj->type == OBJ_STRING);

          ObjString* s = (ObjString*)(argv[i].as.obj);

          for(size_t c = 0; c < s->length; c++) {
            printf("%c", s->characters[c]);
          }
        } break;

      default:
        assert(false); // TODO Handle other types
    }
  }

  Value result;
  result.is_a = TYPE_NIL;
  return result;
}
