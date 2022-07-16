/*
 * The following is a Pratt parser.
 *
 * I found this hard to understand, so I'm commenting it as thoroughly as I can.
 *
 * Other possibilities considered:
 * - Packrat parser: Both Pratt and Packrat are O(n), but Pratt uses less memory.
 * - Recursive descent: hard to do efficiently without global state, in my experience.
 * - Any parser generator: I didn't want to introduce this sort of dependency.
 */

#include "parser.h"
#include "scanner.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

static Node makeNode(NodeType type, size_t line) {
  Node result;
  result.type = type;
  result.line = line;
  return result;
}

static Node* makeAtomNode(Token token) {
  NodeType type;

  switch(token.type) {
    case TOKEN_NIL:
      type = NODE_NIL;
      break;
    case TOKEN_TRUE:
      type = NODE_TRUE;
      break;
    case TOKEN_FALSE:
      type = NODE_FALSE;
      break;
    case TOKEN_IDENTIFIER:
      type = NODE_IDENTIFIER;
      break;
    case TOKEN_NUMBER:
      type = NODE_NUMBER;
      break;
    case TOKEN_SQSTR:
    case TOKEN_DQSTR:
      type = NODE_STRING;
      break;

    default:
      assert(false); // TODO Better handling.
  }

  AtomNode* node = malloc(sizeof(AtomNode));
  node->node = makeNode(type, token.line);
  node->text = token.text;
  node->length = token.length;
  return (Node*)node;
}

static Node* makeUnaryNode(Token operator, Node* arg) {
  NodeType type;

  switch(operator.type) {
    case TOKEN_MINUS: type = NODE_NEGATE; break;
    case TOKEN_NOT:   type = NODE_NOT;    break;

    default:
      assert(false);
  }

  UnaryNode* node = malloc(sizeof(UnaryNode));
  node->node = makeNode(type, operator.line);
  node->arg = arg;
  return (Node*)node;
}

static Node* makeBinaryNode(NodeType type, size_t line, Node* arg0, Node* arg1) {
  BinaryNode* node = malloc(sizeof(BinaryNode));
  node->node = makeNode(type, line);
  node->arg0 = arg0;
  node->arg1 = arg1;
  return (Node*)node;
}

static Node* makeTernaryNode(NodeType type, size_t line, Node* arg0, Node* arg1, Node* arg2) {
  TernaryNode* node = malloc(sizeof(TernaryNode));
  node->node = makeNode(type, line);
  node->arg0 = arg0;
  node->arg1 = arg1;
  node->arg2 = arg2;
  return (Node*)node;
}

typedef enum {
  // This precedence rule should never match
  PREC_NONE,

  // This is the minimum binding power we start with
  PREC_ANY,

  /*
   * Left before right means left < right, because enum values are chosen sequentially
   * Left < right indicates left associative, i.e. 1 + 2 + 3 => (+ (+ 1 2) 3)
   * This is because when we check the precedence it will give a lower binding power.
   */

  // Right associative
  PREC_ASSIGN_RIGHT,
  PREC_ASSIGN_LEFT,

  // Left associative
  PREC_OR_LEFT,
  PREC_OR_RIGHT,

  // Left associative
  PREC_AND_LEFT,
  PREC_AND_RIGHT,

  PREC_NOT,

  // Left associative
  PREC_CMP_LEFT,
  PREC_CMP_RIGHT,

  // Left associative
  PREC_ADD_LEFT,
  PREC_ADD_RIGHT,

  // Also left associative
  PREC_MUL_LEFT,
  PREC_MUL_RIGHT,

  PREC_NEG,

  // Also left associative
  PREC_DOT_LEFT,
  PREC_DOT_RIGHT,
} Precedence;

typedef struct {
  Precedence prefix;
  Precedence infixLeft;
  Precedence infixRight;
  Precedence postfix;
  bool isAtom; // Smallest item last for struct packing
} PrecedenceRule;

Node* parseInternal(Scanner* scanner, Precedence minimumBindingPower);

Node* parseIf(Scanner* scanner, size_t line) {
  // TODO Can we set a precedence that ensures this is a boolean?
  Node* test = parseInternal(scanner, PREC_ANY);

  Token token = Scanner_scan(scanner);
  assert(token.type == TOKEN_COLON);

  Node* leftBranch = parseInternal(scanner, PREC_ANY);

  Node* rightBranch = NULL;

  token = Scanner_scan(scanner);
  assert(token.type == TOKEN_ELSE || token.type == TOKEN_END);

  if(token.type == TOKEN_ELSE) {
    rightBranch = parseInternal(scanner, PREC_ANY);
    token = Scanner_scan(scanner);
  }

  assert(token.type == TOKEN_END);

  return makeTernaryNode(NODE_IF, line, test, leftBranch, rightBranch);
}

