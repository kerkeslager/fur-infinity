#ifndef FUR_CODE_H
#define FUR_CODE_H

#include <stdlib.h>

#include "macros.h"
#include "value.h"

typedef enum {
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_INTEGER,
  OP_STRING,
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
  OP_SET,
  OP_GET,
  OP_PROP,
  OP_JUMP,
  OP_JUMP_IF_TRUE,
  OP_JUMP_IF_FALSE,
  OP_AND,
  OP_OR,
  OP_RETURN,
} Instruction;

void Instruction_print(Instruction);

LIST_DECL(ObjList, Obj*);

typedef struct {
  size_t line;
  size_t run;
} LineRun;

LIST_DECL(LineRunList, LineRun);
LIST_DECL(InstructionList, uint8_t);

typedef struct {
  ObjList interns;
  LineRunList lineRuns;
  InstructionList instructions;
} Code;

void Code_init(Code*);
void Code_free(Code*);

size_t Code_append(Code* self, uint8_t instruction, size_t line);
uint8_t Code_internObject(Code* self, Obj* intern);

// TODO Profile inlining these.
/* TODO We're tracking the index in the calling function, which
 * exposes the internals of Code a bit. Consider some sort of iterator
 * instead. But this is *highly* performance-critical code, so we need to
 * profile to do anything.
 */
Instruction Code_get(Code*, size_t index);
int16_t Code_getInt16(Code*, size_t index);
int32_t Code_getInt32(Code*, size_t index);

Obj* Code_getInterned(Code* self, uint8_t index);

#endif
