# ifndef FUR_COMPILER_H
# define FUR_COMPILER_H

#include "code.h"

typedef struct {
  int notImplemented; // TODO Implement.
} Compiler;

void Compiler_init(Compiler*);
void Compiler_free(Compiler*);

size_t Compiler_compile(Compiler*, Code*, char*);

#endif
