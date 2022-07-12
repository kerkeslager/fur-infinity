#ifndef FUR_PARSER_H
#define FUR_PARSER_H

#include <stdlib.h>

#include "scanner.h"

typedef enum {
  // Atom nodes
  NODE_NIL,
  NODE_TRUE,
  NODE_FALSE,
  NODE_IDENTIFIER,
  NODE_NUMBER,

  // Unary Nodes
  NODE_NEGATE,

  // Binary nodes
  NODE_ADD,
  NODE_SUBTRACT,
  NODE_MULTIPLY,
  NODE_DIVIDE,
} NodeType;

typedef struct {
  NodeType type;
  size_t line;
} Node;

// NODE_NIL, NODE_TRUE, NODE_FALSE, NODE_IDENTIFIER, NODE_NUMBER
typedef struct {
  Node node;
  char* text;
  size_t length;
} AtomNode;

typedef struct {
  Node node;
  Node* arg;
} UnaryNode;

// NODE_ADD, NODE_SUBTRACT, NODE_MULTIPLY, NODE_DIVIDE
typedef struct {
  Node node;
  Node* arg0;
  Node* arg1;
} BinaryNode;

typedef struct {
  Node node;
  Node* arg0;
  Node* arg1;
  Node* arg2;
} TernaryNode;

/*
 * TODO It might be even better to just pass in source, and also hide
 * the Scanner type.
 */
Node* parse(Scanner*);

void Node_free(Node*);

#endif
