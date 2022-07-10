#ifndef FUR_RUNTIME_H
#define FUR_RUNTIME_H

#include "code.h"
#include "value.h"

typedef struct {
  int notImplemented; // TODO Implement.
} Runtime;

void Runtime_init(Runtime*);
void Runtime_free(Runtime*);

Value Runtime_run(Runtime*,Code*);

#endif
