#ifndef FUR_SCANNER_H
#define FUR_SCANNER_H

#include "token.h"

typedef struct {
  char* source;
  char* current;
  size_t line;
} Scanner;

void Scanner_init(Scanner*, char*);

Token Scanner_scan(Scanner*);

#endif
