#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "object.h"
#include "code.h"
#include "compiler.h"
#include "memory.h"
#include "parser.h"

static size_t emitNode(Compiler* self, Code* code, Node* node, bool emitReturn);

void SymbolStack_init(SymbolStack* self) {
  self->top = self->items;
}

void SymbolStack_free(SymbolStack* self) {
}

void SymbolStack_push(SymbolStack* self, Symbol* value) {
  // TODO Handle this.
  assert((self->top - self->items) < MAX_SYMBOLSTACK_DEPTH);

  *(self->top) = value;
  self->top++;
}

Symbol* SymbolStack_pop(SymbolStack* self) {
  assert(self->top > self->items);

  self->top--;
  return *(self->top);
}

Symbol* SymbolStack_peek(SymbolStack* self, uint8_t depth) {
  assert(self->top - depth > self->items);

  return *(self->top - 1 - depth);
}

inline static int16_t SymbolStack_findSymbol(SymbolStack* self, Symbol* symbol) {
  for(Symbol** s = self->top - 1; s >= self->items; s--) {
    if(*s == symbol) {
      return (int16_t)(s - self->items);
    }
  }
  return -1;
}

void Compiler_init(Compiler* self, Runtime* runtime) {
  SymbolStack_init(&(self->stack));
  self->runtime = runtime;
  self->scope = 0;
}

void Compiler_free(Compiler* self) {
  SymbolStack_free(&(self->stack));
}

Symbol* Compiler_getSymbol(Compiler* self, size_t length, char* name) {
  return Runtime_getSymbol(self->runtime, length, name);
}

inline static size_t emitByte(Code* code, size_t line, uint8_t byte) {
  return Code_append(code, byte, line);
}

inline static size_t emitInstruction(Code* code, size_t line, Instruction i) {
  /*
   * This function basically exists to have this assertion.
   *
   * We want to catch if we add too many instructions so that we no longer
   * can fit them all in a uint8_t, which is the basis of our bytecode
   * structure. We're just casting Instruction into uint8_t, so this could
   * cause us mysterious bugs. But we also don't want to add assertions or
   * whatnot everywhere.
   *
   * In most mainstream compilers, the enum Instruction is emitted as a
   * 4-byte integer regardless of how many possible values there are.
   * So we can't just test sizeof(Instruction) <= sizeof(uint8_t), which
   * is what we really want to assert.
   *
   * Instead, if we use this function ubiquitously, we'll get this assertion
   * when we emit the first instruction that goes over the 1-byte limit.
   *
   * We're wrapping emitByte and keeping it around for the case where
   * we are actually pushing in a uint8_t, not an Instruction that needs
   * to be cast.
   */
  assert(i <= UINT8_MAX);

  return emitByte(code, line, (uint8_t)i);
}

inline static Obj* makeObjClosure(Compiler* self, Symbol* name, Node* arguments, Node* body) {
  /* Pretty unlikely we go this deep, but it doesn't hurt to check it. */
  assert(self->scope < SIZE_MAX);
  self->scope++;

  /*
   * TODO Allow name == NULL, which we will need for lambdas.
   */
  assert(name != NULL);

  /*
   * If arguments == NULL, there are no arguments.
   * If arguments->type == NODE_IDENTIFIER, there is one argument.
   * If arguments->type == NODE_COMMA_SEPARATED_LIST, there are multiple arguments.
   */
  assert(arguments == NULL ||
      arguments->type == NODE_IDENTIFIER ||
      arguments->type == NODE_COMMA_SEPARATED_LIST);

  /*
   * TODO For now we only support no-argument functions.
   */
  assert(arguments == NULL);

  if(body == NULL) {
    assert(false); /* TODO Allow empty function bodies (just return nil). */
  }

  /*
   * TODO Code_init does a few things which aren't great for functions.
   * Most notably, code is maintaining a separate list of interned
   * strings, which should probably be shared by the whole runtime.
   */
  Code* functionCode = Code_allocateOne();
  Code_init(functionCode);

  emitNode(self, functionCode, body, true);

  /* TODO This line number isn't really right */
  emitInstruction(functionCode, body->line, OP_RETURN);
  self->scope--;

  ObjClosure* result = ObjClosure_allocateOne();
  ObjClosure_init(result, name, 0, functionCode);
  return (Obj*)result;
}

