#ifndef FUR_VALUE_H
#define FUR_VALUE_H

#include <stdint.h>

typedef struct {
  int32_t integer;
} Value;

void Value_printRepr(Value);
int Value_asSuccess(Value);

#endif
