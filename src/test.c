#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.c"
#include "parser.h"
#include "tokenizer.c"
#include "parser.c"
#include "tokenizer.h"

int main(int argc, char *argv[]) {
  const char *source = "return 12;";
  Token *tokens = malloc(sizeof(*tokens) * MAX_TOKENS);
  tokenize(source, tokens);
  Parser *p = malloc(sizeof(*p));
  NodeId ret = parse(source, tokens, p);

  assert(p->node_arr[ret].tag == NODE_RETURN);
  assert(p->node_arr[ret].value.ret.predecessor == START_NODE);
  NodeId value = p->node_arr[ret].value.ret.value;
  assert(p->node_arr[value].tag = NODE_CONSTANT);
  assert(p->node_arr[value].value.i64 == 12);

  fprintf(stderr, "Tests finished succesfully\n");

  source = "return 3 + 3 * 3;";
  tokenize(source, tokens);
  ret = parse(source, tokens, p);

  assert(p->node_arr[ret].tag == NODE_RETURN);
  assert(p->node_arr[ret].value.ret.predecessor == START_NODE);
  value = p->node_arr[ret].value.ret.value;
  assert(p->node_arr[value].tag = NODE_CONSTANT);
  assert(p->node_arr[value].value.i64 == 12);

  source = "return 3 * 3 + 3;";
  tokenize(source, tokens);
  ret = parse(source, tokens, p);

  assert(p->node_arr[ret].tag == NODE_RETURN);
  assert(p->node_arr[ret].value.ret.predecessor == START_NODE);
  value = p->node_arr[ret].value.ret.value;
  assert(p->node_arr[value].tag = NODE_CONSTANT);
  assert(p->node_arr[value].value.i64 == 12);
}
