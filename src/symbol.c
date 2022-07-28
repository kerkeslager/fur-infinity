#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "symbol.h"

void Symbol_init(Symbol* self, uint32_t h, size_t length, char* name) {
  /*
   * The symbols are generally pulled from source, which will be held
   * for the duration of runtime just like Symbols are, so we don't
   * need to copy or reallocate them.
   */
  assert(length <= UINT8_MAX);
  self->hash = h;
  self->length = length;
  self->name = name;
}
