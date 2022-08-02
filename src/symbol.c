#include <assert.h>
#include <stdio.h>
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

void Symbol_printRepr(Symbol* self) {
  printf(":");
  for(uint8_t i = 0; i < self->length; i++) {
    printf("%c", self->name[i]);
  }
}
