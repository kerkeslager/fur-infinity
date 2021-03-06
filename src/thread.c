#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "code.h"
#include "memory.h"
#include "thread.h"
#include "value.h"

void FrameStack_init(FrameStack* self) {
  self->top = self->items;
}

void FrameStack_free(FrameStack* self) {
  self->top = self->items;
}

void FrameStack_push(FrameStack* self, Frame item) {
  // TODO Handle this.
  assert((self->top - self->items) < MAX_FRAMESTACK_DEPTH);

  *(self->top) = item;
  self->top++;
}

Frame FrameStack_pop(FrameStack* self) {
  assert(self->top > self->items);

  self->top--;
  return *(self->top);
}

void Stack_init(Stack* self) {
  self->top = self->items;
}

void Stack_free(Stack* self) {
  /*
   * This allows us to call GC afterward and have it read the stack as being
   * empty, without any special support in the GC.
   */
  self->top = self->items;
}

void Stack_push(Stack* self, Value item) {
  // TODO Handle this.
  assert((self->top - self->items) < MAX_STACK_DEPTH);

  *(self->top) = item;
  self->top++;
}

Value Stack_pop(Stack* self) {
  assert(self->top > self->items);

  self->top--;
  return *(self->top);
}

Value Stack_peek(Stack* self) {
  assert(self->top > self->items);

  return *(self->top - 1);
}

#ifdef DEBUG
void Stack_print(Stack* self, Value* fp) {
  printf("[");
  for(Value* v = self->items; v < self->top; v++) {
    if(v == fp) printf(" |");
    printf(" ");
    Value_printRepr(*v);
  }
  if(self->top == fp) printf(" |");
  printf(" ]");
}
#endif

void Stack_unary(Stack* self, Value (*unary)(Value)) {
  /*
   * TODO Seee notes on Stack_binary, this has similar properties.
   */
  assert(self->top > self->items);

  Value* ptr = self->top - 1;
  *ptr = unary(*ptr);
}

void Stack_binary(Stack* self, Value (*binary)(Value, Value)) {
  /*
   * A naive implementation of binary operations would do
   * Stack_push(&stack, binary(Stack_pop(&stack), Stack_pop(&stack)))
   *
   * If we trace that, we see that it decrements the top twice, then
   * increments it. This code is frequently called so we need efficiency here.
   *
   * Stack_binary(&stack, binary) has the same effect, with only one
   * decrement, without leaking the implementation of the stack out.
   *
   * TODO Let's profile to make sure the function pointer doesn't actually
   * *lose* us performance. Even if it does, we can reimplement this function
   * with the naive approach and then inline it.
   *
   * See also Stack_unary().
   */
  assert(self->top - 1 > self->items);

  self->top--;
  Value* ptr = self->top - 1;
  *ptr = binary(*ptr, *(self->top));
}

void Thread_init(Thread* self) {
  FrameStack_init(&(self->frames));
  Stack_init(&(self->stack));
  self->heap = NULL;
}

void Thread_free(Thread* self) {
  FrameStack_free(&(self->frames));
  Stack_free(&(self->stack));

  Obj* heap = self->heap;

  while(heap != NULL) {
    Obj* next = heap->next;
    Obj_free(heap);
    heap = next;
  }
}

void Thread_addToHeap(Thread* self, Obj* o) {
  /*
   * Obj_init sets o->next to NULL, and o->next should only be used by this
   * heap, so if it's not, that probably means that o is already in a
   * heap, which it should not be.
   */
  assert(o->next == NULL);

  o->next = self->heap;
  self->heap = o;
}

inline static Value logicalNot(Value arg) {
  assert(arg.is_a == TYPE_BOOLEAN);
  arg.as.boolean = !(arg.as.boolean);
  return arg;
}

inline static Value negate(Value arg) {
  assert(isInteger(arg));
  arg.as.integer = -arg.as.integer;
  return arg;
}

#define INT_BINARY_FUNCTION(name, op) \
  inline static Value name(Value arg0, Value arg1) { \
    assert(isInteger(arg0)); \
    assert(isInteger(arg1)); \
    arg0.as.integer = arg0.as.integer op arg1.as.integer; \
    return arg0; \
  }
INT_BINARY_FUNCTION(add, +)
INT_BINARY_FUNCTION(subtract, -)
INT_BINARY_FUNCTION(multiply, *)
INT_BINARY_FUNCTION(divide, /)
#undef OPERATOR_BINARY_FUNCTION

