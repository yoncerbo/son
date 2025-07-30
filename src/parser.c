#include "parser.h"
#include "common.h"
#include "tokenizer.h"
#include <assert.h>
#include <stdint.h>

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

NodeId Parser_parse_expression(Parser *p) {
  Token tok = Parser_expect_token(p, TOK_DECIMAL);

  int64_t number = 0;
  for (int i = 0; i < tok.len; ++i) {
    number *= 10;
    number += p->source[tok.start + i] - '0';
  }

  NodeId node = Parser_create_node(p, (Node){
    .tag = NODE_CONSTANT,
    .value.i64 = number,
  });
  Parser_add_node_output(p, START_NODE, node);
  return node;
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