const static PrecedenceRule PRECEDENCE_TABLE[] = {
// TokenType              Prefix,     InfixLeft,        InfixRight,         Postfix,    isAtom
  [TOKEN_NIL] =         { PREC_NONE,  PREC_NONE,        PREC_NONE,          PREC_NONE,  true },
  [TOKEN_TRUE] =        { PREC_NONE,  PREC_NONE,        PREC_NONE,          PREC_NONE,  true },
  [TOKEN_FALSE] =       { PREC_NONE,  PREC_NONE,        PREC_NONE,          PREC_NONE,  true },
  [TOKEN_IDENTIFIER] =  { PREC_NONE,  PREC_NONE,        PREC_NONE,          PREC_NONE,  true },
  [TOKEN_NUMBER] =      { PREC_NONE,  PREC_NONE,        PREC_NONE,          PREC_NONE,  true },
  [TOKEN_SQSTR] =       { PREC_NONE,  PREC_NONE,        PREC_NONE,          PREC_NONE,  true },
  [TOKEN_DQSTR] =       { PREC_NONE,  PREC_NONE,        PREC_NONE,          PREC_NONE,  true },
  [TOKEN_ASSIGN] =      { PREC_NONE,  PREC_ASSIGN_LEFT, PREC_ASSIGN_RIGHT,  PREC_NONE,  false },
  [TOKEN_OR] =          { PREC_NONE,  PREC_OR_LEFT,     PREC_OR_RIGHT,      PREC_NONE,  false },
  [TOKEN_AND] =         { PREC_NONE,  PREC_AND_LEFT,    PREC_AND_RIGHT,     PREC_NONE,  false },
  [TOKEN_EQ] =          { PREC_NONE,  PREC_CMP_LEFT,    PREC_CMP_RIGHT,     PREC_NONE,  false },
  [TOKEN_NEQ] =         { PREC_NONE,  PREC_CMP_LEFT,    PREC_CMP_RIGHT,     PREC_NONE,  false },
  [TOKEN_LEQ] =         { PREC_NONE,  PREC_CMP_LEFT,    PREC_CMP_RIGHT,     PREC_NONE,  false },
  [TOKEN_GEQ] =         { PREC_NONE,  PREC_CMP_LEFT,    PREC_CMP_RIGHT,     PREC_NONE,  false },
  [TOKEN_LT] =          { PREC_NONE,  PREC_CMP_LEFT,    PREC_CMP_RIGHT,     PREC_NONE,  false },
  [TOKEN_GT] =          { PREC_NONE,  PREC_CMP_LEFT,    PREC_CMP_RIGHT,     PREC_NONE,  false },
  [TOKEN_PLUS] =        { PREC_NONE,  PREC_ADD_LEFT,    PREC_ADD_RIGHT,     PREC_NONE,  false },
  [TOKEN_MINUS] =       { PREC_NEG,   PREC_ADD_LEFT,    PREC_ADD_RIGHT,     PREC_NONE,  false },
  [TOKEN_NOT] =         { PREC_NOT,   PREC_NONE,        PREC_NONE,          PREC_NONE,  false },
  [TOKEN_STAR] =        { PREC_NONE,  PREC_MUL_LEFT,    PREC_MUL_RIGHT,     PREC_NONE,  false },
  [TOKEN_SLASH] =       { PREC_NONE,  PREC_MUL_LEFT,    PREC_MUL_RIGHT,     PREC_NONE,  false },
  [TOKEN_DOT] =         { PREC_NONE,  PREC_DOT_LEFT,    PREC_DOT_RIGHT,     PREC_NONE,  false },
  [TOKEN_OPEN_PAREN] =  { PREC_NONE,  PREC_NONE,        PREC_NONE,          PREC_NONE,  false },
  [TOKEN_CLOSE_PAREN] = { PREC_NONE,  PREC_NONE,        PREC_NONE,          PREC_NONE,  false },
};

