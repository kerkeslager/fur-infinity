#ifndef FUR_VALUE_H
#define FUR_VALUE_H

#include <stdint.h>

typedef struct {
  union {
    int32_t integer;
  } as;
} Value;

void Value_printRepr(Value);
int Value_asSuccess(Value);

#endif
