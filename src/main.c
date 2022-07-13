#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "code.h"
#include "compiler.h"
#include "parser.h"
#include "scanner.h"
#include "thread.h"
#include "value.h"

typedef enum {
  SCAN,
  PARSE,
  COMPILE,
  RUN,
} Action;

typedef struct {
  bool help;
  bool version;
  Action action;
} Options;

const char* tokenTypeAsString(TokenType type) {
    switch(type) {
      #define MAP(token) case token: return #token;
      MAP(TOKEN_NIL);
      MAP(TOKEN_TRUE);
      MAP(TOKEN_FALSE);
      MAP(TOKEN_NOT);
      MAP(TOKEN_IDENTIFIER);
      MAP(TOKEN_NUMBER);
      MAP(TOKEN_SQSTR);
      MAP(TOKEN_DQSTR);
      MAP(TOKEN_DOT);
      MAP(TOKEN_PLUS);
      MAP(TOKEN_MINUS);
      MAP(TOKEN_STAR);
      MAP(TOKEN_SLASH);
      MAP(TOKEN_OPEN_PAREN);
      MAP(TOKEN_CLOSE_PAREN);
      MAP(TOKEN_EQ);
      MAP(TOKEN_NEQ);
      MAP(TOKEN_GEQ);
      MAP(TOKEN_LEQ);
      MAP(TOKEN_GT);
      MAP(TOKEN_LT);
      MAP(TOKEN_AND);
      MAP(TOKEN_OR);
      MAP(TOKEN_ASSIGN);

      MAP(TOKEN_ERROR);
      MAP(TOKEN_EOF);
      #undef MAP

      default: return NULL;
    }
}

void printScan(Scanner* scanner) {
  Token token;

  size_t previousLine = 0;

  printf("Line Type               Text\n");

  while((token = Scanner_scan(scanner)).type != TOKEN_EOF) {
    const char* tokenTypeString = tokenTypeAsString(token.type);

    if(token.line == previousLine) {
      printf("   | ");
    } else {
      previousLine = token.line;
      printf("%4zu ", token.line);
    }

    if(tokenTypeString == NULL) {
      printf("<unknown>          ");
    } else {
      printf("%-19s", tokenTypeAsString(token.type));
    }

    printf( "\"%.*s\"\n", (int)token.length, token.text);
  }
}

static void printNode(Node*);

static void printUnaryNode(char* name, UnaryNode* node) {
  printf("(%s ", name);
  printNode(node->arg);
  printf(")");
}

static void printBinaryNode(char* name, BinaryNode* node) {
  printf("(%s ", name);
  printNode(node->arg0);
  printf(" ");
  printNode(node->arg1);
  printf(")");
}

