#ifndef FUR_SYMBOL_H
#define FUR_SYMBOL_H

#include "stdint.h"

/*
 * Member variables of Symbol are ordered largest to smallest for struct
 * packing.
 */
typedef struct {
  char* name;
  uint32_t hash;
  uint8_t length;
} Symbol;

void Symbol_init(Symbol*, uint32_t h, size_t length, char*);
void Symbol_printRepr(Symbol*);

#endif
