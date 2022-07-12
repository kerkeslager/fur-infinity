#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "scanner.h"

void Scanner_init(Scanner* self, size_t startLine, char* source) {
  self->source = source;
  self->current = source;
  self->line = startLine;
  self->hasLookahead = false;
}

static void Scanner_scanWhitespace(Scanner* self) {
  for(;;) {
    switch(*(self->current)) {
      case '\n':
        self->line++;
        // Do *not* break, cascade

      case ' ':
      case '\t':
      case '\r':
        self->current++;
        break;

      default:
        return;
    }
  }
}

static Token makeToken(TokenType type, char* text, size_t length, size_t line) {
  Token result;
  result.type = type;
  result.text = text;
  result.length = length;
  result.line = line;
  return result;
}

inline static Token makeError(char* errorMessage, size_t line) {
  return makeToken(
    TOKEN_ERROR,
    errorMessage,
    strlen(errorMessage),
    line
  );
}

static bool isAlphaNumeric(char ch) {
  switch(ch) {
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
      return true;
    default:
      return false;
  }
}
static bool isNumeric(char ch) {
  switch(ch) {
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
      return true;
    default:
      return false;
  }
}

static Token Scanner_scanIdentifier(Scanner* self, char* start) {
  while(isAlphaNumeric(*(self->current))) self->current++;

  return makeToken(TOKEN_IDENTIFIER, start, self->current - start, self->line);
}

static Token Scanner_scanNumber(Scanner* self, char* start) {
  while(isNumeric(*(self->current))) self->current++;

  return makeToken(TOKEN_NUMBER, start, self->current - start, self->line);
}

inline static Token Scanner_scanString(Scanner* self, char* start, char quoteChar) {
  for(;;) {
    char ch = *(self->current);

    switch(ch) {
      case '\n':
        return makeError(
          "Unexpected end of line while scanning string literal",
          self->line
        );

      case '\0':
        return makeError(
          "Unexpected end of file while scanning string literal",
          self->line
        );

      case '\\':
        self->current++;
        ch = *(self->current);

        switch(ch) {
          // TODO Support more escape sequences
          case '\'':
          case '\"':
          case '\\':
          case 'n':
          case 'r':
          case 't':
            self->current++;
            /*
             * Recursing avoids a problem with breaking from nested
             * switch statements.
             */
            return Scanner_scanString(self, start, quoteChar);

          default:
            return makeError(
                "Found unsupported escape sequence while scanning string",
                self->line
            );
        }

      case '\'':
        if(ch == quoteChar) {
          self->current++;
          return makeToken(
            TOKEN_SQSTR,
            start,
            self->current - start,
            self->line
          );
        } break;

      case '"':
        if(ch == quoteChar) {
          self->current++;
          return makeToken(
            TOKEN_DQSTR,
            start,
            self->current - start,
            self->line
          );
        } break;
    }

    self->current++;
  }
}

static Token Scanner_scanKeyword(Scanner* self, char* start, const char* suffix, TokenType type) {
  for(;;) {
    if(*suffix == *(self->current)) {
      suffix++;
      self->current++;
    } else if(isAlphaNumeric(*(self->current))) {
      self->current++;
      return Scanner_scanIdentifier(self, start);
    } else if(*suffix == '\0') {
      return makeToken(type, start, self->current - start, self->line);
    } else {
      return makeToken(TOKEN_IDENTIFIER, start, self->current - start, self->line);
    }
  }
}

