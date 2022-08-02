#include <assert.h>
#include <stdlib.h>

#include "code.h"
#include "compiler.h"
#include "parser.h"
#include "read_file.h"
#include "runtime.h"
#include "scanner.h"

int main(int argc, char** argv) {
  assert(argc == 2);

  char* filename = argv[1];
  char* source = readFile(filename);

  Scanner scanner;
  Scanner_init(&scanner, 1, source);

  Node* tree = parse(&scanner);

  Runtime runtime;
  Runtime_init(&runtime);
  Compiler compiler;
  Compiler_init(&compiler, &runtime);
  Code code;
  Code_init(&code);

  size_t startIndex = Compiler_compile(&compiler, &code, tree);
  Node_free(tree);

  Code_printAsAssembly(&code, startIndex);

  Compiler_free(&compiler);
  Code_free(&code);
  Runtime_free(&runtime);
  free(source);

  return 0;
}
