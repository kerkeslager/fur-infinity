#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "object.h"
#include "code.h"
#include "compiler.h"
#include "parser.h"

void Symbol_init(Symbol* self, size_t length, char* name) {
  assert(length <= 255);
  self->length = length;
  self->name = malloc(length + 1);
  strncpy(self->name, name, length);
  self->name[length] = '\0';
}

void Symbol_free(Symbol* self) {
  free(self->name);
}

void SymbolStack_init(SymbolStack* self) {
  self->top = self->items;
}

void SymbolStack_free(SymbolStack* self) {
  for(Symbol* s = self->items; s < self->top; s++) {
    Symbol_free(s);
  }
  self->top = self->items;
}

void SymbolStack_push(SymbolStack* self, Symbol value) {
  // TODO Handle this.
  assert((self->top - self->items) < MAX_SYMBOLSTACK_DEPTH);

  *(self->top) = value;
  self->top++;
}

Symbol SymbolStack_pop(SymbolStack* self) {
  assert(self->top > self->items);

  self->top--;
  return *(self->top);
}

Symbol SymbolStack_peek(SymbolStack* self, uint8_t depth) {
  assert(self->top - depth > self->items);

  return *(self->top - 1 - depth);
}

void Compiler_init(Compiler* self) {
  SymbolStack_init(&(self->stack));
}

void Compiler_free(Compiler* self) {
  SymbolStack_free(&(self->stack));
}

