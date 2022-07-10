#ifndef FUR_SCANNER_H
#define FUR_SCANNER_H

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
} Scanner;

void Scanner_init(Scanner*, char*);

Token Scanner_scan(Scanner*);

#endif