inline static Value concat(Value arg0, Value arg1) {
  assert(isObj(arg0));
  assert(isObj(arg1));
  assert(arg0.as.obj->type == OBJ_STRING);
  assert(arg1.as.obj->type == OBJ_STRING);

  ObjString* arg0s = (ObjString*)(arg0.as.obj);
  ObjString* arg1s = (ObjString*)(arg1.as.obj);

  size_t length = arg0s->length + arg1s->length;

  /* Add room for trailing null char */
  char* characters = allocateChars(length + 1);
  *characters = '\0';

  strncat(characters, arg0s->characters, arg0s->length);
  strncat(characters, arg1s->characters, arg1s->length);

  ObjString* s = ObjString_allocateOne();
  ObjString_init(s, length, characters);

  Value v;
  v.is_a = TYPE_OBJ;
  v.as.obj = (Obj*)s;

  return v;
}

#define ORDER_BINARY_FUNCTION(name, op) \
  inline static Value name(Value arg0, Value arg1) { \
    assert(isInteger(arg0)); \
    assert(isInteger(arg1)); \
    arg0.is_a = TYPE_BOOLEAN; \
    arg0.as.boolean = (arg0.as.integer op arg1.as.integer); \
    return arg0; \
  }
ORDER_BINARY_FUNCTION(lessThan, <)
ORDER_BINARY_FUNCTION(greaterThan, >)
ORDER_BINARY_FUNCTION(lessThanEquals, <=)
ORDER_BINARY_FUNCTION(greaterThanEquals, >=)
#undef ORDER_BINARY_FUNCTION

inline static Value equals(Value arg0, Value arg1) {
  switch(arg0.is_a) {
    case TYPE_NIL:
      return Value_fromBool(arg0.is_a == arg1.is_a);

    case TYPE_BOOLEAN:
      return Value_fromBool(
        arg1.is_a == TYPE_BOOLEAN &&
        arg0.as.boolean == arg1.as.boolean
      );

    case TYPE_INTEGER:
      return Value_fromBool(
        arg1.is_a == TYPE_INTEGER &&
        arg0.as.integer == arg1.as.integer
      );

    case TYPE_OBJ:
      return Value_fromBool(
          arg1.is_a == TYPE_OBJ &&
          Obj_equals(arg0.as.obj, arg1.as.obj)
        );

    default:
      assert(false);
  }
}

inline static Value notEquals(Value arg0, Value arg1) {
  /* TODO Are there ways to optimzie this? */
  return logicalNot(equals(arg0, arg1));
}

