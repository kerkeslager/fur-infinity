#include "stdio.h"

#include "value.h"

void Value_printRepr(Value value) {
  printf("%d", value.integer);
}

int Value_asSuccess(Value value) {
  return (int)value.integer;
}
