#include "parser.h"
#include "common.h"
#include "tokenizer.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

uint8_t PRECEDENCE[NODE_BINARY_COUNT] = {
  [NODE_MUL - NODE_BINARY_START] = 10,
  [NODE_DIV - NODE_BINARY_START] = 10,
  [NODE_ADD - NODE_BINARY_START] = 9,
  [NODE_SUB - NODE_BINARY_START] = 9,
};

NodeId Parser_create_node(Parser *p, Node node);
void Parser_add_node_output(Parser *p, NodeId user, NodeId used);
void Parser_remove_node(Parser *p, NodeId id);
NodeId Parser_parse_expression(Parser *p);

void Parser_remove_output_node(Parser *p, NodeId user, NodeId used) {
  LinkId prev = 0;
  LinkId link = p->node_arr[used].outputs;
  while (link) {
    if (p->link_arr[link].node != user) {
      prev = link;
      link = p->link_arr[link].next;
      continue;
    }
    // TODO: reuse the link
    if (!prev) p->node_arr[used].outputs = p->link_arr[link].next;
    else p->link_arr[prev].next = p->link_arr[link].next;
    if (!p->node_arr[used].outputs) Parser_remove_node(p, used);
    return;
  }
  assert(0);
}

// Removes node only if it's unused
void Parser_remove_node(Parser *p, NodeId id) {
  if (p->node_arr[id].outputs) return;
  if (id == p->node_len - 1) p->node_len--;
  // TODO: else add to free list
  Node node = p->node_arr[id];
  switch (node.tag) {
    case NODE_START:
      break;
    case NODE_CONSTANT:
      // Parser_remove_output_node(p, START_NODE, id);
      break;
    case NODE_MINUS:
      Parser_remove_output_node(p, node.value.unary.node, id);
      break;
    case NODE_RETURN:
    case NODE_ADD:
    case NODE_SUB:
    case NODE_MUL:
    case NODE_DIV:
      Parser_remove_output_node(p, node.value.binary.left, id);
      Parser_remove_output_node(p, node.value.binary.right, id);
      break;
    case NODE_SCOPE:
      // NOTE: you can only remove top most scope
      assert(id == p->scope);
      uint16_t var_count = node.value.scope.var_count;
      for (int i = 0; i < var_count; i++) {
        Var entry = p->var_arr[p->var_len - i - 1];
        Parser_remove_output_node(p, id, entry.node);
      }
      break;
    default:
      printf("Trying to remove bad node type: `%s`\n", NODE_NAME[node.tag]);
      assert(0);
  }
  p->node_arr[id].tag = NODE_NONE;
}

void Parser_push_scope(Parser *p) {
  NodeId id = Parser_create_node(p, (Node){
    .tag = NODE_SCOPE,
    .value.scope.var_start = p->var_len,
    .value.scope.var_count = 0,
    .value.scope.prev_scope = p->scope,
  });
  p->scope = id;
}

void Parser_pop_scope(Parser *p) {
  assert(p->scope);
  Node scope = p->node_arr[p->scope];
  assert(p->var_len == scope.value.scope.var_start + scope.value.scope.var_count);
  NodeId prev = p->scope;
  Parser_remove_node(p, prev);
  p->var_len -= p->node_arr[p->scope].value.scope.var_count;
  p->scope = p->node_arr[p->scope].value.scope.prev_scope;
}

VarId Parser_push_var(Parser *p, uint32_t start, uint16_t len, NodeId node) {
  assert(p->var_len < MAX_VAR_DEPTH);
  uint16_t var_count = p->node_arr[p->scope].value.scope.var_count;
  uint16_t var_start = p->node_arr[p->scope].value.scope.var_start;
  for (int i = 0; i < var_count; i++) {
    Var entry = p->var_arr[var_start + i];
    if (entry.len != len) continue;
    if (strncmp(&p->source[entry.start], &p->source[start], len)) continue;
    // TODO: show previous variable location and current
    report_error(p->source, start, len, "Redefintion of `%.*s`", len, &p->source[start]);
  }
  VarId id = p->var_len++;
  p->var_arr[id] = (Var){ start, len, node };
  p->node_arr[p->scope].value.scope.var_count++;
  Parser_add_node_output(p, p->scope, node);
  return id;
}

VarId Parser_resolve_var(Parser *p, uint32_t start, uint16_t len) {
  NodeId scope = p->scope;
  while (scope) {
    uint16_t var_count = p->node_arr[scope].value.scope.var_count;
    uint16_t var_start = p->node_arr[scope].value.scope.var_start;
    uint16_t var_end = var_start + var_count;
    for (int i = var_end - 1; i >= var_start; --i) {
      Var var = p->var_arr[i];
      if (var.len != len) continue;
      if (!strncmp(&p->source[var.start], &p->source[start], len)) return i;
    }
    scope = p->node_arr[scope].value.scope.prev_scope;
  }
  report_error(p->source, start, len, "Variable `%.*s` not found", len, &p->source[start]);
}

void Parser_update_var(Parser *p, uint32_t start, uint16_t len, NodeId new_value) {
  NodeId scope = p->scope;
  VarId id = 0;
  while (scope) {
    uint16_t var_count = p->node_arr[scope].value.scope.var_count;
    uint16_t var_start = p->node_arr[scope].value.scope.var_start;
    uint16_t var_end = var_start + var_count;
    for (int i = var_end - 1; i >= var_start; --i) {
      Var var = p->var_arr[i];
      if (var.len != len) continue;
      if (strncmp(&p->source[var.start], &p->source[start], len)) continue;
      id = i;
      goto var_found;
    }
    scope = p->node_arr[scope].value.scope.prev_scope;
  }
  report_error(p->source, start, len, "Variable `%.*s` not found", len, &p->source[start]);
var_found:
  Parser_remove_output_node(p, scope, p->var_arr[id].node);
  Parser_add_node_output(p, scope, new_value);
  p->var_arr[id].node = new_value;
}