Value Thread_run(Thread* self, Code* code, size_t startIndex) {
  /*
   * TODO Wrap the outer level in a closure so we can write assertions against
   * the fact that we should always be within the bounds of the current code's
   * instructions.
   */
  ObjClosure* current = NULL;
  register uint8_t* ip = code->instructions.items + startIndex;
  register uint8_t instruction;
  Value* fp = self->stack.items; /* TODO Profile adding this to a register. */

  /*
   * TODO This is only used so we can assert ip is within the bounds--could be
   * cleaned up pretty significantly.
   */
  Code* rootCode = code;

  for(;;) {
    /*
     * TODO Profile different ways of putting index, instruction, and/or
     * a pointer to the instruction in code into a register.
     */
    instruction = *ip;

    #ifdef DEBUG
    Stack_print(&(self->stack), fp);
    printf(" ");
    Instruction_print(instruction);
    printf("\n");
    fflush(stdout);
    #endif

    /*
     * We increment the index *immediately* so we don't have to remember to
     * increment it in every single case statement.
     */
    ip++;


    switch(instruction) {
      case OP_GET:
        {
          uint8_t stackIndex = Code_getUInt8(code, ip);
          ip++;

          assert(fp + stackIndex >= self->stack.items);

          /*
           * If this fails, it means we either pointed our get instruction
           * beyond the edge of the stack, or we popped off our variable
           * when we shouldn't have. Either is a bug. It would be better to
           * catch the latter case closer to where it happens, but that's hard.
           * "Better late than never."
           */
          assert(fp + stackIndex < self->stack.top);

          Stack_push(&(self->stack), *(fp + stackIndex));
        } break;

      case OP_SET:
        {
          uint8_t stackIndex = Code_getUInt8(code, ip);
          ip++;

          assert(fp + stackIndex >= self->stack.items);
          assert(fp + stackIndex < self->stack.top);

          *(fp + stackIndex) = Stack_pop(&(self->stack));
        } break;

      case OP_NIL:
        {
          Value nil;
          nil.is_a = TYPE_NIL;
          Stack_push(&(self->stack), nil);
        } break;

      case OP_TRUE:
        Stack_push(&(self->stack), Value_fromBool(true));
        break;

      case OP_FALSE:
        Stack_push(&(self->stack), Value_fromBool(false));
        break;

      case OP_INTEGER:
        {
          Stack_push(
              &(self->stack),
              Value_fromInt32(Code_getInt32(code, ip))
          );

          ip += sizeof(int32_t);
        } break;

      case OP_INTERN:
        {
          Stack_push(
            &(self->stack),
            Value_fromObj(
              Code_getInterned(
                code,
                Code_getUInt8(code, ip)
              )
            )
          );
          /*
           * Note that we DO NOT put this into the heap. Once compiled,
           * interned strings exist as long as the program is running. They
           * may be accessed by multiple threads which run the code, and
           * should be treated as immutable. As such, we don't want them
           * garbage collected: they will be freed by Code_free()
           */
          ip++;
        } break;

      case OP_DROP:
        Stack_pop(&(self->stack));
        break;

      #define UNARY_OP(function) Stack_unary(&(self->stack), function)
      case OP_NEGATE: UNARY_OP(negate);       break;
      case OP_NOT:    UNARY_OP(logicalNot);   break;
      #undef UNARY_OP

      #define BINARY_OP(op, function)\
      case op: Stack_binary(&(self->stack), function); \
        break
      BINARY_OP(OP_SUBTRACT, subtract);
      BINARY_OP(OP_MULTIPLY, multiply);
      BINARY_OP(OP_DIVIDE, divide);
      BINARY_OP(OP_EQ, equals);
      BINARY_OP(OP_LT, lessThan);
      BINARY_OP(OP_GT, greaterThan);
      BINARY_OP(OP_NEQ, notEquals);
      BINARY_OP(OP_GEQ, greaterThanEquals);
      BINARY_OP(OP_LEQ, lessThanEquals);

      #undef BINARY_OP

      case OP_JUMP:
        {
          int16_t jump = Code_getInt16(code, ip);
          ip += jump;
          assert(ip <= code->instructions.items + code->instructions.length);
        } break;

      case OP_JUMP_IF_TRUE:
        {
          Value v = Stack_pop(&(self->stack));

          assert(v.is_a == TYPE_BOOLEAN);

          if(v.as.boolean) {
            int16_t jump = Code_getInt16(code, ip);
            ip += jump;
          } else {
            // Step over the space that contains the jump target
            ip += sizeof(int16_t);
          }

          assert(ip <= code->instructions.items + code->instructions.length);
        } break;

      case OP_JUMP_IF_FALSE:
        {
          Value v = Stack_pop(&(self->stack));

          assert(v.is_a == TYPE_BOOLEAN);

          if(!(v.as.boolean)) {
            int16_t jump = Code_getInt16(code, ip);
            ip += jump;
          } else {
            // Step over the space that contains the jump target
            ip += sizeof(int16_t);
          }

          assert(ip <= code->instructions.items + code->instructions.length);
        } break;

      /*
       * OP_AND and OP_OR are a bit complicated. They're actually
       * similar to OP_JUMP_IF_FALSE and OP_JUMP_IF_TRUE, respectively.
       * The way they differ is in how they treat the stack.
       *
       * OP_JUMP_IF_TRUE and OP_JUMP_IF_FALSE pop a value off the stack,
       * which is what you want when implementing if statements and loops.
       * If you left the tested item on the stack, you'd end up having to
       * emit an instruction at the beginning of each branch that drops
       * the excess value.
       *
       * However, `and` and `or` expressions return a value, which in Fur's
       * VM means their overall effect is to add an item to the stack.
       * They also test their left subexpression's result to determine
       * whether to jump past the right subexpression (short-circuiting).
       * Implementing this with OP_JUMP_IF_TRUE and OP_JUMP_IF_FALSE would
       * mean that we have to duplicate the result of the left subexpression,
       * so that the test doesn't lose it in the case that we jump. But then
       * we would have to drop the item we just duplicated in the case
       * that we *don't* jump.
       *
       * The traditional solution to this problem is to just have the
       * conditional jump instructions *peek* at the stack rather than
       * pop from it, and only pop when we decided to not branch. This
       * saves an instruction on average for `and` and `or` statements,
       * but for `if` statements and loops, it requires that we emit an
       * instruction to drop the top item of the stack for every branch or
       * iteration.
       *
       * Put another way, the first solution adds items to the stack we
       * might not need, and the second solution leaves items on the stack
       * which we might not need.
       *
       * OP_AND and OP_OR know whether we need the item they are testing
       * and act accordingly. Initially, they peek at the top of the stack
       * without changing it. If they top item is the value they jump
       * on, they *leave the value on the stack* and jump. If not, they
       * *drop the value from the stack* and continue on without jumping.
       *
       * At this time, this eliminates the need for OP_POP, OP_DROP, or
       * OP_DUP instructions.
       */
      case OP_AND:
        {
          Value v = Stack_peek(&(self->stack));

          assert(v.is_a == TYPE_BOOLEAN);

          if(!(v.as.boolean)) {
            int16_t jump = Code_getInt16(code, ip);
            ip += jump;
          } else {
            // Step over the space that contains the jump target
            ip += sizeof(int16_t);
            Stack_pop(&(self->stack));
          }

          assert(ip <= code->instructions.items + code->instructions.length);
        } break;

      /*
       * See comment above OP_AND
       */
      case OP_OR:
        {
          Value v = Stack_peek(&(self->stack));

          assert(v.is_a == TYPE_BOOLEAN);

          if(v.as.boolean) {
            int16_t jump = Code_getInt16(code, ip);
            ip += jump;
          } else {
            // Step over the space that contains the jump target
            ip += sizeof(int16_t);
            Stack_pop(&(self->stack));
          }

          assert(ip <= code->instructions.items + code->instructions.length);
        } break;

      /*
       * We can't use BINARY_OP for OP_ADD because it needs to handle
       * strings.
       */
      case OP_ADD:
        {
          switch(Stack_peek(&(self->stack)).is_a) {
            case TYPE_INTEGER:
              Stack_binary(&(self->stack), add);
              break;

            case TYPE_OBJ:
              {
                Stack_binary(&(self->stack), concat);
                Value top = Stack_peek(&(self->stack));

                // Add the concatenated string to the heap
                assert(isObj(top));
                assert(top.as.obj->type == OBJ_STRING);
                Thread_addToHeap(self, top.as.obj);
              } break;

            default:
              assert(false);
          }
        } break;

      case OP_CALL:
        {
          uint8_t argc = Code_getUInt8(code, ip);
          ip++;

          Value callee = Stack_pop(&(self->stack));
          assert(callee.is_a == TYPE_OBJ);

          switch(callee.as.obj->type) {
            case OBJ_CLOSURE:
              {
                ObjClosure* closure = (ObjClosure*)(callee.as.obj);
                assert(argc == closure->arity); /* TODO Handle this */

                Frame previous = {
                  .closure = current,
                  .ip = ip,
                  .fp = fp
                };

                FrameStack_push(&(self->frames), previous);

                assert(closure != NULL);
                current = closure;

                /*
                 * TODO This is only used in assertions and should probably be
                 * cleaned up.
                 */
                code = closure->code;

                assert(closure->code != NULL);
                assert(closure->code->instructions.items != NULL);
                ip = closure->code->instructions.items;

                fp = self->stack.top - argc;
                assert(fp <= self->stack.top);
                assert(fp >= self->stack.items);
              } break;

            case OBJ_NATIVE:
              {
                Value (*call)(uint8_t, Value*) = ((ObjNative*)(callee.as.obj))->call;

                /*
                 * We leave the arguments on the stack while the function is
                 * running so that they are considered live by the garbage
                 * collector.
                 */
                Value* argv = self->stack.top - argc;
                Value result = call(argc, self->stack.top - argc);
                *argv = result;
                self->stack.top = argv + 1;

                if(result.is_a == TYPE_OBJ) {
                  Thread_addToHeap(self, result.as.obj);
                }
              } break;

            default:
              assert(false);
          }
        } break;

      case OP_RETURN:
        {
          /*
           * TODO
           * We're leaving global-scoped items on the stack to enable the repl.
           * This is currently fine in modules, because we don't yet support imports
           * so when the module returns, its stack is immediately freed because the
           * thread ends. However, this will keep unnecessary items on the stack
           * when we have multiple modules.
           *
           * A better way to enable the REPL is to parse a single expression instead
           * of an expression list, and then compile it to a REPL-only OP that
           * simply pops off the top item and prints it. Then we can compile modules
           * properly and clean up properly in their returns.
           */
          if(current == NULL) return Stack_pop(&(self->stack));

          assert(fp >= self->stack.items);
          assert(fp < self->stack.top);

          *fp = Stack_peek(&(self->stack));
          self->stack.top = fp + 1;

          Frame previous = FrameStack_pop(&(self->frames));
          current = previous.closure;
          ip = previous.ip;
          fp = previous.fp;

          /*
           * TODO This is only used in assertions and should probably
           * be cleaned up.
           */
          if(previous.closure == NULL) {
            code = rootCode;
          } else {
            code = previous.closure->code;
          }
        } break;

      case OP_NATIVE:
        {
          ObjNative* n = ObjNative_allocateOne();
          ObjNative_init(n, NATIVE[Code_getUInt8(code, ip)].call);

          Value v;
          v.is_a = TYPE_OBJ;
          v.as.obj = (Obj*)n;

          Stack_push(&(self->stack), v);
          /* Add to heap AFTER adding to stack, to be sure it doesn't get GC'ed */
          Thread_addToHeap(self, (Obj*)n);

          ip++;
        } break;

      default:
        assert(false);
    }
  }
}
