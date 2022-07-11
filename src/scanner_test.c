#include <stdio.h>

#include "scanner.h"

int main() {
  const char* input = "1234567890 + - \n * \t / blink182 true false nil";

  Scanner scanner;
  Scanner_init(&scanner, input);

  Token token;

  while((token = Scanner_scan(&scanner)).type != TOKEN_EOF) {
    printf("Token ");

    switch(token.type) {
      case TOKEN_NIL: printf("TOKEN_NIL"); break;
      case TOKEN_TRUE: printf("TOKEN_TRUE"); break;
      case TOKEN_FALSE: printf("TOKEN_FALSE"); break;
      case TOKEN_IDENTIFIER: printf("TOKEN_IDENTIFIER"); break;
      case TOKEN_NUMBER: printf("TOKEN_NUMBER"); break;
      case TOKEN_PLUS: printf("TOKEN_PLUS"); break;
      case TOKEN_MINUS: printf("TOKEN_MINUS"); break;
      case TOKEN_STAR: printf("TOKEN_STAR"); break;
      case TOKEN_SLASH: printf("TOKEN_SLASH"); break;

      case TOKEN_ERROR: printf("TOKEN_ERROR"); break;
      case TOKEN_EOF: printf("TOKEN_EOF"); break;

      default: printf("Unknown token type"); break;
    }

    printf(", line %zu: ", token.line);

    for(int i = 0; i < token.length && i < 10; i++) {
      printf("%c", token.text[i]);
    }

    printf("\n");
  }
}
