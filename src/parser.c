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
#include <stdint.h>

void NodeType_print(NodeType type) {
  switch(type) {
    #define MAP(t) case t: printf(#t); break
      MAP(NODE_NIL);
      MAP(NODE_TRUE);
      MAP(NODE_FALSE);
      MAP(NODE_IDENTIFIER);
      MAP(NODE_NUMBER);
      MAP(NODE_STRING);
      MAP(NODE_NEGATE);
      MAP(NODE_NOT);
      MAP(NODE_PROPERTY);
      MAP(NODE_ADD);
      MAP(NODE_SUBTRACT);
      MAP(NODE_MULTIPLY);
      MAP(NODE_DIVIDE);
      MAP(NODE_EQUALS);
      MAP(NODE_NOT_EQUALS);
      MAP(NODE_GREATER_THAN_EQUALS);
      MAP(NODE_LESS_THAN_EQUALS);
      MAP(NODE_GREATER_THAN);
      MAP(NODE_LESS_THAN);
      MAP(NODE_AND);
      MAP(NODE_OR);
      MAP(NODE_ASSIGN);
      MAP(NODE_WHILE);
      MAP(NODE_CALL);
      MAP(NODE_FN_DEF);
      MAP(NODE_IF);
      MAP(NODE_COMMA_SEPARATED_LIST);
      MAP(NODE_EXPRESSION_LIST);
    #undef MAP
    default:
      assert(false);
  }

  fflush(stdout);
}

inline static void Node_init(Node* self, NodeType type, size_t line) {
  self->type = type;
  self->line = line;
}


static Node makeNode(NodeType type, size_t line) {
  Node result;
  Node_init(&result, type, line);
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

  AtomNode* node = AtomNode_allocateOne();
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

  UnaryNode* node = UnaryNode_allocateOne();
  node->node = makeNode(type, operator.line);
  node->arg = arg;
  return (Node*)node;
}

static Node* makeBinaryNode(NodeType type, size_t line, Node* arg0, Node* arg1) {
  BinaryNode* node = BinaryNode_allocateOne();
  node->node = makeNode(type, line);
  node->arg0 = arg0;
  node->arg1 = arg1;
  return (Node*)node;
}

static Node* makeTernaryNode(NodeType type, size_t line, Node* arg0, Node* arg1, Node* arg2) {
  TernaryNode* node = TernaryNode_allocateOne();
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
} PrecedenceRule;

Node* parseExpression(Scanner*, Precedence minimumBindingPower);
Node* parseExpressionList(Scanner*, uint8_t, TokenType*);

static void ExpressionListNode_init(ExpressionListNode* self) {
  self->length = 0;
  self->capacity = 0;
  self->items = 0;
}

static void ExpressionListNode_append(ExpressionListNode* self, Node* item) {
  /*
   * TODO Combine this with other dynamic arrays.
   */
  if(self->length == self->capacity) {
    if(self->capacity == 0) {
      self->capacity = 8;
    } else {
      self->capacity = self->capacity * 2;
    }

    self->items = realloc(self->items, sizeof(Node*) * self->capacity);

    assert(self->items != NULL); // TODO Handle this
  }

  self->items[self->length] = item;
  self->length++;
}

static void ExpressionListNode_snug(ExpressionListNode* self) {
  /*
   * If we know that we won't be adding any more items to a dynamic array,
   * we can free the excess space we have allocated.
   */

  if(self->capacity > self->length) {
    self->capacity = self->length;
    self->items = realloc(self->items, self->capacity * sizeof(Node*));
    assert(self->items != NULL); // TODO Handle this
  }
}

