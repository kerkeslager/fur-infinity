#ifndef FUR_THREAD_H
#define FUR_THREAD_H

#include "code.h"
#include "object.h"
#include "value.h"

#define MAX_FRAMESTACK_DEPTH 64
#define MAX_STACK_DEPTH 256

typedef struct {
  ObjClosure* closure;
  uint8_t* ip;
  Value* fp;
} Frame;

typedef struct {
  Frame items[MAX_FRAMESTACK_DEPTH];
  Frame* top;
} FrameStack;

void FrameStack_init(FrameStack*);
void FrameStack_free(FrameStack*);

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
  FrameStack frames;
  Stack stack;
  Obj* heap;
} Thread;

void Thread_init(Thread*);
void Thread_free(Thread*);

Value Thread_run(Thread*, Code*, size_t);

#endif
