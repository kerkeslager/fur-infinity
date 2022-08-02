#include <assert.h>
#include <stdlib.h>

#include "read_file.h"
#include "scanner.h"

int main(int argc, char** argv) {
  assert(argc == 2);

  char* filename = argv[1];
  char* source = readFile(filename);

  Scanner scanner;
  Scanner_init(&scanner, 1, source);
  Scanner_printScan(&scanner);

  free(source);

  return 0;
}
