#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "value.h"
#include "object.h"

void Value_printRepr(Value value) {
  switch(value.is_a) {
    case TYPE_NIL:
      printf("nil");
      return;

    case TYPE_BOOLEAN:
      if(value.as.boolean) {
        printf("true");
      } else {
        printf("false");
      } return;

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
