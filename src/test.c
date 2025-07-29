#include <assert.h>
#include <stdlib.h>

#include "common.c"
#include "parser.h"
#include "tokenizer.c"
#include "parser.c"

int main(int argc, char *argv[]) {
  const char *source = "return 12;";
  Token *tokens = malloc(sizeof(*tokens) * MAX_TOKENS);
  tokenize(source, tokens);
  Parser *p = malloc(sizeof(*p));
  NodeId ret = parse(source, tokens, p);

  assert(p->node_arr[ret].tag == NODE_RETURN);
  LinkId in = p->node_arr[ret].inputs;
  assert(in);
  NodeId node = p->link_arr[in].node;
  assert(p->node_arr[node].tag == NODE_CONSTANT);
  in = p->link_arr[in].next;
  assert(in);
  assert(p->link_arr[in].node == START_NODE);
  in = p->link_arr[in].next;
  assert(in == NULL_LINK);
}
