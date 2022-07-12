#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "thread.h"
#include "value.h"

void Stack_init(Stack* self) {
  self->top = self->items;
}

void Stack_free(Stack* self) {
  /*
   * This function is a placeholder. Eventually we'll be pointing to heap-
   * allocated items on the stack, so we'll want to free those. This is just
   * here so we can call this in the appropriate spots and implement this
   * when needed.
   */
}

void printStack(Stack* stack) {
  printf("Count: %zu [", stack->top - stack->items);
  for(Value* v = stack->items; v < stack->top; v++) {
    printf(" ");
    Value_printRepr(*v);
  }
  printf(" ]\n");
}

void Stack_push(Stack* self, Value value) {
  // TODO Handle this.
  assert((self->top - self->items) < MAX_STACK_DEPTH);

  *(self->top) = value;
  self->top++;
}

Value Stack_pop(Stack* self) {
  assert(self->top > self->items);

  self->top--;
  Value result = *(self->top);
  return result;
}

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
  Stack_init(&(self->stack));
  self->heap = NULL;
}

void Thread_free(Thread* self) {
  Stack_free(&(self->stack));
  assert(self->heap == NULL);
}

inline static Value logicalNot(Value arg) {
  switch(arg.is_a) {
    case TYPE_TRUE:
      arg.is_a = TYPE_FALSE;
      return arg;
    case TYPE_FALSE:
      arg.is_a = TYPE_TRUE;
      return arg;
    default:
      assert(false);
  }
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

#define ORDER_BINARY_FUNCTION(name, op) \
  inline static Value name(Value arg0, Value arg1) { \
    assert(isInteger(arg0)); \
    assert(isInteger(arg1)); \
    arg0.is_a = (arg0.as.integer op arg1.as.integer) ? TYPE_TRUE : TYPE_FALSE; \
    return arg0; \
  }
ORDER_BINARY_FUNCTION(lessThan, <)
ORDER_BINARY_FUNCTION(greaterThan, >)
ORDER_BINARY_FUNCTION(lessThanEquals, <=)
ORDER_BINARY_FUNCTION(greaterThanEquals, >=)
#undef ORDER_BINARY_FUNCTION

// TODO Implement short-circuiting.
#define LOGICAL_BINARY_FUNCTION(name, op) \
  inline static Value name(Value arg0, Value arg1) { \
    assert(isBoolean(arg0)); \
    assert(isBoolean(arg1)); \
    return Value_fromBool(Value_toBool(arg0) op Value_toBool(arg1)); \
  }
LOGICAL_BINARY_FUNCTION(logicalAnd, &&);
LOGICAL_BINARY_FUNCTION(logicalOr, ||);
#undef LOGICAL_BINARY_FUNCTION

inline static Value equals(Value arg0, Value arg1) {
  switch(arg0.is_a) {
    case TYPE_NIL:
    case TYPE_TRUE:
    case TYPE_FALSE:
      return Value_fromBool(arg0.is_a == arg1.is_a);

    case TYPE_INTEGER:
      return Value_fromBool(
        arg0.is_a == arg1.is_a && arg0.as.integer == arg1.as.integer
      );

    default:
      assert(false);
  }
}
inline static Value notEquals(Value arg0, Value arg1) {
  if(arg0.is_a != arg1.is_a) {
    return Value_fromBool(true);
  }

  switch(arg0.is_a) {
    case TYPE_INTEGER:
      return Value_fromBool(arg0.as.integer != arg1.as.integer);

    default:
      return Value_fromBool(false);
  }
}

Value Thread_run(Thread* self, Code* code) {
  size_t index = 0;

  for(;;) {
    assert(index < code->instructions.length);

    switch(Code_get(code, index)) {
      case OP_NIL:
        Stack_push(&(self->stack), VALUE_NIL);
        index++;
        break;

      case OP_TRUE:
        Stack_push(&(self->stack), Value_fromBool(true));
        index++;
        break;

      case OP_FALSE:
        Stack_push(&(self->stack), Value_fromBool(false));
        index++;
        break;

      case OP_INTEGER:
        {
          index++;
          Stack_push(
              &(self->stack),
              Value_fromInt32(Code_getInteger(code, index))
          );

          index += sizeof(int32_t);
        } break;

      case OP_STRING:
        {
          index++;
          Stack_push(
            &(self->stack),
            Value_fromObj(
              Code_getInterned(
                code,
                Code_get(code, index)
              )
            )
          );
          /*
           * Note that we DO NOT put this into the heap.
           * Once compiled, interned strings exist as long as the
           * program is running. They may be accessed by multiple
           * threads which run the code, and should be treated as immutable.
           * As such we don't want them garbage collected: they will be
           * freed by the runtime when it frees the code.
           */
          index++;
        } break;

      #define UNARY_OP(function) Stack_unary(&(self->stack), function)
      case OP_NEGATE: UNARY_OP(negate);       index++;  break;
      case OP_NOT:    UNARY_OP(logicalNot);   index++;  break;
      #undef UNARY_OP


      #define BINARY_OP(op, function)\
      case op: Stack_binary(&(self->stack), function); \
        index++; \
        break
      BINARY_OP(OP_ADD, add);
      BINARY_OP(OP_SUBTRACT, subtract);
      BINARY_OP(OP_MULTIPLY, multiply);
      BINARY_OP(OP_DIVIDE, divide);
      BINARY_OP(OP_EQ, equals);
      BINARY_OP(OP_LT, lessThan);
      BINARY_OP(OP_GT, greaterThan);
      BINARY_OP(OP_NEQ, notEquals);
      BINARY_OP(OP_GEQ, greaterThanEquals);
      BINARY_OP(OP_LEQ, lessThanEquals);

      // TODO Implement short-circuiting
      BINARY_OP(OP_AND, logicalAnd);
      BINARY_OP(OP_OR, logicalOr);
      #undef BINARY_OP

      case OP_RETURN:
        {
          Value result;
          result = Stack_pop(&(self->stack));
          // TODO Free values.
          index++;
          return result;
        }

      default:
        assert(false);
    }
  }
}
