#include "token.h"

void Token_init(Token* self, TokenType type, size_t line, size_t length, char* text) {
  self->type = type;
  self->line = line;
  self->length = length;
  self->text = text;
}

void Token_free(Token* self) {
  /*
   * In most cases, self->text points to the source text, which is the responsibility
   * of the caller to free. However, the scanner allocates for error messages, so
   * we must free those.
   */
  if(self->type == TOKEN_ERROR) {
    free(self->text);
  }
}

