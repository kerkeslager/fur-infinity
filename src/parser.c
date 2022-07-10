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

static Node makeNode(NodeType type, size_t line) {
  Node result;
  result.type = type;
  result.line = line;
  return result;
}

static Node* makeAtomNode(NodeType type, size_t line, char* text, size_t length) {
  AtomNode* node = malloc(sizeof(AtomNode));
  node->node = makeNode(type, line);
  node->text = text;
  node->length = length;
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
} Precedence;

typedef struct {
  Precedence prefix;
  Precedence infixLeft;
  Precedence infixRight;
  Precedence postfix;
  bool isAtom; // Smallest item last for struct packing
} PrecedenceRule;

const static PrecedenceRule PRECEDENCE_TABLE[] = {
// TokenType             Prefix,    InfixLeft,     InfixRight,      Postfix,   isAtom
  [TOKEN_NIL] =        { PREC_NONE, PREC_NONE,     PREC_NONE,       PREC_NONE, true },
  [TOKEN_TRUE] =       { PREC_NONE, PREC_NONE,     PREC_NONE,       PREC_NONE, true },
  [TOKEN_FALSE] =      { PREC_NONE, PREC_NONE,     PREC_NONE,       PREC_NONE, true },
  [TOKEN_IDENTIFIER] = { PREC_NONE, PREC_NONE,     PREC_NONE,       PREC_NONE, true },
  [TOKEN_NUMBER] =     { PREC_NONE, PREC_NONE,     PREC_NONE,       PREC_NONE, true },
  [TOKEN_PLUS] =       { PREC_NONE, PREC_ADD_LEFT, PREC_ADD_RIGHT,  PREC_NONE, false },
  [TOKEN_MINUS] =      { PREC_NONE, PREC_ADD_LEFT, PREC_ADD_RIGHT,  PREC_NONE, false },
  [TOKEN_STAR] =       { PREC_NONE, PREC_MUL_LEFT, PREC_MUL_RIGHT,  PREC_NONE, false },
  [TOKEN_SLASH] =      { PREC_NONE, PREC_NONE,     PREC_NONE,       PREC_NONE, false },
};

Node* parse(Scanner* scanner, Precedence minimumBindingPower) {
  /*
   * This function is the core of the Pratt algorithm.
   *
   * We start by matching an atom. In the simplest case, we simply return this.
   * However, we call it the leftOperand, because in the basic case of the
   * parser, this becomes the left operand.
   */
  Token token = Scanner_scan(scanner);
  Node* leftOperand = NULL;

  switch(token.type) {
    case TOKEN_NIL:
      leftOperand = makeAtomNode(NODE_NIL, token.line, token.text, token.length);
    case TOKEN_TRUE:
      leftOperand = makeAtomNode(NODE_TRUE, token.line, token.text, token.length);
    case TOKEN_FALSE:
      leftOperand = makeAtomNode(NODE_FALSE, token.line, token.text, token.length);
    case TOKEN_IDENTIFIER:
      leftOperand = makeAtomNode(NODE_IDENTIFIER, token.line, token.text, token.length);
    case TOKEN_NUMBER:
      leftOperand = makeAtomNode(NODE_NUMBER, token.line, token.text, token.length);
    case TOKEN_EOF:
      return NULL;

    default:
      assert(false); // TODO Better handling.
  }

  /*
   * The basic case is an infix operation, which consists of a operator
   * between a left operand and a right operand. This is simple when
   * the operands are atoms (such as numbers), like "1 + 2".
   */

  for(;;) {
    Token operator = Scanner_scan(scanner);

    Precedence leftBindingPower = PRECEDENCE_TABLE[operator.type].infixLeft;

    if(leftBindingPower < minimumBindingPower) {
      break;
    }

    Node* rightOperand = parse(scanner, PRECEDENCE_TABLE[operator.type].infixRight);

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
        assert(false); // TODO Handle this better
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
