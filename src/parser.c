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
    case TOKEN_MINUS:
      type = NODE_NEGATE;
      break;

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

typedef enum {
  // This precedence rule should never match
  PREC_NONE,

  /*
   * Left before right means left < right, because enum values are chosen sequentially
   * Left < right indicates left associative, i.e. 1 + 2 + 3 => (+ (+ 1 2) 3)
   * This is because when we check the precedence it will give a lower binding power.
   */
  PREC_ADD_LEFT,
  PREC_ADD_RIGHT,

  // Left associative
  PREC_MUL_LEFT,
  PREC_MUL_RIGHT,

  PREC_NEG,
} Precedence;

typedef struct {
  Precedence prefix;
  Precedence infixLeft;
  Precedence infixRight;
  Precedence postfix;
  bool isAtom; // Smallest item last for struct packing
} PrecedenceRule;

const static PrecedenceRule PRECEDENCE_TABLE[] = {
// TokenType             Prefix,    InfixLeft,     InfixRight,      Postfix,    isAtom
  [TOKEN_NIL] =        { PREC_NONE, PREC_NONE,     PREC_NONE,       PREC_NONE,  true },
  [TOKEN_TRUE] =       { PREC_NONE, PREC_NONE,     PREC_NONE,       PREC_NONE,  true },
  [TOKEN_FALSE] =      { PREC_NONE, PREC_NONE,     PREC_NONE,       PREC_NONE,  true },
  [TOKEN_IDENTIFIER] = { PREC_NONE, PREC_NONE,     PREC_NONE,       PREC_NONE,  true },
  [TOKEN_NUMBER] =     { PREC_NONE, PREC_NONE,     PREC_NONE,       PREC_NONE,  true },
  [TOKEN_PLUS] =       { PREC_NONE, PREC_ADD_LEFT, PREC_ADD_RIGHT,  PREC_NONE,  false },
  [TOKEN_MINUS] =      { PREC_NEG,  PREC_ADD_LEFT, PREC_ADD_RIGHT,  PREC_NONE,   false },
  [TOKEN_STAR] =       { PREC_NONE, PREC_MUL_LEFT, PREC_MUL_RIGHT,  PREC_NONE,  false },
  [TOKEN_SLASH] =      { PREC_NONE, PREC_MUL_LEFT, PREC_MUL_RIGHT,  PREC_NONE,  false },
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

    if(operator.type == TOKEN_EOF) return leftOperand;

    Precedence leftBindingPower = PRECEDENCE_TABLE[operator.type].infixLeft;

    if(leftBindingPower < minimumBindingPower) {
      break;
    }

    Scanner_scan(scanner);

    Node* rightOperand = parseInternal(scanner, PRECEDENCE_TABLE[operator.type].infixRight);

    NodeType infixOperatorType;

    switch(operator.type) {
      case TOKEN_PLUS:
        infixOperatorType = NODE_ADD;
        break;
      case TOKEN_MINUS:
        infixOperatorType = NODE_SUBTRACT;
        break;
      case TOKEN_STAR:
        infixOperatorType = NODE_MULTIPLY;
        break;
      case TOKEN_SLASH:
        infixOperatorType = NODE_DIVIDE;
        break;
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
  switch(self->type) {
    // TODO Clean up arg2 for ternary node types here

    // Binary nodes
    case NODE_ADD:
    case NODE_SUBTRACT:
    case NODE_MULTIPLY:
    case NODE_DIVIDE:
      Node_free(((BinaryNode*)self)->arg1);
      // Don't break, cascade to further cleanup

    case NODE_NEGATE:
    // Unary nodes
      Node_free(((UnaryNode*)self)->arg);
      // Don't break, cascade to further cleanup

    // Atom nodes
    case NODE_NIL:
    case NODE_TRUE:
    case NODE_FALSE:
    case NODE_IDENTIFIER:
    case NODE_NUMBER:
      // Note that we don't clean up the text associated with atom nodes
      // This is because it contains pointers to source, which we will clean up once
      // the parse is done and code generated.

      // TODO Free the source once parse is done and code is generated. :)
      break;

    default:
      assert(false);
  }

  free(self);
}

Node* parse(Scanner* scanner) {
  return parseInternal(scanner, PREC_NONE);
}
