#ifndef FUR_THREAD_H
#define FUR_THREAD_H

#define MAX_STACK_DEPTH 256

#include "code.h"
#include "object.h"
#include "value.h"

typedef struct {
  Value items[MAX_STACK_DEPTH];
  Value* top;
} Stack;

void Stack_init(Stack*);
void Stack_free(Stack*);

// TODO Profile inlining these.
void Stack_push(Stack*, Value);
Value Stack_pop(Stack*);
void Stack_binary(Stack*, Value (*binary)(Value, Value));

typedef struct {
  Stack stack;
  Obj* heap;
} Thread;

void Thread_init(Thread*);
void Thread_free(Thread*);

size_t Thread_run(Thread*, Code*, size_t);

#endif
