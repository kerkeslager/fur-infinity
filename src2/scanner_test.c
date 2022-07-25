#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "scanner.h"

static void Token_print(Token self) {
  char type[16];

  switch(self.type) {
    #define MAP(t) case t: sprintf(type, "%s", #t); break
    MAP(TOKEN_INTEGER);
    MAP(TOKEN_NAME);
    MAP(TOKEN_ERROR);
    MAP(TOKEN_EOF);
    #undef MAP

    default:
      assert(false);
  }

  printf("%3zu %-16s \"", self.line, type);

  for(size_t i = 0; i < self.length; i++) {
    printf("%c", self.text[i]);
  }

  printf("\"\n");
}

int main(int argc, char** argv) {
  assert(argc == 2);
  char* source = argv[1];

  Scanner scanner;
  Scanner_init(&scanner, source);

  Token t;

  do {
    t = Scanner_scan(&scanner);
    Token_print(t);
    Token_free(&t);

  } while(t.type != TOKEN_EOF);

  return 0;
}