inline static Obj* makeObjString(AtomNode* node) {
  char* characters = allocateChars(node->length - 1);

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
  ObjString* result = ObjString_allocate(1);

  ObjString_init(result, charactersCount, characters);

  return (Obj*)result;
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
  emitInstruction(code, line, inst);

  assert(sizeof(uint8_t) * 2 == sizeof(int16_t));

  size_t result = emitByte(code, line, 0);
  emitByte(code, line, 0);
  return result;
}

/* TODO This should probably be Code_patchJump */
void Compiler_patchJump(Code* code, size_t jumpIndex, size_t targetIndex) {
  // TODO Handle these
  assert(jumpIndex <= INT32_MAX);
  assert(targetIndex <= INT32_MAX);

  /*
   * Cast into signed integers so that the result of out subtraction is
   * signed, i.e. jumpIndex can be greater than targetIndex.
   */
  int32_t intJumpIndex = (int32_t)jumpIndex;
  int32_t intTargetIndex = (int32_t)targetIndex;

  int32_t jump = intTargetIndex - intJumpIndex;

  /*
   * Now that we have cast into sign, let't catch any overflows and then
   * cast into the right size.
   */
  // TODO Handle these.
  assert(jump <= INT16_MAX);
  assert(jump >= INT16_MIN);

  *((int16_t*)(&(code->instructions.items[jumpIndex]))) = (int16_t)jump;
}

/* TODO This should probably be Code_patchJumpToCurrent */
void Compiler_patchJumpToCurrent(Code* code, size_t jumpIndex) {
  Compiler_patchJump(code, jumpIndex, code->instructions.length);
}

inline static size_t emitBasic(Code* code, size_t line, Instruction i, bool emitReturn) {
  if(emitReturn) {
    return emitInstruction(code, line, i);
  } else {
    return Code_getCurrent(code);
  }
}

/*
 * TODO This shoudl take a Symbol* rather than length and name. That Symbol*
 * should be from the Symbol Table and guaranteed to be pointer-comparable.
 */
