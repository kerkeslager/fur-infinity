#ifndef FUR_CODE_H
#define FUR_CODE_H

#include <stdlib.h>

typedef enum {
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_INTEGER,
  OP_AND,
  OP_OR,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NEGATE,
  OP_NOT,
  OP_EQ,
  OP_LT,
  OP_GT,
  OP_NEQ,
  OP_GEQ,
  OP_LEQ,
  OP_PROP,
  OP_RETURN,
} Instruction;

typedef struct {
  size_t line;
  size_t run;
} LineRun;

typedef struct {
  size_t length;
  size_t capacity;
  LineRun* items;
} LineRunList;

typedef struct {
  size_t length;
  size_t capacity;
  uint8_t* items;
} InstructionList;

typedef struct {
  LineRunList lineRuns;
  InstructionList instructions;
} Code;

Code* Code_new();
void Code_del(Code*);
void Code_append(Code* self, uint8_t instruction, size_t line);

// TODO Profile inlining these.
/* TODO We're tracking the index in the calling function, which
 * exposes the internals of Code a bit. Consider some sort of iterator
 * instead. But this is *highly* performance-critical code, so we need to
 * profile to do anything.
 */
Instruction Code_get(Code*, size_t index);
int32_t Code_getInteger(Code*, size_t index);

#endif
