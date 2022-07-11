#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "scanner.h"
#include "parser.h"

static void printNode(Node*);

static void printBinaryNode(char* name, BinaryNode* node) {
  printf("(%s ", name);
  printNode(node->arg0);
  printf(" ");
  printNode(node->arg1);
  printf(")");
}

static void printNode(Node* node) {
  if(node == NULL) {
    printf("NULL");
    fflush(stdout);
    return;
  }

  switch(node->type) {
    case NODE_ADD:
      printBinaryNode("+\0", (BinaryNode*)node);
      break;
    case NODE_SUBTRACT:
      printBinaryNode("-\0", (BinaryNode*)node);
      break;
    case NODE_MULTIPLY:
      printBinaryNode("*\0", (BinaryNode*)node);
      break;
    case NODE_DIVIDE:
      printBinaryNode("/\0", (BinaryNode*)node);
      break;

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

    default:
      assert(false);
  }
}

int main() {
  char* sources[3] = {
    "1 + 2\0",
    "3 + 4 + 5\0",
    "6 + 7 * 8\0",
  };

  for(int i = 0; i < 3; i++) {
    printf("\"%s\" => ", sources[i]);

    Scanner scanner;
    Scanner_init(&scanner, sources[i]);

    Node* node = parse(&scanner);

    assert(node != NULL);

    printNode(node);
    printf("\n");

    Node_free(node);
  }
}
