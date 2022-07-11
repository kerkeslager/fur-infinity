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

static void emitBytes(Code* code, size_t line, int argc, ...) {
  va_list bytes;
  va_start(bytes, argc);

  for(int i = 0; i < argc; i++) {
    emitByte(code, (uint8_t)(va_arg(bytes, int)), line);
  }
}

static void emitNode(Code* code, Node* node) {
  switch(node->type) {
    case NODE_NIL:
      emitByte(code, 0, (uint8_t)OP_NIL);
      break;
    case NODE_TRUE:
      emitByte(code, 0, (uint8_t)OP_TRUE);
      break;
    case NODE_FALSE:
      emitByte(code, 0, (uint8_t)OP_FALSE);
      break;

    case NODE_IDENTIFIER:
      assert(false); // TODO Implement
    case NODE_NUMBER:
      {
        uint32_t number = 0;
        char* text = ((AtomNode*)node)->text;
        size_t length = ((AtomNode*)node)->length;

        sscanf(text, "%d", &number);

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
        emitBytes(
          code,
          node->line,
          OP_INTEGER,
          ((uint8_t*)(&number))[0],
          ((uint8_t*)(&number))[1],
          ((uint8_t*)(&number))[2],
          ((uint8_t*)(&number))[3]
        );
      }
      break;

    #define BINARY_NODE(op) \
      do { \
        emitNode(code, ((BinaryNode*)node)->arg0); \
        emitNode(code, ((BinaryNode*)node)->arg1); \
        emitByte(code, node->line, op); \
      } while(true)
    case NODE_ADD:      BINARY_NODE(OP_ADD);      break;
    case NODE_SUBTRACT: BINARY_NODE(OP_SUBTRACT); break;
    case NODE_MULTIPLY: BINARY_NODE(OP_MULTIPLY); break;
    case NODE_DIVIDE:   BINARY_NODE(OP_DIVIDE);   break;
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

  return result;
}
