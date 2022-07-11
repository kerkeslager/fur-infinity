#ifndef FUR_RUNTIME_H
#define FUR_RUNTIME_H

#include "code.h"
#include "value.h"

/*
 * When we implement threads, the Runtime will manage them.
 *
 * Currently we have only one thread, so the Runtime is a pointless
 * abstraction, but hopefully it will be easier to implement threads later
 * if we have this in advance.
 */

typedef struct {
  int notImplemented; // TODO Implement.
} Runtime;

void Runtime_init(Runtime*);
void Runtime_free(Runtime*);

Value Runtime_run(Runtime*,Code*);

#endif
