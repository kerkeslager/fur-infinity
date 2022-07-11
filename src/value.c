#include "stdio.h"

#include "value.h"

void Value_printRepr(Value value) {
  printf("%d", value.as.integer);
}

int Value_asSuccess(Value value) {
  return (int)value.as.integer;
}
