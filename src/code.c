#include "code.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "object.h"

void Instruction_print(Instruction i) {
  switch(i) {
    #define MAP(i) case i: printf(#i); break;
    MAP(OP_ADD);
    MAP(OP_AND);
    MAP(OP_CALL);
    MAP(OP_DIVIDE);
    MAP(OP_DROP);
    MAP(OP_EQ);
    MAP(OP_FALSE);
    MAP(OP_GEQ);
    MAP(OP_GET);
    MAP(OP_GT);
    MAP(OP_INTEGER);
    MAP(OP_JUMP);
    MAP(OP_JUMP_IF_TRUE);
    MAP(OP_JUMP_IF_FALSE);
    MAP(OP_LEQ);
    MAP(OP_LT);
    MAP(OP_MULTIPLY);
    MAP(OP_NATIVE);
    MAP(OP_NEGATE);
    MAP(OP_NEQ);
    MAP(OP_NIL);
    MAP(OP_NOT);
    MAP(OP_OR);
    MAP(OP_PROP);
    MAP(OP_RETURN);
    MAP(OP_SET);
    MAP(OP_STRING);
    MAP(OP_SUBTRACT);
    MAP(OP_TRUE);
    #undef MAP

    default:
      assert(false);
  }
}

LIST_IMPL_INIT_NO_PREALLOC(ObjList);
LIST_IMPL_FREE_WITH_ITEMS(ObjList, Obj_free);
LIST_IMPL_APPEND_NO_PREALLOC(ObjList, Obj*, 8);

LIST_IMPL_INIT_PREALLOC(LineRunList, LineRun, 8);
LIST_IMPL_FREE_WITHOUT_ITEMS(LineRunList);
LIST_IMPL_APPEND_PREALLOC(LineRunList, LineRun);

void LineRunList_appendLine(LineRunList* self, size_t line) {
  // If the current line is the same as the previous, continue the run
  if(self->length > 0 && self->items[self->length - 1].line == line) {
    self->items[self->length - 1].run++;
    return;
  }

  LineRun lineRun;
  lineRun.line = line;
  lineRun.run = 1;

  LineRunList_append(self, lineRun);
}

/*
 * We initialize the instruction list with an initial capacity of 32. Most dynamic
 * arrays get capacity lazily, but we're doing this one eagerly because
 * it's going to be used immediately at the call site. Initializing it
 * eagerly also allows us to avoid an if-check when upsizing.
 */
LIST_IMPL_INIT_PREALLOC(InstructionList,uint8_t,32);
LIST_IMPL_FREE_WITHOUT_ITEMS(InstructionList);
LIST_IMPL_LENGTH(InstructionList);
LIST_IMPL_APPEND_PREALLOC(InstructionList,uint8_t);

void Code_init(Code* self) {
  ObjList_init(&(self->interns));
  LineRunList_init(&(self->lineRuns));
  InstructionList_init(&(self->instructions));
};

void Code_free(Code* self) {
  ObjList_free(&(self->interns)); // Free the interns!
  LineRunList_free(&(self->lineRuns));
  InstructionList_free(&(self->instructions));
}

size_t Code_append(Code* self, uint8_t byte, size_t line) {
  size_t result = InstructionList_length(&(self->instructions));
  InstructionList_append(&(self->instructions), byte);
  LineRunList_appendLine(&(self->lineRuns), line);
  return result;
}

Instruction Code_get(Code* self, size_t index) {
  assert(index < self->instructions.length);
  return self->instructions.items[index];
}

int16_t Code_getInt16(Code* self, size_t index) {
  assert(index <= self->instructions.length - sizeof(int16_t));
  return *((int16_t*)(self->instructions.items + index));
}

int32_t Code_getInt32(Code* self, size_t index) {
  assert(index <= self->instructions.length - sizeof(int32_t));
  return *((int32_t*)(self->instructions.items + index));
}

size_t Code_getCurrent(Code* self) {
  return self->instructions.length;
}

uint8_t Code_internObject(Code* self, Obj* intern) {
  /*
   * TODO Check for duplicates. It could be costly at compile time,
   * but the payoff at run time is obvious.
   */

  size_t result = self->interns.length;
  assert(result < 256); /* TODO Handle this */

  ObjList_append(&(self->interns), intern);

  return (uint8_t) result;
}

Obj* Code_getInterned(Code* self, uint8_t index) {
  return self->interns.items[index];
}
