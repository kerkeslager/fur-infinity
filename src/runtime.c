#include "runtime.h"

void Runtime_init(Runtime* self) {
}

void Runtime_free(Runtime* self) {
}

Value Runtime_run(Runtime* self, Code* code) {
  return (Value){ .integer = 0 };
};
