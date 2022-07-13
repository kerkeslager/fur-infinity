#include "code.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "object.h"

void Instruction_print(Instruction i) {
  switch(i) {
    #define MAP(i) case i: printf(#i); break;
    MAP(OP_NIL);
    MAP(OP_TRUE);
    MAP(OP_FALSE);
    MAP(OP_INTEGER);
    MAP(OP_STRING);
    MAP(OP_AND);
    MAP(OP_OR);
    MAP(OP_ADD);
    MAP(OP_SUBTRACT);
    MAP(OP_MULTIPLY);
    MAP(OP_DIVIDE);
    MAP(OP_NEGATE);
    MAP(OP_NOT);
    MAP(OP_EQ);
    MAP(OP_LT);
    MAP(OP_GT);
    MAP(OP_NEQ);
    MAP(OP_GEQ);
    MAP(OP_LEQ);
    MAP(OP_PROP);
    MAP(OP_RETURN);
    #undef MAP

    default:
      assert(false);
  }
}

void ObjList_init(ObjList* self) {
  self->length = 0;
  self->capacity = 0;
  self->items = NULL;
}

void ObjList_free(ObjList* self) {
  for(size_t i = 0; i < self->length; i++) {
    Obj_free(self->items[i]);
  }

  self->length = 0;
  self->capacity = 0;
  free(self->items);
}

void LineRunList_init(LineRunList* self) {
  self->length = 0;
  self->capacity = 8;
  self->items = malloc(sizeof(LineRun) * self->capacity);
}

void LineRunList_free(LineRunList* self) {
  free(self->items);
}

void LineRunList_append(LineRunList* self, size_t line) {
  // If the current line is the same as the previous, continue the run
  if(self->length > 0 && self->items[self->length - 1].line == line) {
    self->items[self->length - 1].run++;
    return;
  }

  if(self->length == self->capacity) {
    self->capacity *= 2;
    self->items = realloc(self->items, sizeof(LineRun) * self->capacity);

    assert(self->items != NULL); // TODO Handle this.
  }

  LineRun lineRun;
  lineRun.line = line;
  lineRun.run = 1;

  self->items[self->length] = lineRun;
  self->length++;
}

void InstructionList_init(InstructionList* self) {
  /*
   * We initialize the instruction list with an initial capacity of 16. Most dynamic
   * arrays get capacity lazily, but we're doing this one eagerly because
   * it's going to be used immediately at the call site. Initializing it
   * eagerly also allows us to avoid an if-check when upsizing.
   */
  self->length = 0;
  self->capacity = 16;
  self->items = malloc(sizeof(uint8_t) * self->capacity);
}

void InstructionList_free(InstructionList* self) {
  free(self->items);
}

void InstructionList_append(InstructionList* self, uint8_t byte) {
  if(self->length == self->capacity) {
    self->capacity *= 2;
    self->items = realloc(self->items, sizeof(uint8_t) * self->capacity);

    assert(self->items != NULL); // TODO Handle this.
  }

  self->items[self->length] = byte;
  self->length++;
}

Code* Code_new() {
  Code* self = malloc(sizeof(Code));
  ObjList_init(&(self->interns));
  LineRunList_init(&(self->lineRuns));
  InstructionList_init(&(self->instructions));
  return self;
};

void Code_del(Code* self) {
  ObjList_free(&(self->interns)); // Free the interns!
  LineRunList_free(&(self->lineRuns));
  InstructionList_free(&(self->instructions));
  free(self);
}

void Code_append(Code* self, uint8_t byte, size_t line) {
  InstructionList_append(&(self->instructions), byte);
  LineRunList_append(&(self->lineRuns), line);
}

Instruction Code_get(Code* self, size_t index) {
  assert(index < self->instructions.length);
  return self->instructions.items[index];
}

int32_t Code_getInteger(Code* self, size_t index) {
  assert(index <= self->instructions.length - sizeof(int32_t));
  return *((int32_t*)(self->instructions.items + index));
}

uint8_t Code_internObject(Code* self, Obj* intern) {
  /*
   * TODO Check for duplicates. It could be costly at compile time,
   * but the payoff at run time is obvious.
   */

  size_t result = self->interns.length;
  assert(result < 256); // TODO Handle this.

  if(self->interns.length == self->interns.capacity) {
    if(self->interns.capacity == 0) {
      self->interns.capacity = 8;
    } else {
      self->interns.capacity = self->interns.capacity * 2;
    }

    self->interns.items = realloc(
        self->interns.items,
        sizeof(uint8_t) * self->interns.capacity
    );

    assert(self->interns.items != NULL);
  }

  self->interns.items[result] = intern;
  self->interns.length++;

  return (uint8_t) result;
}

Obj* Code_getInterned(Code* self, uint8_t index) {
  return self->interns.items[index];
}
