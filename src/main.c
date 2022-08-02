#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "code.h"
#include "compiler.h"
#include "memory.h"
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

void printScan(Scanner* scanner) {
  Token token;

  size_t previousLine = 0;

  printf("Line Type               Text\n");

  while((token = Scanner_scan(scanner)).type != TOKEN_EOF) {

    if(token.line == previousLine) {
      printf("   | ");
    } else {
      previousLine = token.line;
      printf("%4zu ", token.line);
    }

    const char* tokenTypeString = TokenType_asString(token.type);
    assert(tokenTypeString != NULL);
    printf("%-19s", tokenTypeString);

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

static void printTernaryNode(char* name, TernaryNode* node) {
  printf("(%s ", name);
  printNode(node->arg0);
  printf(" ");
  printNode(node->arg1);

  // The third argument to an if statement may be null
  if(node->arg2 != NULL) {
    printf(" ");
    printNode(node->arg2);
  }

  printf(")");
}

static void printNode(Node* node) {
  assert(node != NULL);

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
      printf("%.*s", (int)((AtomNode*)node)->length, ((AtomNode*)node)->text);
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
    MAP_INFIX(NODE_WHILE, while);
    #undef MAP_INFIX

    case NODE_IF:
      printTernaryNode("if", (TernaryNode*)node);
      break;

    case NODE_FN_DEF:
      {
        TernaryNode* tNode = (TernaryNode*)node;
        printf("(def ");
        printNode(tNode->arg0);

        if(tNode->arg1 == NULL) {
          printf(" () ");
        } else {
          assert(false);
        }

        printNode(tNode->arg2);

        printf(")");
      }
      break;

    case NODE_COMMA_SEPARATED_LIST:
      {
        ExpressionListNode* elNode = (ExpressionListNode*)node;

        printf("(");
        for(size_t i = 0; i < elNode->length; i++) {
          if(i != 0) printf(" ");
          printNode(elNode->items[i]);
        }
        printf(")");
      } break;

    case NODE_EXPRESSION_LIST:
      {
        ExpressionListNode* elNode = (ExpressionListNode*)node;

        printf("(do");
        for(size_t i = 0; i < elNode->length; i++) {
          printf(" ");
          printNode(elNode->items[i]);
        }
        printf(")");
      } break;

    case NODE_CALL:
      {
        BinaryNode* bNode = (BinaryNode*)node;

        printf("(__call__ ");

        printNode(bNode->arg0);

        printf(" (");

        ExpressionListNode* elNode = (ExpressionListNode*)(bNode->arg1);

        for(size_t i = 0; i < elNode->length; i++) {
          if(i > 0) printf(" ");

          printNode(elNode->items[i]);
        }

        printf("))");
      } break;

    default:
      assert(false);
  }
}

void printCodeAsAssembly(Code* code, size_t startInstructionIndex) {
  size_t lineRunIndex = 0;
  size_t lineRunCounter = 0;

  #define INCREMENT_LINE() \
    do { \
      assert(lineRunIndex < code->lineRuns.length); \
      if(lineRunCounter == code->lineRuns.items[lineRunIndex].run) { \
        lineRunIndex++; \
      } \
    } while(false)


  // Run the line up to the current spot
  for(size_t i = 0; i < startInstructionIndex; i++) {
    INCREMENT_LINE();
  }

  for(size_t i = startInstructionIndex; i < code->instructions.length; i++) {
    char opString[32] = "";
    char argString[32] = "";

    INCREMENT_LINE();
    #undef INCREMENT_LINE

    size_t instructionIndex = i;
    size_t line = code->lineRuns.items[lineRunIndex].line;

    switch(code->instructions.items[i]) {
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

      #define ONE_BYTE_ARG(op, name) \
        case op: \
          strcpy(opString, #name); \
          i++; \
          sprintf( \
              argString, \
              "%d", \
              code->instructions.items[i] \
          ); \
          break
      ONE_BYTE_ARG(OP_SET, set);
      ONE_BYTE_ARG(OP_GET, get);
      ONE_BYTE_ARG(OP_CALL, call);
      ONE_BYTE_ARG(OP_NATIVE, native);
      #undef ONE_BYTE_ARG

      #define JUMP(op, name) \
        case op: \
          strcpy(opString, #name); \
          i++; \
          sprintf( \
              argString, \
              "%i", \
              *((int16_t*)&(code->instructions.items[i])) \
          ); \
          i++; \
          break
      JUMP(OP_JUMP, jump);
      JUMP(OP_JUMP_IF_TRUE, jump_if_true);
      JUMP(OP_JUMP_IF_FALSE, jump_if_false);
      JUMP(OP_AND, and_jump);
      JUMP(OP_OR, or_jump);
      #undef JUMP

      #define MAP(op, name) \
      case op: \
        strcpy(opString, #name); \
        break
      MAP(OP_NIL, push_nil);
      MAP(OP_TRUE, push_true);
      MAP(OP_FALSE, push_false);
      MAP(OP_DROP, drop);

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
      MAP(OP_PROP, prop);
      #undef MAP

      default:
        printf("Number: %i\n", code->instructions.items[i]);
        fflush(stdout);
        assert(false);
    }

    printf("%-20s %-5s; inst: %zu, line:%zu\n", opString, argString, instructionIndex, line);
  }
}

Value runString(
    Compiler* compiler,
    Code* code,
    Thread* thread,
    Options options,
    Scanner* scanner) {

  /*
   * TODO Give the code access to the command line args. Probably in the
   * function that calls this, actually, since that is where we initialize
   * the code, and we have access to both argc, and argv there.
   */

  if(options.action == SCAN) {
    printScan(scanner);
    return Value_fromInt32(0);
  }

  Node* tree = parse(scanner);

  if(options.action == PARSE) {
    printNode(tree);
    Node_free(tree);
    return Value_fromInt32(0);
  }

  size_t startIndex = Compiler_compile(compiler, code, tree);
  Node_free(tree);

  if(options.action == COMPILE) {
    printCodeAsAssembly(code, startIndex);
    return Value_fromInt32(0);
  }

  assert(options.action == RUN);
  Thread_run(thread, code);

  return Stack_pop(&(thread->stack));
}

static int repl(Options options) {
  /*
   * We want a consistent thread and code to maintain state across evals, so
   * that users can do things in the REPL like:
   *
   * fur> greeting = 'Hello, world'
   * fur> print(greeting)
   * Hello, worldfur>
   *
   * Eventually we'll probably want a consistent runtime to manage
   * multiple threads and modules as well.
   */
  Runtime runtime;
  Runtime_init(&runtime);
  Compiler compiler;
  Compiler_init(&compiler, &runtime);
  Code code;
  Code_init(&code);
  Thread thread;
  Thread_init(&thread);

  char source[1024];

  for(size_t lineNumber = 1; ; lineNumber++) {
    printf("fur(%zu)> ", lineNumber);

    if (!fgets(source, sizeof(source), stdin)) {
      printf("\n");
      break;
    }

    Scanner scanner;
    Scanner_init(&scanner, lineNumber, source);

    Value result = runString(
      &compiler,
      &code,
      &thread,
      options,
      &scanner
    );

    if(options.action == RUN) {
      printf("=> ");
      Value_printRepr(result);
      printf("\n");
    } else if(options.action == PARSE) {
      printf("\n");
    }
  }

  Compiler_free(&compiler);
  Code_free(&code);
  Thread_free(&thread);
  Runtime_free(&runtime);

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

  char* buffer = allocateChars(fileSize + 1);

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

int runFile(Options options, char* filename) {
  char* source = readFile(filename);

  Runtime runtime;
  Runtime_init(&runtime);
  Compiler compiler;
  Compiler_init(&compiler, &runtime);
  Code code;
  Code_init(&code);
  Thread thread;
  Thread_init(&thread);

  Scanner scanner;
  Scanner_init(&scanner, 1, source);

  runString(&compiler, &code, &thread, options, &scanner);

  Compiler_free(&compiler);
  Code_free(&code);
  Thread_free(&thread);
  Runtime_free(&runtime);
  free(source);

  return 0;
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
  printf("%-20s %-59s\n", "-v, --version",          "Print version information and exit");
}

int main(int argc, char** argv) {
  Options options;
  options.action = RUN;
  options.help = false;
  options.version = false;

  for(int i = 1; i < argc; i++) {
    if(argv[i][0] == '-') {
      if(argv[i][1] == '-') {
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
      return runFile(options, argv[i]);
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
