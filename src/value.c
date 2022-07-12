#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "value.h"
#include "object.h"

void Value_printRepr(Value value) {
  switch(value.is_a) {
    case TYPE_NIL:    printf("nil");    return;
    case TYPE_TRUE:   printf("true");   return;
    case TYPE_FALSE:  printf("false");  return;

    case TYPE_INTEGER:
      printf("%d", value.as.integer);
      return;

    case TYPE_OBJ:
      return Obj_printRepr(value.as.obj);

    default:
      assert(false);
      return;
  }
}

int Value_asSuccess(Value value) {
  assert(value.is_a == TYPE_INTEGER);
  return (int)value.as.integer;
}
