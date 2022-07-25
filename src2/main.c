#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define REPL_BUFFER_LENGTH 1024

static void repl() {
  char source[REPL_BUFFER_LENGTH];

  for(;;) {
    printf("> ");

    if (!fgets(source, REPL_BUFFER_LENGTH, stdin)) {
      printf("\n");
      break;
    }

    printf("=> %s", source);
  }
}

#undef REPL_BUFFER_LENGTH

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

void runFile(char* filename) {
  char* source = readFile(filename);
  printf("%s", source);
}

int main(int argc, char** argv) {
  switch(argc) {
    case 1:
      repl();
      return 0;

    case 2:
      runFile(argv[1]);
      return 0;

    default:
      assert(false);
  }

  return 0;
}
