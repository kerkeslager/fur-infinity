#ifndef FUR_RUNTIME_H
#define FUR_RUNTIME_H

#include "symbol.h"
#include "symbol_table.h"

typedef struct {
  SymbolTable symbols;
} Runtime;

void Runtime_init(Runtime*);
void Runtime_free(Runtime*);

Symbol* Runtime_getSymbol(Runtime*, size_t length, char* name);

#endif