void emitAssignment(Compiler* self, Code* code, bool allowReassignment, size_t line, Symbol* name) {
  /*
   * We are looking to see if there is an existing "declaration"
   * for this variable, which would cause it to have a location on
   * the compiler stack. If it does, the location of the variable
   * on the thread stack is the same as the location of the symbol
   * on the compiler stack (when read from the bottom).
   */
  int16_t index = SymbolStack_findSymbol(&(self->stack), name);

  if(index > -1) {
    assert(allowReassignment);
    emitInstruction(code, line, OP_SET);
    emitByte(code, line, index);
    return;
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
  SymbolStack_push(&(self->stack), name);
}

/*
 * emitReturn tells us whether the node should return a value by placing the
 * item on the stack. This allows us to perform an optimization.
 *
 * In Fur semantics, every statement is an expression, meaning that it returns a
 * value by placing it on the stack. When compiling a function like this:
 *
 *     def foo():
 *       <expression A>
 *       <expression B>
 *     end
 *
 * <expression A> returns a value, but that value is isn't used, and we can't
 * leave it on the stack, * whereas <expression B returns a value which is
 * then returned by calls to foo().
 *
 * From the persepective of the parent function, the easiest way to deal with
 * this is to emit an OP_DROP after <expression A>, but there are many cases
 * where that is inefficient. For example:
 *
 *     def foo():
 *       a = 1
 *       <expression B>
 *     end
 *
 * The "expression" `a = 1` is actually more of a *statement*. If we're
 * treating it as an expression, then it has to return a value (we do that by
 * emitting a OP_NIL, when it's necessary). But if we use the naive
 * implementation, this means we're emitting an OP_NIL followed by OP_DROP,
 * which is obviously not ideal.
 *
 * Passing emitReturn=false to the function compiling `a = 1` lets the function
 * know whether or not the return is going to be used. If it's not going to be
 * used, we can just not emit the OP_NIL, but in cases where we *do* use the
 * return value, we can pass in emitReturn=true. There are some cases where
 * we will still need to emit an OP_DROP, for example this:
 *
 *     def foo():
 *       bar()
 *       <expression B>
 *     end
 *
 * We can't necessarily prevent bar() from returning a value by placing it on
 * the stack, because calls to bar() in other parts of the code might actually
 * use the return value. So when we're compiling the expression `bar()`, we
 * still have to emit an `OP_DROP` afterward if we receive emitReturn=false.
 * But at least in cases where the expression doesn't have a natural return,
 * we can avoid placing anything on the stack in the first place if it isn't
 * going to be used.
 */
static size_t emitNode(Compiler* self, Code* code, Node* node, bool emitReturn) {
  switch(node->type) {
    case NODE_NIL:
      return emitBasic(code, node->line, OP_NIL, emitReturn);

    case NODE_TRUE:
      return emitBasic(code, node->line, OP_TRUE, emitReturn);

    case NODE_FALSE:
      return emitBasic(code, node->line, OP_FALSE, emitReturn);

    case NODE_IDENTIFIER:
      {
        /*
         * Currently, accessing an identifier does not have any side effects,
         * so we can do this. However, special idenfitiers might change
         * this in the future, so we need to be careful.
         */
        if(!emitReturn) return Code_getCurrent(code);

        /*
         * Similar to the NODE_ASSIGN, we have to look to see if the
         * *compiler* knows if the variable exists. If so, the location
         * on the compiler's symbol stack will be the same as the location
         * of the stored value on the thread's stack, read from the
         * top.
         */
        AtomNode* aNode = (AtomNode*)node;
        Symbol* name = Compiler_getSymbol(self, aNode->length, aNode->text);

        int16_t index = SymbolStack_findSymbol(&(self->stack), name);

        if(index > -1) {
          size_t result = emitInstruction(code, node->line, OP_GET);
          emitByte(code, node->line, (uint8_t)index);
          return result;
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
         *
         * Next we try the builtins. Since user defined variables are tried
         * first, this means it's possible to override built in functions.
         */
        /*
         * TODO Think about this more. I can't think of a way that loading
         * natives "late" like this bites us, but it's bothering my
         * intuition, so maybe there's some case my subconscious has thought
         * of where it goes wrong.
         *
         * A more foolproof way is to simply preload natives onto the
         * stack like locals, but currently our locals stack depth tops out
         * at UINT8_MAX, and adding natives below that would necessitate
         * expanding the stack to UINT16_MAX, meaning larger get/set
         * instructions. Python, for example, has 71 (documented) builtins
         * at the time of this writing, which would leave only 255-71 = 184
         * slots for locals.
         */
        for(size_t i = 0; i < NATIVE_COUNT; i++) {
          if(!strncmp(NATIVE[i].name, aNode->text, aNode->length)) {
            size_t result = emitInstruction(code, node->line, OP_NATIVE);

            /*
             * We can only have UINT8_MAX native functions built in at the
             * top level (we should namespace others in builtin modules).
             */
            assert(i <= UINT8_MAX);
            emitByte(code, node->line, (uint8_t)i);

            return result;
          }
        }

        char buffer[aNode->length + 1];
        snprintf(buffer, aNode->length, "%s", aNode->text);
        buffer[aNode->length] = '\0';
        printf("Unknown identifier \"%s\"\n", buffer);
        fflush(stdout);
        assert(false); // TODO Handle this
      }

    case NODE_NUMBER:
      {
        if(!emitReturn) return Code_getCurrent(code);

        int32_t number = 0;
        char* text = ((AtomNode*)node)->text;
        size_t length = ((AtomNode*)node)->length;

        for(size_t i = 0; i < length; i++) {
          uint8_t digit = text[i] - '0';
          assert(digit < 10);
          number = number * 10 + digit;
        }

        size_t result = emitInstruction(code, node->line, OP_INTEGER);
        emitInteger(code, node->line, number);
        return result;
      } break;

    case NODE_STRING:
      {
        if(!emitReturn) return Code_getCurrent(code);

        Obj* obj = makeObjString((AtomNode*)node);

        uint8_t index = Code_internObject(code, (Obj*)obj);
        size_t result = emitInstruction(code, node->line, OP_STRING);
        emitByte(code, node->line, index);
        return result;
      } break;

    #define UNARY_NODE(op) \
      do { \
        size_t result = emitNode( \
            self, \
            code, \
            ((UnaryNode*)node)->arg, \
            emitReturn \
          ); \
        if(emitReturn) emitByte(code, node->line, op); \
        return result; \
      } while(false)
    case NODE_NEGATE: UNARY_NODE(OP_NEGATE);
    case NODE_NOT:    UNARY_NODE(OP_NOT);
    #undef UNARY_NODE

    #define BINARY_NODE(type,op) \
    case type: \
      do { \
        size_t result = emitNode( \
            self, \
            code, \
            ((BinaryNode*)node)->arg0, \
            emitReturn \
          ); \
        emitNode(self, code, ((BinaryNode*)node)->arg1, emitReturn); \
        if(emitReturn) emitByte(code, node->line, op); \
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
        size_t result = emitNode(self, code, bNode->arg0, true);
        size_t toPatch = emitJump(self, code, node->line, OP_AND);
        emitNode(self, code, bNode->arg1, true);

        Compiler_patchJumpToCurrent(code, toPatch);

        /*
         * TODO It isn't obvious how to short circuit, respect potential
         * side effects, and not emit this instruction. This probably isn't
         * a high priority optimizatin, however, since this isn't the
         * usual case.
         */
        if(!emitReturn) emitInstruction(code, node->line, OP_DROP);

        return result;
      }

    case NODE_OR:
      {
        BinaryNode* bNode = (BinaryNode*)node;
        size_t result = emitNode(self, code, bNode->arg0, true);
        size_t toPatch = emitJump(self, code, node->line, OP_OR);
        emitNode(self, code, bNode->arg1, true);

        Compiler_patchJumpToCurrent(code, toPatch);

        /*
         * TODO It isn't obvious how to short circuit, respect potential
         * side effects, and not emit this instruction. This probably isn't
         * a high priority optimizatin, however, since this isn't the
         * usual case.
         */
        if(!emitReturn) emitInstruction(code, node->line, OP_DROP);

        return result;
      }

    case NODE_IF:
      {
        TernaryNode* tNode = (TernaryNode*)node;
        size_t result = emitNode(self, code, tNode->arg0, true);
        size_t patch0 = emitJump(self, code, node->line, OP_JUMP_IF_FALSE);
        emitNode(self, code, tNode->arg1, emitReturn);

        size_t patch1 = emitJump(self, code, node->line, OP_JUMP);
        Compiler_patchJumpToCurrent(code, patch0);

        /*
         * TODO If the else branch doesn't emit any instructions, we don't
         * need a jump to get past it.
         */

        if(tNode->arg2 == NULL) {
          if(emitReturn) emitInstruction(code, node->line, OP_NIL);
        } else {
          emitNode(self, code, tNode->arg2, emitReturn);
        }
        Compiler_patchJumpToCurrent(code, patch1);
        return result;
      }

    case NODE_WHILE:
      {
        BinaryNode* bNode = (BinaryNode*)node;

        size_t result = emitNode(self, code, bNode->arg0, true);
        size_t patch0 = emitJump(self, code, node->line, OP_JUMP_IF_FALSE);

        emitNode(self, code, bNode->arg1, false);

        /*
         * Jump back to the beginning of the loop.
         */
        /*
         * TODO Add a function so that we don't have to patch the jump when
         * we already know where we're jumping to.
         */
        size_t patch1 = emitJump(self, code, node->line, OP_JUMP);
        Compiler_patchJump(code, patch1, result);

        Compiler_patchJumpToCurrent(code, patch0);

        if(emitReturn) emitInstruction(code, node->line, OP_NIL);
        return result;
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
         * TODO Consider storing variables in a separate symbol table,
         * as they have separate performance concerns from strings.
         */
        size_t result = emitNode(self, code, bNode->arg1, true);

        AtomNode* targetNode = ((AtomNode*)(bNode->arg0));
        Symbol* targetSymbol = Compiler_getSymbol(
            self,
            targetNode->length,
            targetNode->text
          );

        emitAssignment(
            self,
            code,
            true, /* Allow reassignment */
            node->line,
            targetSymbol
          );

        if(emitReturn) emitInstruction(code, node->line, OP_NIL);

        return result;
      }

    case NODE_EXPRESSION_LIST:
      {
        ExpressionListNode* elNode = (ExpressionListNode*)node;

        if(elNode->length == 0 && emitReturn) {
          return emitInstruction(code, node->line, OP_NIL);
        }

        if(elNode->length == 1) {
          return emitNode(self, code, elNode->items[0], emitReturn);
        }

        /*
         * Emit the first node outside the loop, so we can record
         * where it is and return it.
         */
        size_t result = emitNode(self, code, elNode->items[0], false);

        for(size_t i = 1; i < elNode->length - 1; i++) {
          /*
           * emitReturn is always false for these, since we are discarding
           * the results anyway.
           */
          emitNode(self, code, elNode->items[i], false);
        }

        /*
         * Emit the last node outside the loop, so we can return its value
         * if necessary.
         */
        emitNode(self, code, elNode->items[elNode->length - 1], emitReturn);

        return result;
      }

    case NODE_CALL:
      {
        BinaryNode* bNode = (BinaryNode*)node;

        assert(bNode->arg1->type == NODE_COMMA_SEPARATED_LIST);

        Node* callee = bNode->arg0;
        ExpressionListNode* arguments = (ExpressionListNode*)(bNode->arg1);

        /*
         * We support at most UINT8_MAX arguments to a function.
         */
        assert(arguments->length <= UINT8_MAX); // TODO Handle this

        size_t result;

        if(arguments->length > 0) {
          /*
           * If there are any arguments, emit the first argument outside the
           * loop so we can capture where it's emitted to.
           */
          result = emitNode(self, code, arguments->items[0], true);

          for(size_t i = 1; i < arguments->length; i++) {
            emitNode(self, code, arguments->items[i], true);
          }

          emitNode(self, code, callee, true);
        } else {
          /*
           * If there are not any arguments, we capture the emitted callee
           * location instead.
           */
          result = emitNode(self, code, callee, true);
        }

        emitInstruction(code, node->line, OP_CALL);
        emitByte(code, node->line, (uint8_t)arguments->length);

        /*
         * Function calls always emit a return and it's difficult to change
         * that without adding types.
         */
        if(!emitReturn) emitInstruction(code, node->line, OP_DROP);

        return result;
      }

    case NODE_FN_DEF:
      {
        Node* nameNode = ((TernaryNode*)node)->arg0;
        assert(nameNode->type == NODE_IDENTIFIER);
        Symbol* name = Compiler_getSymbol(
            self,
            ((AtomNode*)nameNode)->length,
            ((AtomNode*)nameNode)->text
          );

        Obj* obj = makeObjClosure(
          self,
          name,
          ((TernaryNode*)node)->arg1,
          ((TernaryNode*)node)->arg2
        );

        uint8_t index = Code_internObject(code, obj);

        /* TODO Obviously OP_STRING should be OP_OBJ now */
        size_t result = emitInstruction(code, node->line, OP_STRING);
        emitByte(code, node->line, index);

        emitAssignment(
            self,
            code,
            false, /* Don't allow reassignment */
            node->line,
            name);

        if(emitReturn) emitInstruction(code, node->line, OP_NIL);

        return result;
      }

    default:
      assert(false);
  }
}

size_t Compiler_compile(Compiler* self, Code* code, Node* tree) {
  size_t result =  emitNode(self, code, tree, true);

  /* TODO This fixes the integration tests but probably broke the repl */
  emitInstruction(code, tree->line, OP_RETURN);

  return result;
}