static Node* parseCall(Scanner* scanner, size_t line) {
  Token start = Scanner_scan(scanner);
  assert(start.type == TOKEN_OPEN_PAREN);

  ExpressionListNode* elNode = ExpressionListNode_allocateOne();

  Node_init((Node*)elNode, NODE_COMMA_SEPARATED_LIST, line);
  ExpressionListNode_init(elNode);

  Token close = Scanner_peek(scanner);

  // No arguments
  if(close.type == TOKEN_CLOSE_PAREN) {
    /*
     * No need to call ExpressionListNode_snug because this is instantiated
     * with a null items pointer.
     */
    return (Node*)elNode;
  }

  for(;;) {
    Node* expr = parseExpression(scanner, PREC_ANY);

    ExpressionListNode_append(elNode, expr);

    close = Scanner_scan(scanner);

    if(close.type == TOKEN_CLOSE_PAREN) {
      ExpressionListNode_snug(elNode);
      return (Node*)elNode;
    } else {
      assert(close.type == TOKEN_COMMA);
    }
  }
}

Node* parseFunctionDefinition(Scanner* scanner, size_t line) {
  Token token = Scanner_scan(scanner);
  assert(token.type == TOKEN_IDENTIFIER);
  Node* name = makeAtomNode(token);

  token = Scanner_peek(scanner);
  assert(token.type == TOKEN_OPEN_PAREN);

  /*
   * TODO There are some inefficiencies introduced by this function,
   * which is also used to parse argument lists for function calls.
   * We want some code reuse here, but let's try and optimize it.
   */
  /*
   * TODO Rename this function.
   */
  Node* arguments = parseCall(scanner, line);

  for(size_t i = 0; i < ((ExpressionListNode*)arguments)->length; i++) {
    Node* a = ((ExpressionListNode*)arguments)->items[i];
    assert(a->type == NODE_IDENTIFIER);
  }

  token = Scanner_scan(scanner);
  assert(token.type == TOKEN_CLOSE_PAREN);

  token = Scanner_scan(scanner);
  assert(token.type == TOKEN_COLON);

  TokenType expectedExit = TOKEN_END;

  Node* body = parseExpressionList(scanner, 1, &expectedExit);

  token = Scanner_scan(scanner);
  assert(token.type == TOKEN_END);

  return makeTernaryNode(NODE_FN_DEF, line, name, arguments, body);
}

Node* parseIf(Scanner* scanner, size_t line) {
  // TODO Can we set a precedence that ensures this is a boolean?
  Node* test = parseExpression(scanner, PREC_ANY);

  Token token = Scanner_scan(scanner);
  assert(token.type == TOKEN_COLON);

  TokenType leftBranchExpectedExits[] = {
    TOKEN_ELSE,
    TOKEN_END
  };

  Node* leftBranch = parseExpressionList(scanner, 2, leftBranchExpectedExits);

  Node* rightBranch = NULL;

  token = Scanner_scan(scanner);
  assert(token.type == TOKEN_ELSE || token.type == TOKEN_END);

  if(token.type == TOKEN_ELSE) {
    TokenType rightBranchExpecteExit = TOKEN_END;

    rightBranch = parseExpressionList(scanner, 1, &rightBranchExpecteExit);

    token = Scanner_scan(scanner);
  }

  assert(token.type == TOKEN_END);

  return makeTernaryNode(NODE_IF, line, test, leftBranch, rightBranch);
}

Node* parseWhile(Scanner* scanner, size_t line) {
  // TODO Can we set a precedence that ensures this is a boolean?
  Node* test = parseExpression(scanner, PREC_ANY);

  Token token = Scanner_scan(scanner);
  assert(token.type == TOKEN_COLON);

  TokenType expectedExit = TOKEN_END;

  Node* body = parseExpressionList(scanner, 1, &expectedExit);

  token = Scanner_scan(scanner);
  assert(token.type == TOKEN_END);

  return makeBinaryNode(NODE_WHILE, line, test, body);
}

