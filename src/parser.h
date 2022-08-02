#ifndef FUR_PARSER_H
#define FUR_PARSER_H

#include <stdlib.h>

#include "memory.h"
#include "scanner.h"

typedef enum {
  // Atom nodes
  NODE_NIL,
  NODE_TRUE,
  NODE_FALSE,
  NODE_IDENTIFIER,
  NODE_NUMBER,
  NODE_STRING,

  // Unary Nodes
  NODE_NEGATE,
  NODE_NOT,

  // Binary nodes
  NODE_PROPERTY,
  NODE_ADD,
  NODE_SUBTRACT,
  NODE_MULTIPLY,
  NODE_DIVIDE,
  NODE_EQUALS,
  NODE_NOT_EQUALS,
  NODE_GREATER_THAN_EQUALS,
  NODE_LESS_THAN_EQUALS,
  NODE_GREATER_THAN,
  NODE_LESS_THAN,
  NODE_AND,
  NODE_OR,
  NODE_ASSIGN,
  NODE_WHILE,
  NODE_CALL,

  // Ternary nodes
  NODE_FN_DEF,
  NODE_IF,

  // List nodes
  NODE_COMMA_SEPARATED_LIST,
  NODE_EXPRESSION_LIST,

} NodeType;

void NodeType_print(NodeType);

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

inline static ALLOCATE_ONE_IMPL(AtomNode);

typedef struct {
  Node node;
  Node* arg;
} UnaryNode;

inline static ALLOCATE_ONE_IMPL(UnaryNode);

// NODE_ADD, NODE_SUBTRACT, NODE_MULTIPLY, NODE_DIVIDE
typedef struct {
  Node node;
  Node* arg0;
  Node* arg1;
} BinaryNode;

inline static ALLOCATE_ONE_IMPL(BinaryNode);

typedef struct {
  Node node;
  Node* arg0;
  Node* arg1;
  Node* arg2;
} TernaryNode;

inline static ALLOCATE_ONE_IMPL(TernaryNode);

typedef struct {
  Node node;
  size_t length;
  size_t capacity;
  Node** items;
} ExpressionListNode;

inline static ALLOCATE_ONE_IMPL(ExpressionListNode);

Node* parse(Scanner*);
Node* parseStatement(Scanner*);

void Node_free(Node*);
void Node_print(Node*);

#endif
