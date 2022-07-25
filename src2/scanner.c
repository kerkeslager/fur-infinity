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

inline static Token Scanner_scanName(Scanner* self) {
  /*
   * When we enter this function, we've already checked a letter, so we just record
   * where it is without checking it, and then move on.
   */
  char* start = self->current;
  self->current++;

  for(;;) {
    switch(*(self->current)) {
      case 'A':
      case 'B':
      case 'C':
      case 'D':
      case 'E':
      case 'F':
      case 'G':
      case 'H':
      case 'I':
      case 'J':
      case 'K':
      case 'L':
      case 'M':
      case 'N':
      case 'O':
      case 'P':
      case 'Q':
      case 'R':
      case 'S':
      case 'T':
      case 'U':
      case 'V':
      case 'W':
      case 'X':
      case 'Y':
      case 'Z':
      case 'a':
      case 'b':
      case 'c':
      case 'd':
      case 'e':
      case 'f':
      case 'g':
      case 'h':
      case 'i':
      case 'j':
      case 'k':
      case 'l':
      case 'm':
      case 'n':
      case 'o':
      case 'p':
      case 'q':
      case 'r':
      case 's':
      case 't':
      case 'u':
      case 'v':
      case 'w':
      case 'x':
      case 'y':
      case 'z':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        self->current++;
        break;

      default:
        {
          Token t;
          Token_init(&t, TOKEN_NAME, self->line, self->current - start, start);
          return t;
        }
    }
  }
}

inline static Token Scanner_scanNumber(Scanner* self) {
  /*
   * When we enter this function, we've already checked a digit, so we just record
   * where it is without checking it, and then move on.
   */

  char* start = self->current;
  self->current++;

  for(;;) {
    switch(*(self->current)) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        self->current++;
        break;

      default:
        {
          Token t;
          Token_init(&t, TOKEN_INTEGER, self->line, self->current - start, start);
          return t;
        }
    }
  }
}

Token Scanner_scan(Scanner* self) {
  switch(*(self->current)) {
    case '\0':
      {
        Token t;
        Token_init(&t, TOKEN_EOF, self->line, 0, self->current);
        return t;
      }

    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'U':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z':
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
      return Scanner_scanName(self);

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      return Scanner_scanNumber(self);

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
