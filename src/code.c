#include "code.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

uint8_t Code_getUInt8(Code* self, uint8_t* ip) {
  assert((size_t)(ip - self->instructions.items)
      < self->instructions.length);
  return *ip;
}

int16_t Code_getInt16(Code* self, uint8_t* ip) {
  /* Note the "<=", not "<" */
  assert((size_t)(ip - self->instructions.items)
      <= self->instructions.length - sizeof(int16_t));
  return *((int16_t*)ip);
}

int32_t Code_getInt32(Code* self, uint8_t* ip) {
  /* Note the "<=", not "<" */
  assert((size_t)(ip - self->instructions.items)
      <= self->instructions.length - sizeof(int32_t));
  return *((int32_t*)ip);
}

size_t Code_getCurrent(Code* self) {
  return self->instructions.length;
}

uint8_t Code_internObject(Code* self, Obj* intern) {
  switch(intern->type) {
    case OBJ_CLOSURE:
      /*
       * There can't really be duplicate closures.
       */
      break;

    case OBJ_STRING:
      /*
       * TODO Check for duplicates. It could be costly at compile time,
       * but the payoff at run time is obvious.
       */
      break;

    default:
      assert(false);
  }

  size_t result = self->interns.length;
  assert(result < 256); /* TODO Handle this */

  ObjList_append(&(self->interns), intern);

  return (uint8_t) result;
}

Obj* Code_getInterned(Code* self, uint8_t index) {
  return self->interns.items[index];
}

void Code_printAsAssembly(Code* code, size_t startInstructionIndex) {
  size_t lineRunIndex = 0;
  size_t lineRunCounter = 0;

  #define INCREMENT_LINE() \
    do { \
      assert(lineRunIndex < code->lineRuns.length); \
      if(lineRunCounter == code->lineRuns.items[lineRunIndex].run) { \
        lineRunIndex++; \
      } \
    } while(false)


  // Run the line up to the current spot
  for(size_t i = 0; i < startInstructionIndex; i++) {
    INCREMENT_LINE();
  }

  for(size_t i = startInstructionIndex; i < code->instructions.length; i++) {
    char opString[32] = "";
    char argString[32] = "";

    INCREMENT_LINE();
    #undef INCREMENT_LINE

    size_t instructionIndex = i;
    size_t line = code->lineRuns.items[lineRunIndex].line;

    switch(code->instructions.items[i]) {
      case OP_INTEGER:
        strcpy(opString, "push_int");
        sprintf(
            argString,
            "%d",
            *((int32_t*)(code->instructions.items + i + 1))
        );
        i += sizeof(int32_t);
        break;
      case OP_STRING:
        strcpy(opString, "push_string");
        i++;
        sprintf(
            argString,
            "%d",
            code->instructions.items[i]
        );
        break;

      #define ONE_BYTE_ARG(op, name) \
        case op: \
          strcpy(opString, #name); \
          i++; \
          sprintf( \
              argString, \
              "%d", \
              code->instructions.items[i] \
          ); \
          break
      ONE_BYTE_ARG(OP_SET, set);
      ONE_BYTE_ARG(OP_GET, get);
      ONE_BYTE_ARG(OP_CALL, call);
      ONE_BYTE_ARG(OP_NATIVE, native);
      #undef ONE_BYTE_ARG

      #define JUMP(op, name) \
        case op: \
          strcpy(opString, #name); \
          i++; \
          sprintf( \
              argString, \
              "%i", \
              *((int16_t*)&(code->instructions.items[i])) \
          ); \
          i++; \
          break
      JUMP(OP_JUMP, jump);
      JUMP(OP_JUMP_IF_TRUE, jump_if_true);
      JUMP(OP_JUMP_IF_FALSE, jump_if_false);
      JUMP(OP_AND, and_jump);
      JUMP(OP_OR, or_jump);
      #undef JUMP

      #define MAP(op, name) \
      case op: \
        strcpy(opString, #name); \
        break
      MAP(OP_NIL, push_nil);
      MAP(OP_TRUE, push_true);
      MAP(OP_FALSE, push_false);
      MAP(OP_DROP, drop);

      MAP(OP_ADD, add);
      MAP(OP_SUBTRACT, sub);
      MAP(OP_MULTIPLY, mul);
        // This is "integer divide", i.e. 3 / 2 = 1
        // Distinguished from "divide", i.e. 3 / 2 = 1.5
      MAP(OP_DIVIDE, int_div);
      MAP(OP_NEGATE, neg);
      MAP(OP_NOT, not);
      MAP(OP_RETURN, ret);
      MAP(OP_EQ, eq);
      MAP(OP_LT, lt);
      MAP(OP_GT, gt);
      MAP(OP_NEQ, neq);
      MAP(OP_GEQ, geq);
      MAP(OP_LEQ, leq);
      MAP(OP_PROP, prop);
      #undef MAP

      default:
        printf("Number: %i\n", code->instructions.items[i]);
        fflush(stdout);
        assert(false);
    }

    printf("%-20s %-5s; inst: %zu, line:%zu\n", opString, argString, instructionIndex, line);
  }
}
