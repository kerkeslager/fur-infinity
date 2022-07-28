# ifndef FUR_COMPILER_H
# define FUR_COMPILER_H

#include "code.h"
#include "parser.h"
#include "symbol.h"

#define MAX_SYMBOLSTACK_DEPTH 256

typedef struct {
  Symbol items[MAX_SYMBOLSTACK_DEPTH];
  Symbol* top;
} SymbolStack;

void SymbolStack_init(SymbolStack*);
void SymbolStack_free(SymbolStack*);

// TODO Profile inlining these.
void SymbolStack_push(SymbolStack*, Symbol);
Symbol SymbolStack_pop(SymbolStack*);
Symbol SymbolStack_peek(SymbolStack*, uint8_t depth);

typedef struct {
  SymbolStack stack;
  size_t scope;
} Compiler;

void Compiler_init(Compiler*);
void Compiler_free(Compiler*);

size_t Compiler_compile(Compiler*, Code*, Node*);

#endif