const static PrecedenceRule PRECEDENCE_TABLE[] = {
// TokenType              Prefix,     InfixLeft,        InfixRight
  [TOKEN_NIL] =         { PREC_NONE,  PREC_NONE,        PREC_NONE         },
  [TOKEN_TRUE] =        { PREC_NONE,  PREC_NONE,        PREC_NONE         },
  [TOKEN_FALSE] =       { PREC_NONE,  PREC_NONE,        PREC_NONE         },
  [TOKEN_IDENTIFIER] =  { PREC_NONE,  PREC_NONE,        PREC_NONE         },
  [TOKEN_NUMBER] =      { PREC_NONE,  PREC_NONE,        PREC_NONE         },
  [TOKEN_SQSTR] =       { PREC_NONE,  PREC_NONE,        PREC_NONE         },
  [TOKEN_DQSTR] =       { PREC_NONE,  PREC_NONE,        PREC_NONE         },
  [TOKEN_ASSIGN] =      { PREC_NONE,  PREC_ASSIGN_LEFT, PREC_ASSIGN_RIGHT },
  [TOKEN_OR] =          { PREC_NONE,  PREC_OR_LEFT,     PREC_OR_RIGHT     },
  [TOKEN_AND] =         { PREC_NONE,  PREC_AND_LEFT,    PREC_AND_RIGHT    },
  [TOKEN_EQ] =          { PREC_NONE,  PREC_CMP_LEFT,    PREC_CMP_RIGHT    },
  [TOKEN_NEQ] =         { PREC_NONE,  PREC_CMP_LEFT,    PREC_CMP_RIGHT    },
  [TOKEN_LEQ] =         { PREC_NONE,  PREC_CMP_LEFT,    PREC_CMP_RIGHT    },
  [TOKEN_GEQ] =         { PREC_NONE,  PREC_CMP_LEFT,    PREC_CMP_RIGHT    },
  [TOKEN_LT] =          { PREC_NONE,  PREC_CMP_LEFT,    PREC_CMP_RIGHT    },
  [TOKEN_GT] =          { PREC_NONE,  PREC_CMP_LEFT,    PREC_CMP_RIGHT    },
  [TOKEN_PLUS] =        { PREC_NONE,  PREC_ADD_LEFT,    PREC_ADD_RIGHT    },
  [TOKEN_MINUS] =       { PREC_NEG,   PREC_ADD_LEFT,    PREC_ADD_RIGHT    },
  [TOKEN_NOT] =         { PREC_NOT,   PREC_NONE,        PREC_NONE         },
  [TOKEN_STAR] =        { PREC_NONE,  PREC_MUL_LEFT,    PREC_MUL_RIGHT    },
  [TOKEN_SLASH] =       { PREC_NONE,  PREC_MUL_LEFT,    PREC_MUL_RIGHT    },
  [TOKEN_DOT] =         { PREC_NONE,  PREC_DOT_LEFT,    PREC_DOT_RIGHT    },
  [TOKEN_OPEN_PAREN] =  { PREC_NONE,  PREC_NONE,        PREC_NONE         },
  [TOKEN_CLOSE_PAREN] = { PREC_NONE,  PREC_NONE,        PREC_NONE         },
  [TOKEN_IF] =          { PREC_NONE,  PREC_NONE,        PREC_NONE         },
  [TOKEN_WHILE] =       { PREC_NONE,  PREC_NONE,        PREC_NONE         },
};