Token Parser_expect_token(Parser *p, TokenTag tag) {
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

void Parser_add_node_output(Parser *p, NodeId user, NodeId used) {
  assert(p->link_len < MAX_LINKS);
  LinkId id = p->link_len++;
  LinkId prev = p->node_arr[used].outputs;
  p->link_arr[id] = (Link){ prev, user };
  p->node_arr[used].outputs = id;
}

NodeId Parser_parse_atom(Parser *p) {
  Token tok = p->token_arr[p->pos++];
  NodeId node;
  switch (tok.tag) {
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
      node = Parser_create_node(p, (Node){
        .tag = NODE_CONSTANT,
        .value.i64 = number,
        .type = TYPE_INT,
      });
      // Parser_add_node_output(p, START_NODE, node);
      break;
    default: report_error(p->source, tok.start, tok.len,
      "Unexpected token while parsing atom: `%s`", TOK_NAMES[tok.tag]);
  }
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
  Parser_add_node_output(p, node, inner);
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
    NodeId right = Parser_parse_binary(p, new_prec);
    if (p->node_arr[left].tag != NODE_CONSTANT
        || p->node_arr[right].tag != NODE_CONSTANT) {
      NodeId node = Parser_create_node(p, (Node){
        .tag = op,
        .value.binary.left = left,
        .value.binary.right = right,
      });
      Parser_add_node_output(p, node, left);
      Parser_add_node_output(p, node, right);
      continue;
    }
    uint64_t l = p->node_arr[left].value.i64;
    uint64_t r = p->node_arr[right].value.i64;
    Parser_remove_node(p, right);
    Parser_remove_node(p, left);
    switch (op) {
      case NODE_ADD: l += r; break;
      case NODE_SUB: l -= r; break;
      case NODE_MUL: l *= r; break;
      case NODE_DIV: l /= r; break;
      default: assert(0);
    }
    left = Parser_create_node(p, (Node){
      .tag = NODE_CONSTANT,
      .value.i64 = l,
    });
  }
  return left;
}

NodeId Parser_parse_expression(Parser *p) {
  NodeId value = Parser_parse_binary(p, 0);
  return value;
}

NodeId Parser_parse_statement(Parser *p);

NodeId Parser_parse_block(Parser *p) {
  Parser_push_scope(p);
  assert(p->pos);
  Token start = p->token_arr[p->pos - 1];
  NodeId node = 0;
  while (p->token_arr[p->pos].tag != TOK_RBRACE) {
    Token tok = p->token_arr[p->pos];
    if (!tok.tag) report_error(p->source, start.start, start.len,
        "No matching `}` found before EOF");
    NodeId elem = Parser_parse_statement(p);
    if (elem) node = elem;
  }
  p->pos++;
  Parser_pop_scope(p);
  return node;
}

NodeId Parser_parse_statement(Parser *p) {
  NodeId node = 0, value = 0;
  Token tok = p->token_arr[p->pos++];
  switch (tok.tag) {
    case TOK_IDENT:
      Parser_expect_token(p, TOK_EQ);
      value = Parser_parse_expression(p);
      Parser_update_var(p, tok.start, tok.len, value);
      break;
    case TOK_INT:
      tok = Parser_expect_token(p, TOK_IDENT);
      Parser_expect_token(p, TOK_EQ);
      value = Parser_parse_expression(p);
      Parser_push_var(p, tok.start, tok.len, value);
      break;
    case TOK_SEMICOLON:
      break;
    case TOK_LBRACE:
      node = Parser_parse_block(p);
      break;
    case TOK_RETURN:
      value = Parser_parse_expression(p);
      Parser_expect_token(p, TOK_SEMICOLON);
      node = Parser_create_node(p, (Node){
        .tag = NODE_RETURN,
        .value.ret.predecessor = START_NODE,
        .value.ret.value = value,
      });
      Parser_add_node_output(p, node, START_NODE);
      Parser_add_node_output(p, node, value);
      break;
    default: report_error(p->source, tok.start, tok.len,
          "Expected statement, got `%s`", TOK_NAMES[tok.tag]);
  }
  return node;
}

NodeId Parser_parse_top_level(Parser *p) {
  Parser_push_scope(p);
  NodeId node = 0;
  while (p->token_arr[p->pos].tag != TOK_NONE) {
    NodeId elem = Parser_parse_statement(p);
    if (elem) node = elem;
  }
  Parser_pop_scope(p);
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

  Parser_parse_top_level(p);
}

void print_nodes(const Parser *p) {
  for (int i = 0; i < p->node_len; ++i) {
    Node node = p->node_arr[i];
    if (!node.tag) continue;
    printf("% 2d %s", i, NODE_NAME[node.tag]);
    switch (node.tag) {
      case NODE_CONSTANT:
        printf(" %d", node.value.i64);
        // printf("%d", START_NODE);
        break;
      case NODE_RETURN:
        printf(" %d, ", node.value.ret.predecessor);
        printf("%d", node.value.ret.value);
        break;
      case NODE_START:
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
  }
}
