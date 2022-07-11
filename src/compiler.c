#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include "compiler.h"
#include "parser.h"

void Compiler_init(Compiler* self) {
}

void Compiler_free(Compiler* self) {
}

inline static void emitByte(Code* code, size_t line, uint8_t byte) {
  Code_append(code, byte, line);
}

inline static void emitInteger(Code* code, size_t line, int32_t integer) {
  /*
   * TODO If you trace what this does it's sort of a mess.
   *
   * However, I don't want to rewrite it now because this code may go
   * away;  we might remove the OP_INTEGER instruction in favor of an
   * OP_CONSTANT instruction which places the integer (or any other
   * constant) in a constant table on the Code object. There are some
   * tradeoffs here:
   *
   * PROS:
   * - Less space used in bytecode.
   * - We can deduplicate integers and use less space in the constant
   *   table as well.
   * CONS:
   * - Add a pointer lookup at runtime when pushing integers to the
   *   stack.
   *
   * Ultimately, let's wait to make a decision on this until we have
   * performance tests to be our guide.
   */

  uint8_t* bytes = (uint8_t*)(&integer);
  for(int i = 0; i < 4; i++) emitByte(code, line, bytes[i]);
}

static void emitNode(Code* code, Node* node) {
  switch(node->type) {
    case NODE_NIL:
      emitByte(code, node->line, (uint8_t)OP_NIL);
      return;
    case NODE_TRUE:
      emitByte(code, node->line, (uint8_t)OP_TRUE);
      return;
    case NODE_FALSE:
      emitByte(code, node->line, (uint8_t)OP_FALSE);
      return;

    case NODE_IDENTIFIER:
      assert(false); // TODO Implement
    case NODE_NUMBER:
      {
        int32_t number = 0;
        char* text = ((AtomNode*)node)->text;
        size_t length = ((AtomNode*)node)->length;

        for(size_t i = 0; i < length; i++) {
          uint8_t digit = text[i] - '0';
          assert(digit < 10);
          number = number * 10 + digit;
        }

        emitByte(code, node->line, (uint8_t)OP_INTEGER);
        emitInteger(code, node->line, number);
      }
      break;

    #define BINARY_NODE(op) \
      do { \
        emitNode(code, ((BinaryNode*)node)->arg0); \
        emitNode(code, ((BinaryNode*)node)->arg1); \
        emitByte(code, node->line, op); \
      } while(false)
    case NODE_ADD:      BINARY_NODE(OP_ADD);      return;
    case NODE_SUBTRACT: BINARY_NODE(OP_SUBTRACT); return;
    case NODE_MULTIPLY: BINARY_NODE(OP_MULTIPLY); return;
    case NODE_DIVIDE:   BINARY_NODE(OP_DIVIDE);   return;
    #undef BINARY_NODE

    default:
      assert(false);
  }
}

Code* Compiler_compile(Compiler* self, char* source) {
  Scanner scanner;
  Scanner_init(&scanner, source);
  Node* tree = parse(&scanner);

  Code* result = Code_new();

  emitNode(result, tree);

  /*
   * TODO We need to emit this because otherwise the runtime doesn't know
   * it's time to return. However, we don't want to require an explicit
   * return at the end of files, so there's no line associated with this
   * instruction. It probably makes sense for this to be the line of the
   * EOF in the file, but getting that requires some plumbing. Using 0
   * until I can get around to implementing this.
   */
  emitByte(result, 0, OP_RETURN);

  Node_free(tree);

  return result;
}
