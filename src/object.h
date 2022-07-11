#ifndef FUR_OBJECT_H
#define FUR_OBJECT_H

typedef struct Object Object;

struct Object {
  int notImplemented; // TODO Implement.

  // Heap access for garbage collection.
  Object* next;
};

#endif
