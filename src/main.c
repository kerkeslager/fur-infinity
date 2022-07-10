#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "code.h"
#include "compiler.h"
#include "runtime.h"
#include "value.h"

static int repl() {
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

    Code* code = Compiler_compile(&compiler, line);
    Value result = Runtime_run(&runtime, code);
    Value_printRepr(result);
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

int runFile(char* filename, int argc, char** argv) {
  char* source = readFile(filename);

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

int main(int argc, char** argv) {
  char* filename = NULL;

  for(int i = 1; i < argc; i++) {
    if(argv[i][0] == '-') {
      if(argv[i][1] == '-') {
        printf("Long form argument: %s\n", argv[i]);
      } else {
        printf("Short form argument: %s\n", argv[i]);
      }
    } else {
      filename = argv[i];

      return runFile(filename, argc - i - 1, argv + i + 1);
    }
  }

  return repl();
}