static Token Scanner_scanInternal(Scanner* self) {
  /*
   * This is the core function of the scanner, but it's wrapped in the
   * Scanner_scan() and Scanner_peek() functions. The only purpose of
   * this wrapping is to manage the self->lookahead and self->hasLookahead
   * properties.
   */
  Scanner_scanWhitespace(self);

  switch(*(self->current)) {
    case 'a':
      {
        char* start = self->current;
        self->current++;
        return Scanner_scanKeyword(self, start, "nd", TOKEN_AND);
      }

    case 'f':
      {
        char* start = self->current;
        self->current++;
        return Scanner_scanKeyword(self, start, "alse", TOKEN_FALSE);
      }

    case 'n':
      {
        char* start = self->current;
        self->current++;

        switch(*(self->current)) {
          case 'i':
            self->current++;
            return Scanner_scanKeyword(self, start, "l", TOKEN_NIL);
          case 'o':
            self->current++;
            return Scanner_scanKeyword(self, start, "t", TOKEN_NOT);
          default:
            return Scanner_scanIdentifier(self, start);
        }
      }

    case 'o':
      {
        char* start = self->current;
        self->current++;
        return Scanner_scanKeyword(self, start, "r", TOKEN_OR);
      }

    case 't':
      {
        char* start = self->current;
        self->current++;
        return Scanner_scanKeyword(self, start, "rue", TOKEN_TRUE);
      }

    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
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
      {
        char* start = self->current;
        self->current++;
        return Scanner_scanIdentifier(self, start);
      }

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
      {
        char* start = self->current;
        self->current++;
        return Scanner_scanNumber(self, start);
      }

    case '\'':
    case '"':
      {
        char* start = self->current;
        self->current++;
        return Scanner_scanString(self, start, *start);
      }

    #define ONE_CHAR_TOKEN(ch, type) \
    case ch: \
      { \
        char* start = self->current; \
        self->current++; \
        return makeToken(type, start, 1, self->line); \
      }
    ONE_CHAR_TOKEN('+', TOKEN_PLUS);
    ONE_CHAR_TOKEN('-', TOKEN_MINUS);
    ONE_CHAR_TOKEN('*', TOKEN_STAR);
    ONE_CHAR_TOKEN('/', TOKEN_SLASH);
    ONE_CHAR_TOKEN('(', TOKEN_OPEN_PAREN);
    ONE_CHAR_TOKEN(')', TOKEN_CLOSE_PAREN);
    ONE_CHAR_TOKEN('.', TOKEN_DOT);
    #undef ONE_CHAR_TOKEN

    case '<':
      {
        char* start = self->current;
        self->current++;

        switch(*(self->current)) {
          case '=':
            self->current++;
            return makeToken(TOKEN_LEQ, start, 2, self->line);
          default:
            return makeToken(TOKEN_LT, start, 1, self->line);
        }
      }
    case '>':
      {
        char* start = self->current;
        self->current++;

        switch(*(self->current)) {
          case '=':
            self->current++;
            return makeToken(TOKEN_GEQ, start, 2, self->line);
          default:
            return makeToken(TOKEN_GT, start, 1, self->line);
        }
      }
    case '=':
      {
        char* start = self->current;
        self->current++;

        switch(*(self->current)) {
          case '=':
            self->current++;
            return makeToken(TOKEN_EQ, start, 2, self->line);
          default:
            return makeToken(TOKEN_ASSIGN, start, 1, self->line);
        }
      }
    case '!':
      {
        char* start = self->current;
        self->current++;

        switch(*(self->current)) {
          case '=':
            self->current++;
            return makeToken(TOKEN_NEQ, start, 2, self->line);
          default:
            return makeError(
              "Unexpected token \"!\"",
              self->line
            );
        }
      }

    case '\0':
      return makeToken(TOKEN_EOF, self->current, 1, self->line);

    default:
      // TODO More detailed error message and a sane length value
      return makeError(
        "General scan error",
        self->line
      );
  }
}

Token Scanner_scan(Scanner* self) {
  if(self->hasLookahead) {
    self->hasLookahead = false;
    return self->lookahead;
  }

  return Scanner_scanInternal(self);
}

Token Scanner_peek(Scanner* self) {
  if(!self->hasLookahead) {
    self->lookahead = Scanner_scanInternal(self);
    self->hasLookahead = true;
  }

  return self->lookahead;
}
