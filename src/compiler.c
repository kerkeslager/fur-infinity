#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include "object.h"
#include "code.h"
#include "compiler.h"
#include "parser.h"

inline static void emitByte(Code* code, size_t line, uint8_t byte) {
  Code_append(code, byte, line);
}

inline static uint8_t emitString(Code* code, AtomNode* node) {
  char* characters = malloc(node->length - 1);
  assert(characters != NULL);

  size_t tokenIndex = 0;
  size_t charactersCount = 0;

  /*
   * Set tokenIndex to 1 to skip the opening quote in the lexeme.
   * Check against node->length - 1 to skip the closing quote in the lexeme.
   * Loop through each character and collapse escape sequences.
   */
  for(size_t tokenIndex = 1; tokenIndex < node->length - 1; tokenIndex++) {
    if(node->text[tokenIndex] == '\\') {
      tokenIndex++;

      switch(node->text[tokenIndex]) {
        case '\'':
        case '\"':
        case '\\':
          characters[charactersCount] = node->text[tokenIndex];
          break;

        case 'n':
          characters[charactersCount] = '\n';
          break;
        case 'r':
          characters[charactersCount] = '\r';
          break;
        case 't':
          characters[charactersCount] = '\t';
          break;

        default:
          // Unsupported escape sequences should have been caught by the
          // scanner.
          assert(false);
      }
    } else {
      characters[charactersCount] = node->text[tokenIndex];
    }

    charactersCount++;
  }

  /*
   * Even though we've got the length stored, let's append a null byte
   * for convenience and potentially safety (though we shouldn't rely on this).
   */
  characters[charactersCount] = '\0';

  /*
   * TODO If there were a lot of escape sequences, the string could be
   * significantly shorter than the lexeme, so it might be worthwhile
   * to realloc down. Let's revisit after Unicode support.
   */
  ObjString* result = malloc(sizeof(ObjString));

  ObjString_init(result, charactersCount, characters);

  return Code_internObject(code, (Obj*)result);
}

inline static uint8_t emitSymbol(Code* code, AtomNode* node) {
  // Length + space for null byte
  char* characters = malloc(node->length + 1);
  assert(characters != NULL);

  size_t tokenIndex = 0;
  size_t charactersCount = 0;

  /*
   * TODO This code was copied from emitString so it contains the loop
   * which is used for unescaping characters--not a concern in symbols.
   * We should probably use strncpy.
   */
  for(size_t tokenIndex = 0; tokenIndex < node->length - 1; tokenIndex++) {
    characters[charactersCount] = node->text[tokenIndex];
    charactersCount++;
  }

  /*
   * Even though we've got the length stored, let's append a null byte
   * for convenience and potentially safety (though we shouldn't rely on this).
   */
  characters[charactersCount] = '\0';

  ObjString* result = malloc(sizeof(ObjString));
  ObjString_init(result, charactersCount, characters);

  /*
   * TODO Consider interning symbols separately or under a different type,
   * since they have different performance concerns.
   */
  return Code_internObject(code, (Obj*)result);
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
   * - Add a pointer lookup at run time when pushing integers to the
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
      {
        /*
         * It's a bit unintuitive, but we actually must *emit* the symbol--
         * we can't read it from the symbol table, because the get
         * instruction may be in the code before the set instruction
         * in the code, for example:
         *
         * def get_a():
         *   a
         * end
         *
         * a = 1
         *
         * This would mean that expression `a` would not be in the symbol
         * table yet when trying to emit the OP_GET.
         */
        uint8_t index = emitSymbol(code, (AtomNode*)node);
        emitByte(code, node->line, (uint8_t)OP_GET);
        emitByte(code, node->line, index);
        return;
      }

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

    case NODE_STRING:
      {
        uint8_t index = emitString(code, (AtomNode*)node);
        emitByte(code, node->line, (uint8_t)OP_STRING);
        emitByte(code, node->line, index);
      } break;

    #define UNARY_NODE(op) \
      do { \
        emitNode(code, ((UnaryNode*)node)->arg); \
        emitByte(code, node->line, op); \
      } while(false)
    case NODE_NEGATE: UNARY_NODE(OP_NEGATE);  return;
    case NODE_NOT:    UNARY_NODE(OP_NOT);     return;
    #undef UNARY_NODE

    #define BINARY_NODE(type,op) \
    case type: \
      do { \
        emitNode(code, ((BinaryNode*)node)->arg0); \
        emitNode(code, ((BinaryNode*)node)->arg1); \
        emitByte(code, node->line, op); \
        return; \
      } while(false)
    BINARY_NODE(NODE_PROPERTY,            OP_PROP);
    BINARY_NODE(NODE_ADD,                 OP_ADD);
    BINARY_NODE(NODE_SUBTRACT,            OP_SUBTRACT);
    BINARY_NODE(NODE_MULTIPLY,            OP_MULTIPLY);
    BINARY_NODE(NODE_DIVIDE,              OP_DIVIDE);
    BINARY_NODE(NODE_EQUALS,              OP_EQ);
    BINARY_NODE(NODE_GREATER_THAN,        OP_GT);
    BINARY_NODE(NODE_LESS_THAN,           OP_LT);
    BINARY_NODE(NODE_NOT_EQUALS,          OP_NEQ);
    BINARY_NODE(NODE_GREATER_THAN_EQUALS, OP_GEQ);
    BINARY_NODE(NODE_LESS_THAN_EQUALS,    OP_LEQ);

    // TODO Implement short-circuiting
    BINARY_NODE(NODE_AND,                 OP_AND);
    BINARY_NODE(NODE_OR,                  OP_OR);
    #undef BINARY_NODE

    case NODE_ASSIGN:
      {
        BinaryNode* bNode = (BinaryNode*)node;

        /*
         * TODO This doesn't support a lot of expressions, like
         * foo.bar = baz, but it's better to fail fast for now until
         * we can implement those.
         */
        assert(bNode->arg0->type == NODE_IDENTIFIER);

        /*
         * TODO This makes it urgent that we deduplicate interned strings,
         * since we are interning a separate string for every variable
         * and many variables will have the same name. We also want symbol
         * equality to be fast, so making interned strings references equal
         * would be a big win.
         */
        /*
         * TODO Consider storing variables in a separate symbol table,
         * as they have separate performance concerns from strings.
         */
        emitNode(code, bNode->arg1);
        uint8_t index = emitSymbol(code, ((AtomNode*)bNode->arg0));
        emitByte(code, node->line, (uint8_t)OP_SET);
        emitByte(code, node->line, index);
        return;
      }

    default:
      assert(false);
  }
}

/*
 * TODO Change the name of `result` to `code`. The reasons for the
 * previous name no longer apply.
 */
size_t compile(Code* result, char* source) {
  // TODO We should probably move scanning and parsing out of this
  // function.
  Scanner scanner;
  Scanner_init(&scanner, 1, source);
  Node* tree = parse(&scanner);

  size_t startOfEmittedCode = result->instructions.length;

  /*
   * TODO We should probably start encapsulating operations on code
   * instead of mucking around with its guts directly from the compile
   * function.
   */
  emitNode(result, tree);

  /*
   * TODO We need to emit this because otherwise the thread doesn't know
   * it's time to return. However, we don't want to require an explicit
   * return at the end of files, so there's no line associated with this
   * instruction. It probably makes sense for this to be the line of the
   * EOF in the file, but getting that requires some plumbing. Using 0
   * until I can get around to implementing this.
   */
  emitByte(result, 0, OP_RETURN);

  Node_free(tree);

  return startOfEmittedCode;
}
