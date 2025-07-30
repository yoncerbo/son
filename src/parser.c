#include "parser.h"
#include "common.h"
#include "tokenizer.h"
#include <assert.h>
#include <stdint.h>

uint8_t PRECEDENCE[NODE_BINARY_COUNT] = {
  [NODE_MUL - NODE_BINARY_START] = 10,
  [NODE_DIV - NODE_BINARY_START] = 10,
  [NODE_ADD - NODE_BINARY_START] = 9,
  [NODE_SUB - NODE_BINARY_START] = 9,
};

static inline Token Parser_expect_token(Parser *p, TokenTag tag) {
  Token tok = p->token_arr[p->pos++];
  if (tok.tag == tag) return tok;
  report_error(p->source, tok.start, tok.len,
    "Expected `%s`, got `%s`", TOK_NAMES[tag], TOK_NAMES[tok.tag]);
}

NodeId Parser_create_node(Parser *p, Node node) {
  assert(p->node_len < MAX_NODES);
  NodeId id = p->node_len++;
  p->node_arr[id] = node;
  return id;
}

void Parser_add_node_output(Parser *p, NodeId node, NodeId output) {
  assert(p->link_len < MAX_LINKS);
  LinkId id = p->link_len++;
  LinkId prev = p->node_arr[node].outputs;
  p->link_arr[id] = (Link){ prev, output };
  p->node_arr[node].outputs = id;
}

NodeId Parser_parse_atom(Parser *p) {
  Token tok = Parser_expect_token(p, TOK_DECIMAL);

  int64_t number = 0;
  for (int i = 0; i < tok.len; ++i) {
    number *= 10;
    number += p->source[tok.start + i] - '0';
  }

  NodeId node = Parser_create_node(p, (Node){
    .tag = NODE_CONSTANT,
    .value.i64 = number,
    .type = TYPE_INT,
  });
  // Parser_add_node_output(p, START_NODE, node);
  return node;
}

NodeId Parser_parse_unary(Parser *p) {
  Token tok = p->token_arr[p->pos];
  NodeTag nt;
  switch (tok.tag) {
    case TOK_MINUS:
      nt = NODE_MINUS;
      break;
    default:
      return Parser_parse_atom(p);
  }
  p->pos++;
  NodeId inner = Parser_parse_unary(p);
  // Fold the constant
  if (p->node_arr[inner].tag == NODE_CONSTANT) {
    // Shouldn't be used by anything
    assert(p->node_arr[inner].outputs == NULL_LINK);
    int64_t value = p->node_arr[inner].value.i64;
    p->node_arr[inner].value.i64 = -value;
    return inner;
  }
  NodeId node = Parser_create_node(p, (Node){
    .tag = nt,
    .value.unary.node = inner,
  });
  Parser_add_node_output(p, inner, node);
  return node;
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
      default: return left;
    }
    new_prec = PRECEDENCE[op - NODE_BINARY_START];
    if (new_prec < prec) break;
    p->pos++;
    assert(left == p->node_len - 1);
    assert(p->node_arr[left].outputs == NULL_LINK);
    assert(p->node_arr[left].tag == NODE_CONSTANT);
    NodeId right = Parser_parse_binary(p, new_prec);
    assert(right == p->node_len - 1);
    assert(p->node_arr[right].outputs == NULL_LINK);
    assert(p->node_arr[right].tag == NODE_CONSTANT);
    uint64_t l = p->node_arr[left].value.i64;
    uint64_t r = p->node_arr[right].value.i64;
    switch (op) {
      case NODE_ADD: l += r; break;
      case NODE_SUB: l -= r; break;
      case NODE_MUL: l *= r; break;
      case NODE_DIV: l /= r; break;
      default: assert(0);
    }
    p->node_len--;
    p->node_arr[left].value.i64 = l;
  }
  return left;
}

NodeId Parser_parse_expression(Parser *p) {
  NodeId value = Parser_parse_binary(p, 0);
  if (p->node_arr[value].tag == NODE_CONSTANT) {
    Parser_add_node_output(p, START_NODE, value);
  }
  return value;
}

NodeId Parser_parse_statement(Parser *p) {
  Parser_expect_token(p, TOK_RETURN);
  NodeId value = Parser_parse_expression(p);
  Parser_expect_token(p, TOK_SEMICOLON);

  NodeId node = Parser_create_node(p, (Node){
    .tag = NODE_RETURN,
    .value.ret.predecessor = START_NODE,
    .value.ret.value = value,
  });
  Parser_add_node_output(p, START_NODE, node);
  Parser_add_node_output(p, value, node);
  return node;
}

NodeId parse(const char *source, const Token *tokens, Parser *p) {
  *p = (Parser){
    .source = source,
    .token_arr = tokens,
    .node_len = 1, // skip start node
    .link_len = 1, // space for null link
  };
  p->node_arr[START_NODE] = (Node){ .tag = NODE_START };

  Parser_parse_statement(p);
}

void print_nodes(const Parser *p) {
  for (int i = 0; i < p->node_len; ++i) {
    Node node = p->node_arr[i];
    printf("% 2d %s [", i, NODE_NAME[node.tag]);
    switch (node.tag) {
      case NODE_CONSTANT:
        printf("%d", START_NODE);
        break;
      case NODE_RETURN:
        printf("%d ", node.value.ret.predecessor);
        printf("%d", node.value.ret.value);
        break;
      case NODE_START:
      default:
    }
    printf("] [");
    LinkId id = node.outputs;
    while (id) {
      printf("%d", p->link_arr[id].node);
      if (!p->link_arr[id].next) break;
      putchar(' ');
      id = p->link_arr[id].next;
    }
    printf("]\n");
  }
}
