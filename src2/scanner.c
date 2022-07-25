#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "scanner.h"

void Scanner_init(Scanner* self, char* source) {
  self->source = source;
  self->current = source;
  self->line = 1;
}

Token Scanner_scan(Scanner* self) {
  switch(*(self->current)) {
    case '\0':
      {
        Token t;
        Token_init(&t, TOKEN_EOF, self->line, 0, self->current);
        return t;
      }

    default:
      {
        char* message = malloc(32);
        sprintf(message, "Unrecognized character \"%c\"", *(self->current));
        self->current++;
        Token t;
        Token_init(&t, TOKEN_ERROR, self->line, strlen(message), message);
        return t;
      }
  }
}
