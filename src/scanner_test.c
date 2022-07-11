#include <stdio.h>

#include "scanner.h"

const char* tokenTypeAsString(TokenType type) {
    switch(type) {
      case TOKEN_NIL: return "TOKEN_NIL";
      case TOKEN_TRUE: return "TOKEN_TRUE";
      case TOKEN_FALSE: return "TOKEN_FALSE";
      case TOKEN_IDENTIFIER: return "TOKEN_IDENTIFIER";
      case TOKEN_NUMBER: return "TOKEN_NUMBER";
      case TOKEN_PLUS: return "TOKEN_PLUS";
      case TOKEN_MINUS: return "TOKEN_MINUS";
      case TOKEN_STAR: return "TOKEN_STAR";
      case TOKEN_SLASH: return "TOKEN_SLASH";

      case TOKEN_ERROR: return "TOKEN_ERROR";
      case TOKEN_EOF: return "TOKEN_EOF";

      default: return NULL;
    }
}

int main() {
  char* input = "1234567890 + - \n * \t / blink182 true false nil 82foo aBaB8\0";

  Scanner scanner;
  Scanner_init(&scanner, input);

  Token token;

  size_t previousLine = 0;

  printf("    Line   Type              Text\n");
  printf("---------------------------------------------\n");

  while((token = Scanner_scan(&scanner)).type != TOKEN_EOF) {
    const char* tokenTypeString = tokenTypeAsString(token.type);

    if(token.line == previousLine) {
      printf("       |   ");
    } else {
      previousLine = token.line;
      printf("%8zu   ", token.line);
    }

    if(tokenTypeString == NULL) {
      printf("<unknown>          ");
    } else {
      printf("%-19s", tokenTypeAsString(token.type));
    }

    printf( "\"%.*s\"\n", (int)token.length, token.text);
  }
}
