#include <assert.h>
#include <stdbool.h>

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

void Stack_push(Stack* self, Value value) {
  // TODO Handle this.
  assert((self->top - self->items) < MAX_STACK_DEPTH);

  *(self->top) = value;
  self->top++;
}

Value Stack_pop(Stack* self) {
  assert(self->top > self->items);

  Value result = *(self->top);
  self->top--;
  return result;
}

void Stack_binary(Stack* self, Value (*binary)(Value, Value)) {
  /*
   * A naive implementation of binary operations would do
   * Stack_push(&stack, binary(Stack_pop(&stack), Stack_pop(&stack)))
   *
   * If we trace that, we see that it decrements the top twice, then
   * increments it.
   *
   * Stack_binary(&stack, binary) has the same effect, with only one
   * decrement, without leaking the implementation of the stack out.
   *
   * TODO Let's profile to make sure the function pointer doesn't actually
   * *lose* us performance. Even if it does, we can reimplement this function
   * with the naive approach and then inline it.
   */
  assert(self->top - 1 > self->items);

  Value arg1 = *(self->top);
  self->top--;
  *(self->top) = binary(*(self->top), arg1);
}

void Thread_init(Thread* self) {
  Stack_init(&(self->stack));
  self->heap = NULL;
}

void Thread_free(Thread* self) {
  Stack_free(&(self->stack));
  assert(self->heap == NULL);
}

#define OPERATOR_BINARY_FUNCTION(name, op) \
  inline static Value name(Value arg0, Value arg1) { \
    Value result; \
    result.as.integer = arg0.as.integer op arg1.as.integer; \
    return result; \
  }

OPERATOR_BINARY_FUNCTION(add, +)
OPERATOR_BINARY_FUNCTION(subtract, -)
OPERATOR_BINARY_FUNCTION(multiply, *)
OPERATOR_BINARY_FUNCTION(divide, /)

#undef OPERATOR_BINARY_FUNCTION

Value Thread_run(Thread* self, Code* code) {
  size_t index = 0;

  for(;;) {
    assert(index < code->instructions.length);

    switch(Code_get(code, index)) {
      case OP_NIL:
      case OP_TRUE:
      case OP_FALSE:
        assert(false); // TODO Implement

      case OP_INTEGER:
        {
          index++;
          Value value;
          value.as.integer = Code_getInteger(code, index);
          Stack_push(&(self->stack), value);
          index += sizeof(int32_t);
        } break;

      #define BINARY_OP(function) Stack_binary(&(self->stack), function)
      case OP_ADD:      BINARY_OP(add);       index++;  break;
      case OP_SUBTRACT: BINARY_OP(subtract);  index++;  break;
      case OP_MULTIPLY: BINARY_OP(multiply);  index++;  break;
      case OP_DIVIDE:   BINARY_OP(divide);    index++;  break;
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
