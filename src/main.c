#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "code.h"
#include "compiler.h"
#include "memory.h"
#include "parser.h"
#include "read_file.h"
#include "scanner.h"
#include "thread.h"
#include "value.h"

typedef struct {
  bool help;
  bool version;
} Options;

Value runString(
    Compiler* compiler,
    Code* code,
    Thread* thread,
    Scanner* scanner) {

  /*
   * TODO Give the code access to the command line args. Probably in the
   * function that calls this, actually, since that is where we initialize
   * the code, and we have access to both argc, and argv there.
   */

  Node* tree = parse(scanner);

  size_t startIndex = Compiler_compile(compiler, code, tree);
  Node_free(tree);

  return Thread_run(thread, code);
}

static int repl() {
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
      &scanner
    );

    printf("=> ");
    Value_printRepr(result);
    printf("\n");
  }

  Compiler_free(&compiler);
  Code_free(&code);
  Thread_free(&thread);
  Runtime_free(&runtime);

  return 0;
}

int runFile(char* filename) {
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

  runString(&compiler, &code, &thread, &scanner);

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

  printf("\n");

  printf("Options syntax:\n");
  printf("* Short form options are \"-\" followed by a letter, like \"-a\"\n");
  printf("* Short form options which don't take an argument may be combined; \"-abc\" is equivalent to \"-a -b -c\"\n");
  printf("* Long-form options are \"--\" followed by a word, like \"--foo\"\n");
  printf("* Options which take an argument may be passed with a space or \"=\", like \"-o=arg\" or \"-o arg\"\n");

  printf("\n");

  printf("Options:\n");
  printf("%-20s %-59s\n", "-h, --help",             "Print this help text and exit");
  printf("%-20s %-59s\n", "-v, --version",          "Print version information and exit");
}

int main(int argc, char** argv) {
  Options options;
  options.help = false;
  options.version = false;

  for(int i = 1; i < argc; i++) {
    if(argv[i][0] == '-') {
      if(argv[i][1] == '-') {
        // Long form arguments
        if(!strcmp("--help", argv[i])) {
          options.help = true;
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
      return runFile(argv[i]);
    }
  }

  if(options.help) {
    printVersion();
    printHelp();
  } else if(options.version) {
    printVersion();
  } else {
    return repl();
  }

  return 0;
}
