#include "parser.h"
#include "common.h"
#include "tokenizer.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

Token Parser_expect_token(Parser *p, TokenTag tag) {
  Token tok = p->token_arr[p->pos++];
  if (tok.tag == tag) return tok;
  print_error(p->source, tok.start, tok.len,
    "Expected `%s`, got `%s`", TOK_NAMES[tag], TOK_NAMES[tok.tag]);
  exit(1);
}

NodeId parse(const char *source, const Token *tokens, Parser *p) {
  *p = (Parser){
    .source = source,
    .token_arr = tokens,
    .node_len = 3, // null node, start node, stop node
    .link_len = 1, // space for null link
  };
  p->node_arr[START_NODE] = (Node){ .tag = NODE_START };
  p->node_arr[STOP_NODE] = (Node){ .tag = NODE_STOP };

  Parser_parse_top_level(p);
}

void print_nodes(const Parser *p) {
  for (int i = 0; i < p->node_len; ++i) {
    Node node = p->node_arr[i];
    if (!node.tag) continue;
    printf("% 2d %s", i, NODE_NAME[node.tag]);
    switch (node.tag) {
      case NODE_PROJ:
        printf(" %d", node.value.proj.select);
        break;
      case NODE_CONSTANT:
        printf(" %d", node.value.i64);
        break;
      default:
    }
    printf(" [");
    LinkId id = node.outputs;
    while (id) {
      printf("%d", p->link_arr[id].node);
      if (!p->link_arr[id].next) break;
      putchar(' ');
      id = p->link_arr[id].next;
    }
    printf("]\n");
    if (node.tag == NODE_SCOPE) {
      for (int i = 0; i < node.value.scope.var_count; ++i) {
        VarId id = node.value.scope.var_start + i;
        Var var = p->var_arr[id];
        printf("  %.*s: %d\n", var.len, &p->source[var.start], var.node);
      }
    }
  }
  printf("\n");
}