Node* parseExpression(Scanner* scanner, Precedence minimumBindingPower) {
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
    case TOKEN_EOF:
      return NULL;

    /* Atoms */
    case TOKEN_NIL:
    case TOKEN_TRUE:
    case TOKEN_FALSE:
    case TOKEN_IDENTIFIER:
    case TOKEN_NUMBER:
    case TOKEN_SQSTR:
    case TOKEN_DQSTR:
      leftOperand = makeAtomNode(token);
      break;

    case TOKEN_DEF:
      return parseFunctionDefinition(scanner, token.line);

    case TOKEN_IF:
      return parseIf(scanner, token.line);

    case TOKEN_OPEN_PAREN:
      leftOperand = parseExpression(scanner, PREC_ANY);
      token = Scanner_scan(scanner);
      // TODO Handle this
      assert(token.type == TOKEN_CLOSE_PAREN);
      break;

    case TOKEN_WHILE:
      return parseWhile(scanner, token.line);

    default:
      {
        if(PRECEDENCE_TABLE[token.type].prefix > PREC_NONE) {
          Node* prefixOperand = parseExpression(
              scanner,
              PRECEDENCE_TABLE[token.type].prefix
            );

          // TODO Handle this
          assert(prefixOperand != NULL);

          leftOperand = makeUnaryNode(token, prefixOperand);
        }
      } break;
  }

  if(leftOperand == NULL) {
    printf(
        "Unable to parse %s on line %zu\n",
        TokenType_asString(token.type),
        token.line
      );
    fflush(stdout);
    assert(false);
  }

  /*
   * The basic case is an infix operation, which consists of a operator
   * between a left operand and a right operand. This is simple when
   * the operands are atoms (such as numbers), like "1 + 2".
   */

  for(;;) {
    Token operator = Scanner_peek(scanner);

    switch(operator.type) {
      case TOKEN_OPEN_PAREN:
        {
          Node* arguments = parseCall(scanner, leftOperand->line);
          BinaryNode* call = BinaryNode_allocateOne();
          Node_init((Node*)call, NODE_CALL, leftOperand->line);

          call->arg0 = leftOperand;
          call->arg1 = arguments;
          leftOperand = (Node*)call;

          continue;
        }
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

    Node* rightOperand = parseExpression(scanner, PRECEDENCE_TABLE[operator.type].infixRight);

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
    case NODE_FN_DEF:
      Node_free(((TernaryNode*)self)->arg2);
      // Don't break, cascade to further cleanup

    // Binary nodes
    case NODE_ADD:
    case NODE_AND:
    case NODE_ASSIGN:
    case NODE_CALL:
    case NODE_DIVIDE:
    case NODE_EQUALS:
    case NODE_GREATER_THAN:
    case NODE_GREATER_THAN_EQUALS:
    case NODE_LESS_THAN:
    case NODE_LESS_THAN_EQUALS:
    case NODE_MULTIPLY:
    case NODE_NOT_EQUALS:
    case NODE_OR:
    case NODE_PROPERTY:
    case NODE_SUBTRACT:
    case NODE_WHILE:
      Node_free(((BinaryNode*)self)->arg1);
      // Don't break, cascade to further cleanup

    case NODE_NEGATE:
    case NODE_NOT:
    // Unary nodes
      Node_free(((UnaryNode*)self)->arg);
      // Don't break, cascade to further cleanup

    // Atom nodes
    case NODE_FALSE:
    case NODE_IDENTIFIER:
    case NODE_NIL:
    case NODE_NUMBER:
    case NODE_STRING:
    case NODE_TRUE:
      // Note that we don't clean up the text associated with atom nodes
      // This is because it contains pointers to source, which we will clean up once
      // the parse is done and code generated.
      break;

    case NODE_COMMA_SEPARATED_LIST:
    case NODE_EXPRESSION_LIST:
      {
        ExpressionListNode* el = (ExpressionListNode*)self;

        for(size_t i = 0; i < el->length; i++) {
          Node_free(el->items[i]);
        }
        free(el->items);
      } break;

    default:
      assert(false);
  }

  free(self);
}

Node* parseExpressionList(Scanner* scanner, uint8_t expectedExitCount, TokenType* expectedExits) {
  /*
   * TODO Don't return an ExpressionList if it would contain only one
   * node. Instead just return the one node.
   */
  ExpressionListNode* node = ExpressionListNode_allocateOne();
  ExpressionListNode_init(node);

  Token token = Scanner_peek(scanner);
  Node_init((Node*)node, NODE_EXPRESSION_LIST, token.line);

  for(;;) {
    switch(token.type) {
      case TOKEN_ELSE:
      case TOKEN_END:
      case TOKEN_EOF:
        for(uint8_t i = 0; i < expectedExitCount; i++) {
          if(token.type == expectedExits[i]) {
            ExpressionListNode_snug(node);
            return (Node*)node;
          }
        }
        assert(false);

      default:
        ExpressionListNode_append(node, parseExpression(scanner, PREC_ANY));
        token = Scanner_peek(scanner);
    }
  }
}

Node* parse(Scanner* scanner) {
  TokenType exit = TOKEN_EOF;

  return parseExpressionList(scanner, 1, &exit);
}