Node* parseInternal(Scanner* scanner, Precedence minimumBindingPower) {
  /*
   * This function is the core of the Pratt algorithm.
   *
   * We start by matching an atom. In the simplest case, we simply return this.
   * However, we call it the leftOperand, because in the basic case of the
   * parser, this becomes the left operand.
   */
  Token token = Scanner_scan(scanner);
  Node* leftOperand = NULL;

  if(token.type == TOKEN_EOF) {
    return NULL;
  } else if(PRECEDENCE_TABLE[token.type].isAtom) {
    leftOperand = makeAtomNode(token);
  } else if(token.type == TOKEN_IF) {
    return parseIf(scanner, token.line);
  } else if(token.type == TOKEN_OPEN_PAREN) {
    leftOperand = parseInternal(scanner, PREC_ANY);
    token = Scanner_scan(scanner);
    // TODO Handle this
    assert(token.type == TOKEN_CLOSE_PAREN);
  } else if(PRECEDENCE_TABLE[token.type].prefix > PREC_NONE) {
    Node* prefixOperand = parseInternal(
      scanner,
      PRECEDENCE_TABLE[token.type].prefix
    );

    // TODO Handle this
    assert(prefixOperand != NULL);

    leftOperand = makeUnaryNode(token, prefixOperand);
  }

  assert(leftOperand != NULL);

  /*
   * The basic case is an infix operation, which consists of a operator
   * between a left operand and a right operand. This is simple when
   * the operands are atoms (such as numbers), like "1 + 2".
   */

  for(;;) {
    Token operator = Scanner_peek(scanner);

    switch(operator.type) {
      case TOKEN_ELSE:
      case TOKEN_END:
      case TOKEN_EOF:
        return leftOperand;
      default:
        break;
    }

    Precedence leftBindingPower = PRECEDENCE_TABLE[operator.type].infixLeft;

    if(leftBindingPower < minimumBindingPower) {
      break;
    }

    Scanner_scan(scanner);

    Node* rightOperand = parseInternal(scanner, PRECEDENCE_TABLE[operator.type].infixRight);

    NodeType infixOperatorType;

    switch(operator.type) {
      #define MAP_INFIX(tokenType, nodeType) \
      case tokenType: infixOperatorType = nodeType; break
      MAP_INFIX(TOKEN_PLUS,   NODE_ADD);
      MAP_INFIX(TOKEN_MINUS,  NODE_SUBTRACT);
      MAP_INFIX(TOKEN_STAR,   NODE_MULTIPLY);
      MAP_INFIX(TOKEN_SLASH,  NODE_DIVIDE);
      MAP_INFIX(TOKEN_EQ,     NODE_EQUALS);
      MAP_INFIX(TOKEN_NEQ,    NODE_NOT_EQUALS);
      MAP_INFIX(TOKEN_GEQ,    NODE_GREATER_THAN_EQUALS);
      MAP_INFIX(TOKEN_LEQ,    NODE_LESS_THAN_EQUALS);
      MAP_INFIX(TOKEN_GT,     NODE_GREATER_THAN);
      MAP_INFIX(TOKEN_LT,     NODE_LESS_THAN);
      MAP_INFIX(TOKEN_ASSIGN, NODE_ASSIGN);
      MAP_INFIX(TOKEN_AND,    NODE_AND);
      MAP_INFIX(TOKEN_OR,     NODE_OR);
      MAP_INFIX(TOKEN_DOT,    NODE_PROPERTY);
      #undef MAP_INFIX
      default:
        printf("Unkown infix operator: \"%.*s\"", (int)operator.length, operator.text);
        exit(1);
    }

    leftOperand = makeBinaryNode(
        infixOperatorType,
        operator.line,
        leftOperand,
        rightOperand
    );
  }

  return leftOperand;
}

void Node_free(Node* self) {
  if(self == NULL) return;

  switch(self->type) {
    case NODE_IF:
      Node_free(((TernaryNode*)self)->arg2);
      // Don't break, cascade to further cleanup

    // Binary nodes
    case NODE_PROPERTY:
    case NODE_ADD:
    case NODE_SUBTRACT:
    case NODE_MULTIPLY:
    case NODE_DIVIDE:
    case NODE_EQUALS:
    case NODE_NOT_EQUALS:
    case NODE_GREATER_THAN_EQUALS:
    case NODE_LESS_THAN_EQUALS:
    case NODE_GREATER_THAN:
    case NODE_LESS_THAN:
    case NODE_AND:
    case NODE_OR:
    case NODE_ASSIGN:
      Node_free(((BinaryNode*)self)->arg1);
      // Don't break, cascade to further cleanup

    case NODE_NEGATE:
    case NODE_NOT:
    // Unary nodes
      Node_free(((UnaryNode*)self)->arg);
      // Don't break, cascade to further cleanup

    // Atom nodes
    case NODE_NIL:
    case NODE_TRUE:
    case NODE_FALSE:
    case NODE_IDENTIFIER:
    case NODE_NUMBER:
    case NODE_STRING:
      // Note that we don't clean up the text associated with atom nodes
      // This is because it contains pointers to source, which we will clean up once
      // the parse is done and code generated.
      break;

    default:
      assert(false);
  }

  free(self);
}

Node* parse(Scanner* scanner) {
  return parseInternal(scanner, PREC_ANY);
}