static void printNode(Node* node) {
  switch(node->type) {
    case NODE_NIL:
      printf("nil");
      break;
    case NODE_TRUE:
      printf("true");
      break;
    case NODE_FALSE:
      printf("false");
      break;
    case NODE_IDENTIFIER:
      printf("<var \"%.*s\">", (int)((AtomNode*)node)->length, ((AtomNode*)node)->text);
      break;
    case NODE_NUMBER:
      printf("%.*s", (int)((AtomNode*)node)->length, ((AtomNode*)node)->text);
      break;
    case NODE_STRING:
      printf("%.*s", (int)((AtomNode*)node)->length, ((AtomNode*)node)->text);
      break;

    case NODE_NEGATE:
      printUnaryNode("-\0", (UnaryNode*)node);
      break;
    case NODE_NOT:
      printUnaryNode("not", (UnaryNode*)node);
      break;

    #define MAP_INFIX(type, s) \
    case type: \
      printBinaryNode(#s, (BinaryNode*) node);\
      break
    MAP_INFIX(NODE_ADD, +);
    MAP_INFIX(NODE_SUBTRACT, -);
    MAP_INFIX(NODE_MULTIPLY, *);
    MAP_INFIX(NODE_DIVIDE, /);
    MAP_INFIX(NODE_EQUALS, ==);
    MAP_INFIX(NODE_NOT_EQUALS, !=);
    MAP_INFIX(NODE_GREATER_THAN_EQUALS, >=);
    MAP_INFIX(NODE_LESS_THAN_EQUALS, <=);
    MAP_INFIX(NODE_GREATER_THAN, >);
    MAP_INFIX(NODE_LESS_THAN, <);
    MAP_INFIX(NODE_ASSIGN, =);
    MAP_INFIX(NODE_AND, and);
    MAP_INFIX(NODE_OR, or);
    MAP_INFIX(NODE_PROPERTY, .);
    #undef MAP_INFIX

    default:
      assert(false);
  }
}

void printCodeAsAssembly(Code* code) {
  size_t lineRunIndex = 0;
  size_t lineRunCounter = 0;

  for(size_t i = 0; i < code->instructions.length; i++) {
    char opString[20] = "";
    char argString[20] = "";

    assert(lineRunIndex < code->lineRuns.length);

    if(lineRunCounter == code->lineRuns.items[lineRunIndex].run) {
      lineRunIndex++;
    }

    size_t line = code->lineRuns.items[lineRunIndex].line;

    switch(code->instructions.items[i]) {
      #define MAP(op, name) \
      case op: \
        strcpy(opString, #name); \
        break
      MAP(OP_NIL, push_nil);
      MAP(OP_TRUE, push_true);
      MAP(OP_FALSE, push_false);
      case OP_INTEGER:
        strcpy(opString, "push_int");
        sprintf(
            argString,
            "%d",
            *((int32_t*)(code->instructions.items + i + 1))
        );
        i += sizeof(int32_t);
        break;
      case OP_STRING:
        strcpy(opString, "push_string");
        i++;
        sprintf(
            argString,
            "%d",
            code->instructions.items[i]
        );
        break;
      MAP(OP_ADD, add);
      MAP(OP_SUBTRACT, sub);
      MAP(OP_MULTIPLY, mul);
        // This is "integer divide", i.e. 3 / 2 = 1
        // Distinguished from "divide", i.e. 3 / 2 = 1.5
      MAP(OP_DIVIDE, int_div);
      MAP(OP_NEGATE, neg);
      MAP(OP_NOT, not);
      MAP(OP_RETURN, ret);
      MAP(OP_EQ, eq);
      MAP(OP_LT, lt);
      MAP(OP_GT, gt);
      MAP(OP_NEQ, neq);
      MAP(OP_GEQ, geq);
      MAP(OP_LEQ, leq);
      MAP(OP_AND, and);
      MAP(OP_OR, or);
      MAP(OP_SET, set);
      MAP(OP_GET, get);
      MAP(OP_PROP, prop);
      #undef MAP
      default:
        assert(false);
    }

    printf("%-10s %-5s ; %zu\n", opString, argString, line);
  }
}

Value runString(
    Thread* thread,
    Options options,
    char* source,
    int argc,
    char** argv,
    size_t startLine) {
  switch(options.action) {
    case SCAN:
      {
        Scanner scanner;
        Scanner_init(&scanner, startLine, source);
        printScan(&scanner);
        return Value_fromInt32(0);
      }

    case PARSE:
      {
        Scanner scanner;
        Scanner_init(&scanner, startLine, source);
        Node* tree = parse(&scanner);
        printNode(tree);
        Node_free(tree);
        return Value_fromInt32(0);
      }

    case COMPILE:
      {
        Compiler compiler;
        Compiler_init(&compiler);
        Code* code = Compiler_compile(&compiler, source);
        printCodeAsAssembly(code);
        Compiler_free(&compiler);
        return Value_fromInt32(0);
      }

    case RUN:
      {
        Compiler compiler;
        Compiler_init(&compiler);

        Code* code = Compiler_compile(&compiler, source);
        Value result = Thread_run(thread, code);

        /*
         * TODO We're leaking memory here, because we should be calling
         * a Code_free() which frees the code (notably including interned
         * strings).
         */
        Compiler_free(&compiler);

        return result;
      }

    default:
      assert(false);
      return Value_fromInt32(1); // Just to quiet the cc
  }
}

static int repl(Options options) {
  /*
   * TODO We're currently throwing away the Code in every read/eval/print
   * cycle. This means that in situations like:
   *
   * fur> a = 1
   * fur> a
   * => 1
   *
   * The Code gets thrown away between assignment and reference. Since the
   * variable name "a" is interned on the Code object, we need to fix this
   * before we can reference the variable name.
   *
   * This also means that our compiler must support compiling completely
   * new sources onto an existing Code object. I'm not aware of any decisions
   * I've made that will make that impossible, but it's something to be
   * aware of, I guess.
   */
  Compiler compiler;
  Compiler_init(&compiler);

  /*
   * We want a consistent thread to maintain state across evals, so that
   * users can do things in the REPL like:
   *
   * fur> greeting = 'Hello, world'
   * fur> print(greeting)
   * Hello, worldfur>
   *
   * Eventually we'll probably want a consistent runtime as well.
   */
  Thread thread;
  Thread_init(&thread);

  char line[1024];

  size_t lineNumber = 1;

  for(;;) {
    printf("fur> ");

    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }

    Value result = runString(
      &thread,
      options,
      line,
      0,          // Don't pass in any command line arguments
      NULL,       // The empty argument array
      lineNumber  // The line number to start on
    );

    if(options.action == RUN) {
      printf("=> ");
      Value_printRepr(result);
      printf("\n");
    } else if(options.action == PARSE) {
      printf("\n");
    }

    // TODO We don't support heap allocation yet, but when we do, we will
    // need to free the value here if it's heap-allocated.
  }

  Thread_free(&thread);

  return 0;
}

static char* readFile(const char* path) {
  FILE* file = fopen(path, "rb");

  if (file == NULL) {
    fprintf(stderr, "Could not open file \"%s\".\n", path);
    exit(1);
  }

  fseek(file, 0L, SEEK_END);
  size_t fileSize = ftell(file);
  rewind(file);

  char* buffer = (char*)malloc(fileSize + 1);

  if (buffer == NULL) {
    fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
    exit(1);
  }

  size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
  if (bytesRead < fileSize) {
    fprintf(stderr, "Could not read file \"%s\".\n", path);
    exit(1);
  }
  buffer[bytesRead] = '\0';

  fclose(file);
  return buffer;
}

int runFile(Options options, char* filename, int argc, char** argv) {
  char* source = readFile(filename);

  Thread thread;
  Thread_init(&thread);

  Value result = runString(&thread, options, source, argc, argv, 1);

  /*
   * We must call Value_asSuccess() before Thread_free(), because result
   * may reference the heap, which is freed by Thread_free()
   */
  int ret = Value_asSuccess(result);

  Thread_free(&thread);
  free(source);

  return ret;
}

void printVersion() {
  printf("Fur: the 1000-year language.\n");
  printf("It's too early in the 1000 years for version information to be meaningful.\n");
}

void printUsage() {
  printf("Usage: fur [options] [filename] [args]\n");
}

void printHelp() {
  printUsage();

  printf("\n");

  printf("Examples:\n");
  printf("%-30s %-49s\n", "fur",                                  "Run the repl");
  printf("%-30s %-49s\n", "fur program.fur 1 2 3",                "Run `program.fur` with arguments `1`, `2`, and `3`");
  printf("%-30s %-49s\n", "fur --scan",                           "Run the repl, but output tokens instead of running code (--parse and --bytes work similarly)");
  printf("%-30s %-49s\n", "fur --scan program.fur",               "Run the scanner on `program.fur` and output scan result instead of running");
  printf("%-30s %-49s\n", "fur -i \"print('Hello, world')\" foo", "Run the fur code \"print('Hello, world')\" with the argument \"foo\"");

  printf("\n");

  printf("Options syntax:\n");
  printf("* Short form options are \"-\" followed by a letter, like \"-a\"\n");
  printf("* Short form options which don't take an argument may be combined; \"-abc\" is equivalent to \"-a -b -c\"\n");
  printf("* Long-form options are \"--\" followed by a word, like \"--foo\"\n");
  printf("* Options which take an argument may be passed with a space or \"=\", like \"-o=arg\" or \"-o arg\"\n");

  printf("\n");

  printf("Options:\n");
  printf("%-20s %-59s\n", "--compile",              "Don't run code, print the fur assembly");
  printf("%-20s %-59s\n", "-h, --help",             "Print this help text and exit");
  printf("%-20s %-59s\n", "--parse",                "Don't run code, print a representation of the syntax");
  printf("%-20s %-59s\n", "--scan",                 "Don't run code, print a representation of the tokens");
  printf("%-20s %-59s\n", "-i, --interpret [arg]",  "Interpret [arg] as fur code");
  printf("%-20s %-59s\n", "-v, --version",          "Print version information and exit");
}

int main(int argc, char** argv) {
  Options options;
  options.action = RUN;
  options.help = false;
  options.version = false;

  for(int i = 1; i < argc; i++) {
    if(argv[i][0] == '-') {
      // TODO Capture = arguments like -i="print('Hello, world')"
      if(!strncmp("-i", argv[i], strlen("-i")) || !strncmp("--interpret", argv[i], strlen("--interpret"))) {
        i++;
        if(i == argc) {
          fprintf(stderr, "-i/--interpret requires an argument\n");
          printf("Pass -h or --help for more information.\n");
          return 1;
        }

        Thread thread;
        Thread_init(&thread);

        Value result = runString(
          &thread,
          options,
          argv[i],      // Use the argument as the source
          argc - i - 1, // Pass in remaining arguments as command line args
          argv + i + 1,
          1
        );

        /*
         * We must call Value_asSuccess before freeing the thread, because if
         * result is an object, it may reference the heap, which is freed
         * when the thread is freed.
         */
        int ret = Value_asSuccess(result);
        Thread_free(&thread);
        return ret;
      } else if(argv[i][1] == '-') {
        // Long form arguments
        if(!strcmp("--compile", argv[i])) {
          options.action = COMPILE;
        } else if(!strcmp("--help", argv[i])) {
          options.help = true;
        } else if(!strcmp("--parse", argv[i])) {
          options.action = PARSE;
        } else if(!strcmp("--scan", argv[i])) {
          options.action = SCAN;
        } else if(!strcmp("--version", argv[i])) {
          options.version = true;
        } else {
          fprintf(stderr, "Unknown argument: %s\n", argv[i]);
          printUsage();
          printf("Pass -h or --help for more information.\n");
          return 1;
        }
      } else {
        for(int j = 1; argv[i][j] != '\0'; j++) {
          if(argv[i][j] == 'i') {
            fprintf(stderr, "Short-form argument -%c cannot be combined with other short-form options because it takes an argument\n", argv[i][j]);
            printf("Pass -h or --help for more information.\n");
            return 1;
          }

          if(argv[i][j] == 'h') {
            options.help = true;
          } else if(argv[i][j] == 'v') {
            options.version = true;
          } else {
            fprintf(stderr, "Unknown argument: -%c\n", argv[i][j]);
            printUsage();
            printf("Pass -h or --help for more information.\n");
            return 1;
          }
        }
      }
    } else {
      return runFile(
          options,
          argv[i],      // Fur source file name
          argc - i - 1, // Number of arguments after filename
          argv + i + 1  // Remaining arguments after filename
      );
    }
  }

  if(options.help) {
    printVersion();
    printHelp();
  } else if(options.version) {
    printVersion();
  } else {
    return repl(options);
  }

  return 0;
}
