#ifndef FUR_CODE_H
#define FUR_CODE_H

#include <stdlib.h>

#include "value.h"

typedef enum {
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_INTEGER,
  OP_STRING,
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
  OP_SET,
  OP_GET,
  OP_PROP,
  OP_RETURN,
} Instruction;

void Instruction_print(Instruction);

/*
 * TODO We have enough of these dynamic arrays that we can start pulling
 * out some duplications into macros.
 */

typedef struct {
  size_t length;
  size_t capacity;
  Obj** items;
} ObjList;

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
  ObjList interns;
  LineRunList lineRuns;
  InstructionList instructions;
} Code;

void Code_init(Code*);
void Code_free(Code*);

void Code_append(Code* self, uint8_t instruction, size_t line);
uint8_t Code_internObject(Code* self, Obj* intern);

// TODO Profile inlining these.
/* TODO We're tracking the index in the calling function, which
 * exposes the internals of Code a bit. Consider some sort of iterator
 * instead. But this is *highly* performance-critical code, so we need to
 * profile to do anything.
 */
Instruction Code_get(Code*, size_t index);
int32_t Code_getInteger(Code*, size_t index);

Obj* Code_getInterned(Code* self, uint8_t index);

#endif
