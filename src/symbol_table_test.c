#include <assert.h>
#include <stdio.h>

#include "symbol.h"
#include "symbol_table.h"

int main() {
  SymbolTable symbols;
  SymbolTable_init(&symbols);

  char* buffer = "ab";
  char* buffer2 = "a";

  Symbol* a   = SymbolTable_getSymbol(&symbols, 1, buffer);
  Symbol* a2  = SymbolTable_getSymbol(&symbols, 1, buffer2);

  assert(a == a2);

  Symbol* b = SymbolTable_getSymbol(&symbols, 1, buffer + 1);

  assert(a != b);

  printf("%c\n", a->name[0]);
  printf("%c\n", b->name[0]);
}
