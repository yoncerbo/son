#include "parser.h"

uint8_t PRECEDENCE[NODE_BINARY_COUNT] = {
  [NODE_MUL - NODE_BINARY_START] = 10,
  [NODE_DIV - NODE_BINARY_START] = 10,
  [NODE_ADD - NODE_BINARY_START] = 9,
  [NODE_SUB - NODE_BINARY_START] = 9,
  [NODE_EQ - NODE_BINARY_START] = 7,
  [NODE_NE - NODE_BINARY_START] = 7,
  [NODE_LT - NODE_BINARY_START] = 7,
  [NODE_LE - NODE_BINARY_START] = 7,
  [NODE_GT - NODE_BINARY_START] = 7,
  [NODE_GE - NODE_BINARY_START] = 7,
};

NodeId Parser_parse_atom(Parser *p) {
  Token tok = p->token_arr[p->pos++];
  NodeId node;
  switch (tok.tag) {
    case TOK_TRUE:
      return Parser_create_constant(p, true);
    case TOK_FALSE:
      return Parser_create_constant(p, false);
    case TOK_IDENT:
      VarId var = Parser_resolve_var(p, tok.start, tok.len);
      node = p->var_arr[var].node;
      break;
    case TOK_LPAREN:
      node = Parser_parse_expression(p);
      Parser_expect_token(p, TOK_RPAREN);
      break;
    case TOK_DECIMAL:
      int64_t number = 0;
      for (int i = 0; i < tok.len; ++i) {
        number *= 10;
        number += p->source[tok.start + i] - '0';
      }
      return Parser_create_constant(p, number); 
    default:
      print_error(p->source, tok.start, tok.len,
        "Unexpected token while parsing atom: `%s`", TOK_NAMES[tok.tag]);
      exit(1);
  }
  return node;
}

NodeId Parser_parse_unary(Parser *p) {
  Token tok = p->token_arr[p->pos];
  NodeTag op;
  switch (tok.tag) {
    case TOK_MINUS: op = NODE_MINUS; break;
    case TOK_BANG: op = NODE_NOT; break;
    default: return Parser_parse_atom(p);
  }
  p->pos++;
  NodeId inner = Parser_parse_unary(p);
  if (p->node_arr[inner].tag != NODE_CONSTANT) {
    return Parser_create_unary_node(p, op, inner);
  }
  int64_t value = p->node_arr[inner].value.i64;
  Parser_remove_node(p, inner);
  switch (op) {
    case NODE_MINUS: value = -value; break;
    case NODE_NOT: value = !value; break;
    default: assert(0);
  }
  return Parser_create_constant(p, value);
}

NodeId Parser_parse_binary(Parser *p, uint8_t prec) {
  NodeId left = Parser_parse_unary(p);
  Token tok;
  NodeTag op;
  uint8_t new_prec;
  while (1) {
    tok = p->token_arr[p->pos];
    switch (tok.tag) {
      case TOK_PLUS: op = NODE_ADD; break;
      case TOK_MINUS: op = NODE_SUB; break;
      case TOK_STAR: op = NODE_MUL; break;
      case TOK_SLASH: op = NODE_DIV; break;
      case TOK_DEQ: op = NODE_EQ; break;
      case TOK_NE: op = NODE_NE; break;
      case TOK_LT: op = NODE_LT; break;
      case TOK_LE: op = NODE_LE; break;
      case TOK_GT: op = NODE_GT; break;
      case TOK_GE: op = NODE_GE; break;
      default: return left;
    }
    new_prec = PRECEDENCE[op - NODE_BINARY_START];
    if (new_prec < prec) break;
    p->pos++;
    NodeId right = Parser_parse_binary(p, new_prec);
    if (p->node_arr[left].tag != NODE_CONSTANT
        || p->node_arr[right].tag != NODE_CONSTANT) {
      left = Parser_create_binary_node(p, op, left, right);
      continue;
    }
    int64_t l = p->node_arr[left].value.i64;
    int64_t r = p->node_arr[right].value.i64;
    Parser_remove_node(p, right);
    Parser_remove_node(p, left);
    switch (op) {
      case NODE_ADD: l += r; break;
      case NODE_SUB: l -= r; break;
      case NODE_MUL: l *= r; break;
      case NODE_DIV: l /= r; break;
      case NODE_EQ: l = l == r; break;
      case NODE_NE: l = l != r; break;
      case NODE_GE: l = l >= r; break;
      case NODE_GT: l = l > r; break;
      case NODE_LE: l = l <= r; break;
      case NODE_LT: l = l < r; break;
      default: assert(0);
    }
    left = Parser_create_constant(p, l);
  }
  return left;
}

NodeId Parser_parse_expression(Parser *p) {
  NodeId value = Parser_parse_binary(p, 0);
  return value;
}

