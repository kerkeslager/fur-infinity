#ifndef FUR_RUNTIME_H
#define FUR_RUNTIME_H

#include "symbol.h"

typedef struct {
  size_t capacity;
  size_t load;
  Symbol** items;
} SymbolTable;

typedef struct {
  SymbolTable symbols;
} Runtime;

void Runtime_init(Runtime*);
void Runtime_free(Runtime*);

Symbol* Runtime_getSymbol(Runtime*, size_t length, char* name);

#endif

