#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "runtime.h"
#include "symbol.h"
#include "symbol_table.h"

Symbol* Runtime_getSymbol(Runtime* self, size_t length, char* name) {
  assert(self != NULL);
  assert(name != NULL);
  assert(length <= UINT8_MAX);
  return SymbolTable_getSymbol(&(self->symbols), (uint8_t)length, name);
}

void Runtime_init(Runtime* self) {
  SymbolTable_init(&(self->symbols));
}

void Runtime_free(Runtime* self) {
  SymbolTable_free(&(self->symbols));
}
