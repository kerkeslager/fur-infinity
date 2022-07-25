#ifndef FUR_TOKEN_H
#define FUR_TOKEN_H

#include <stdlib.h>

typedef enum {
  TOKEN_INTEGER,
  TOKEN_NAME,

  TOKEN_ERROR,
  TOKEN_EOF
} TokenType;

typedef struct {
  size_t line;
  size_t length;
  char* text;
  TokenType type; /* Keep this last for struct packing. */
} Token;

void Token_init(Token*, TokenType type, size_t line, size_t length, char* text);
void Token_free(Token*);

#endif
