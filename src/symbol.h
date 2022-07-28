#ifndef FUR_SYMBOL_H
#define FUR_SYMBOL_H

typedef struct {
  char* name;
  uint8_t length;
} Symbol;

void Symbol_init(Symbol*, size_t length, char*);
void Symbol_free(Symbol*);

#endif