inline static size_t emitByte(Code* code, size_t line, uint8_t byte) {
  return Code_append(code, byte, line);
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

size_t emitJump(Compiler* self, Code* code, size_t line, Instruction inst) {
  emitByte(code, line, (uint8_t)inst);

  assert(sizeof(uint8_t) * 2 == sizeof(int16_t));

  size_t result = emitByte(code, line, 0);
  emitByte(code, line, 0);
  return result;
}

void Compiler_patchJump(Compiler* self, Code* code, size_t jumpIndex, size_t targetIndex) {
  if(targetIndex - jumpIndex > INT16_MAX) {
    assert(false); // TODO Handle this
  }

  if((int16_t)targetIndex - (int16_t)jumpIndex < INT16_MIN) {
    /*
     * TODO Handle this--to be honest I'm not entirely sure the check
     * above is correct because the subtraction of the two size_ts is
     * probably wrapping around to a positive. But I don't think we're
     * emitting anything that would trigger this condition so I'm not
     * going to fix it at the moment.
     */
    assert(false);
  }

  int16_t jmp = (int16_t)targetIndex - (int16_t)jumpIndex;

  *((int16_t*)(&(code->instructions.items[jumpIndex]))) = jmp;
}

static size_t emitNode(Compiler* self, Code* code, Node* node) {
  switch(node->type) {
    case NODE_NIL:
      return emitByte(code, node->line, (uint8_t)OP_NIL);
    case NODE_TRUE:
      return emitByte(code, node->line, (uint8_t)OP_TRUE);
    case NODE_FALSE:
      return emitByte(code, node->line, (uint8_t)OP_FALSE);

    case NODE_IDENTIFIER:
      {
        /*
         * Similar to the NODE_ASSIGN, we have to look to see if the
         * *compiler* knows if the variable exists. If so, the location
         * on the compiler's symbol stack will be the same as the location
         * of the stored value on the thread's stack, read from the
         * bottom.
         */
        AtomNode* aNode = (AtomNode*)node;

        for(Symbol* s = self->stack.top - 1; s >= self->stack.items; s--) {
          if(s->length == aNode->length &&
              !strncmp(s->name, aNode->text, s->length)) {
            size_t result = emitByte(code, node->line, (uint8_t)OP_GET);
            emitByte(code, node->line, s - (self->stack.items));

            return result;
          }
        }

        /*
         * If we didn't find the variable on the compiler's stack, the
         * compiler hasn't seen it before.
         *
         * Note that this means we don't (yet!) support late bindings like
         * this:
         *
         * def getFoo():
         *   foo
         * end
         *
         * foo = 'Hello, world'
         */
        assert(false); // TODO Handle this
        return 0;
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

        size_t result = emitByte(code, node->line, (uint8_t)OP_INTEGER);
        emitInteger(code, node->line, number);
        return result;
      } break;

    case NODE_STRING:
      {
        uint8_t index = emitString(code, (AtomNode*)node);
        size_t result = emitByte(code, node->line, (uint8_t)OP_STRING);
        emitByte(code, node->line, index);
        return result;
      } break;

    #define UNARY_NODE(op) \
      do { \
        size_t result = emitNode(self, code, ((UnaryNode*)node)->arg); \
        emitByte(code, node->line, op); \
        return result; \
      } while(false)
    case NODE_NEGATE: UNARY_NODE(OP_NEGATE);
    case NODE_NOT:    UNARY_NODE(OP_NOT);
    #undef UNARY_NODE

    #define BINARY_NODE(type,op) \
    case type: \
      do { \
        size_t result = emitNode(self, code, ((BinaryNode*)node)->arg0); \
        emitNode(self, code, ((BinaryNode*)node)->arg1); \
        emitByte(code, node->line, op); \
        return result; \
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

    #undef BINARY_NODE

    case NODE_AND:
      {
        BinaryNode* bNode = (BinaryNode*)node;
        size_t result = emitNode(self, code, bNode->arg0);
        size_t toPatch = emitJump(self, code, node->line, OP_AND);
        emitNode(self, code, bNode->arg1);

        Compiler_patchJump(self, code, toPatch, code->instructions.length);
        return result;
      }

    case NODE_OR:
      {
        BinaryNode* bNode = (BinaryNode*)node;
        size_t result = emitNode(self, code, bNode->arg0);
        size_t toPatch = emitJump(self, code, node->line, OP_OR);
        emitNode(self, code, bNode->arg1);

        Compiler_patchJump(self, code, toPatch, code->instructions.length);
        return result;
      }

    case NODE_IF:
      {
        TernaryNode* tNode = (TernaryNode*)node;
        size_t result = emitNode(self, code, tNode->arg0);
        size_t patch0 = emitJump(self, code, node->line, OP_JUMP_IF_FALSE);
        emitNode(self, code, tNode->arg1);

        if(tNode->arg2 == NULL) {
          Compiler_patchJump(self, code, patch0, code->instructions.length);
          return result;
        } else {
          size_t patch1 = emitJump(self, code, node->line, OP_JUMP);
          emitNode(self, code, tNode->arg2);
          Compiler_patchJump(self, code, patch0, code->instructions.length);
          Compiler_patchJump(self, code, patch1, code->instructions.length);
          return result;
        }
      }

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
        size_t result = emitNode(self, code, bNode->arg1);

        AtomNode* targetNode = ((AtomNode*)(bNode->arg0));

        /*
         * We are looking to see if there is an existing "declaration"
         * for this variable, which would cause it to have a location on
         * the compiler stack. If it does, the location of the variable
         * on the thread stack is the same as the location of the symbol
         * on the compiler stack (when read from the bottom).
         */
        for(Symbol* s = self->stack.top - 1; s >= self->stack.items; s--) {
          if(s->length == targetNode->length &&
              !strncmp(s->name, targetNode->text, s->length)) {
            emitByte(code, node->line, (uint8_t)OP_SET);
            emitByte(code, node->line, s - (self->stack.items));

            /*
             * TODO We'd prefer not to emit a nil instruction after every
             * assignment, but the repl expects a return and if it pops,
             * the variable we just assigned now points off the end of the
             * stack.
             *
             * This feels like a hack, but the alternatives I have
             * thought of so far are even hackier, so I'm just going to
             * leave this until I have a more elegant solution.
             */
            emitByte(code, node->line, (uint8_t)OP_NIL);
            return result;
          }
        }

        /*
         * We only get here if the symbol is not on the stack, meaning
         * the compiler has not seen it before.
         *
         * This is the implicit "declaration" case of a variable. If the
         * variable has not been assigned before, we emit no instructions,
         * because the value we want to store in the variable is already
         * on the top of the stack, which is where we will store the
         * variable. However, we must record the stack location *in the
         * compiler* so that we can emit instructions that reference
         * this stack location by number.
         */
        assert(targetNode->length < 255);
        Symbol s;
        Symbol_init(
            &s,
            targetNode->length,
            targetNode->text
            );

        SymbolStack_push(&(self->stack), s);

        /*
         * TODO We'd prefer not to emit a nil instruction after every
         * assignment, but the repl expects a return and if it pops,
         * the variable we just assigned now points off the end of the
         * stack.
         *
         * This feels like a hack, but the alternatives I have
         * thought of so far are even hackier, so I'm just going to
         * leave this until I have a more elegant solution.
         *
         * This is *particularly* galling with declarations, because
         * if we don't have to emit this NIL, our declaration is an
         * extremely satisfying total of 0 instructions at runtime.
         */
        emitByte(code, node->line, (uint8_t)OP_NIL);
        return result;
      }

    default:
      assert(false);
  }
}

size_t Compiler_compile(Compiler* self, Code* code, Node* tree) {
  return emitNode(self, code, tree);
}
