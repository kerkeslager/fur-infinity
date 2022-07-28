#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "symbol.h"

void Symbol_init(Symbol* self, size_t length, char* name) {
  assert(length <= 255);
  self->length = length;
  self->name = allocateChars(length + 1);
  strncpy(self->name, name, length);
  self->name[length] = '\0';
}

void Symbol_free(Symbol* self) {
  free(self->name);
}
