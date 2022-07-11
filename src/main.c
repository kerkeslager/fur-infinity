#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "code.h"
#include "compiler.h"
#include "runtime.h"
#include "value.h"

typedef enum {
  RUN,
  SCAN,
  PARSE,
  COMPILE,
} Action;

typedef struct {
  bool help;
  bool version;
  Action action;
} Options;

void printCodeAsAssembly(Code* code) {
  assert(false);
}

static int repl(Options options) {
  Compiler compiler;
  Compiler_init(&compiler);

  Runtime runtime;
  Runtime_init(&runtime);

  char line[1024];

  for(;;) {
    printf("> ");

    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }

    switch(options.action) {
      case SCAN:
        assert(false); // Not implemented
      case PARSE:
        assert(false); // Not implemented
      case COMPILE:
        {
          Code* code = Compiler_compile(&compiler, line);
          printCodeAsAssembly(code);
        } break;
      case RUN:
        {
          Code* code = Compiler_compile(&compiler, line);
          Value result = Runtime_run(&runtime, code);
          Value_printRepr(result);
        } break;
      default:
        assert(false);
    }
  }

  Compiler_free(&compiler);
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


int runString(Options options, char* source, int argc, char** argv) {
  Compiler compiler;
  Compiler_init(&compiler);

  Runtime runtime;
  Runtime_init(&runtime);

  Code* code = Compiler_compile(&compiler, source);
  Value result = Runtime_run(&runtime, code);

  Compiler_free(&compiler);
  Runtime_free(&runtime);

  free(source);

  return Value_asSuccess(result);
}

int runFile(Options options, char* filename, int argc, char** argv) {
  char* source = readFile(filename);

  return runString(options, source, argc, argv);
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
  char* filename = NULL;

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
        return runString(options, argv[i], argc - i - 1, argv + i + 1);
      } else if(argv[i][1] == '-') {
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
      filename = argv[i];

      return runFile(options, filename, argc - i - 1, argv + i + 1);
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
