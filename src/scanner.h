#ifndef FUR_SCANNER_H
#define FUR_SCANNER_H

#include <stdbool.h>

typedef enum {
  TOKEN_NIL,
  TOKEN_TRUE,
  TOKEN_FALSE,
  TOKEN_IDENTIFIER,
  TOKEN_NUMBER,
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_STAR,
  TOKEN_SLASH,

  TOKEN_ERROR,
  TOKEN_EOF,
} TokenType;

typedef struct {
  TokenType type;
  char* text;
  size_t length;
  size_t line;
} Token;

typedef struct {
  char* source;
  char* current;
  size_t line;
  Token lookahead;
  bool hasLookahead; // TODO This feels like a hack--is there a better way?
} Scanner;

void Scanner_init(Scanner*, size_t, char*);

Token Scanner_scan(Scanner*);
Token Scanner_peek(Scanner*);

#endif
