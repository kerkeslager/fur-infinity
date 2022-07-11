#include "runtime.h"
#include "thread.h"

void Runtime_init(Runtime* self) {
}

void Runtime_free(Runtime* self) {
}

Value Runtime_run(Runtime* self, Code* code) {
  Thread thread;
  Thread_init(&thread);

  Value result = Thread_run(&thread, code);

  Thread_free(&thread);

  return result;
};
