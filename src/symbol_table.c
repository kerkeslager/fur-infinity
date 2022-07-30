#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "symbol.h"
#include "symbol_table.h"

static uint32_t hash(size_t length, char* text) {
  uint32_t result = 2166136261u;
  for (size_t i = 0; i < length; i++) {
    result ^= (uint8_t)text[i];
    result *= 16777619;
  }
  return result;
}

inline static bool Symbol_equal(Symbol* self, uint32_t h, uint8_t length, char* name) {
  if(self->hash != h) return false;
  if(self->length != length) return false;
  if(self->name == name) return true;

  for(uint8_t i = 0; i < length; i++) {
    if(self->name[i] != name[i]) return false;
  }

  return true;
}

void SymbolTable_init(SymbolTable* self) {
  self->capacity = 0;
  self->load = 0;
  self->items = NULL;
}

void SymbolTable_free(SymbolTable* self) {
  if(self->items != NULL) {
    for(size_t i = 0; i < self->capacity; i++) {
      if(self->items[i] != NULL) free(self->items[i]);
    }
    free(self->items);
  }
}

inline static void SymbolTable_initItems(SymbolTable* self) {
  assert(self->capacity > 0);
  assert(self->items != NULL);
  for(size_t i = 0; i < self->capacity; i++) {
    self->items[i] = NULL;
  }
}

inline static void SymbolTable_expand(SymbolTable* self) {
  assert(self->items != NULL);
  assert(self->capacity > 0);
  size_t oldCapacity = self->capacity;
  Symbol** oldItems = self->items;

  /* TODO Use allocators from memory.h */
  self->capacity = self->capacity * 2;
  self->items = malloc(sizeof(Symbol*) * self->capacity);
  assert(self->items != NULL);

  SymbolTable_initItems(self);

  for(size_t i = 0; i < oldCapacity; i++) {
    if(oldItems[i] != NULL) {
      size_t index = oldItems[i]->hash % self->capacity;

      for(;;) {
        if(self->items[index] == NULL) {
          self->items[index] = oldItems[i];
          break;
        }

        index = (index + 1) % self->capacity;
      }
    }
  }

  assert(oldItems != NULL);
  free(oldItems);
}

#define MAX_LOAD 0.75

/*
 * If the symbol is in the list, we never need to expand the table, because we're
 * just going to return the symbol that already exists. If the table is at MAX_LOAD,
 * we expand the table without checking to see if that's actually necessary.
 *
 * In practice, that only costs us time if we insert symbols *exactly* up to
 * MAX_LOAD, then get a symbol that already exists, and then *never* add any more
 * symbols that don't exist: if we don't insert up to MAX_LOAD and then get a
 * symbol that already exists, we don't trigger the expansion, and if we add
 * even one more symbol that doesn't exist, we get that time back because we would
 * have to expand the table, but it's already expanded.
 *
 * Since I believe exactly hitting MAX_LOAD like that is a relatively rare occurence,
 * it seems probable that we would lose more time by checking to see if symbols
 * already exist before expanding the table.
 */
Symbol* SymbolTable_getSymbol(SymbolTable* self, uint8_t length, char* name) {
  if(self->capacity == 0) {
    assert(self->items == NULL);
    self->capacity = 64;
    self->items = malloc(sizeof(Symbol*) * self->capacity);
    SymbolTable_initItems(self);
  } else if(((double)(self->load + 1)) / ((double)self->capacity) > MAX_LOAD) {
    SymbolTable_expand(self);
  }

  uint32_t h = hash(length, name);

  size_t index = h % self->capacity;

  for(;;) {
    if(self->items[index] == NULL) {
      Symbol* newSymbol = malloc(sizeof(Symbol));
      Symbol_init(newSymbol, h, length, name);
      self->items[index] = newSymbol;
      self->load++;
      return newSymbol;
    } else if(Symbol_equal(self->items[index], h, length, name)) {
      return self->items[index];
    }

    index = (index + 1) % self->capacity;
  }

  assert(false);
}

#undef MAX_LOAD
