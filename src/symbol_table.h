#ifndef FUR_SYMBOL_TABLE_H
#define FUR_SYMBOL_TABLE_H

#include "symbol.h"

typedef struct {
  size_t capacity;
  size_t load;
  Symbol** items;
} SymbolTable;

void SymbolTable_init(SymbolTable*);
void SymbolTable_free(SymbolTable*);
Symbol* SymbolTable_getSymbol(SymbolTable*, uint8_t length, char* name);

#endif
