#include "parser.h"
#include "common.h"
#include "tokenizer.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

const Str GRAPH_BUILTIN_NAME = STR("graph");
const Str PRINT_AST_BUILTIN_NAME = STR("nodes");

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

NodeId Parser_create_node(Parser *p, Node node);
void Parser_add_node_output(Parser *p, NodeId user, NodeId used);
void Parser_remove_node(Parser *p, NodeId id);
NodeId Parser_parse_expression(Parser *p);

void print_nodes(const Parser *p);

void Parser_remove_output_node(Parser *p, NodeId user, NodeId used) {
  if (!user || !used) return;
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
  printf("bad %d\n", used);
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
    case NODE_PHI:
      Parser_remove_output_node(p, node.value.phi.ctrl, id);
      Parser_remove_output_node(p, node.value.phi.left, id);
      Parser_remove_output_node(p, node.value.phi.right, id);
      break;
    case NODE_SCOPE:
      // NOTE: you can only remove top most scope
      uint16_t var_start = node.value.scope.var_start;
      uint16_t var_count = node.value.scope.var_count;
      for (int i = 0; i < var_count; i++) {
        Var entry = p->var_arr[var_start + i];
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
    .value.scope.ctrl = 0,
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

void Parser_update_ctrl(Parser *p, NodeId new_ctrl) {
  NodeId scope = p->scope;
  while (scope) {
    NodeId ctrl = p->node_arr[scope].value.scope.ctrl;
    if (ctrl) {
      p->node_arr[scope].value.scope.ctrl = new_ctrl;
      return;
    }
    scope = p->node_arr[scope].value.scope.prev_scope;
  }
}

NodeId Parser_resolve_ctrl(Parser *p) {
  NodeId scope = p->scope;
  while (scope) {
    NodeId ctrl = p->node_arr[scope].value.scope.ctrl;
    if (ctrl) return ctrl;
    scope = p->node_arr[scope].value.scope.prev_scope;
  }
  return 0;
}

VarId Parser_push_var(Parser *p, uint32_t start, uint16_t len, NodeId node) {
  assert(p->var_len < MAX_VAR_DEPTH);
  uint16_t var_count = p->node_arr[p->scope].value.scope.var_count;
  uint16_t var_start = p->node_arr[p->scope].value.scope.var_start;
  for (int i = 0; i < var_count; i++) {
    Var entry = p->var_arr[var_start + i];
    if (entry.len != len) continue;
    if (strncmp(&p->source[entry.start], &p->source[start], len)) continue;
    print_error_message("Redefintion of `%.*s`", len, &p->source[start]);
    print_note(p->source, entry.start, entry.len, "Variable first defined here");
    print_note(p->source, start, len, "Redefinition here");
    exit(1);
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
  print_error(p->source, start, len, "Variable `%.*s` not found", len, &p->source[start]);
  exit(1);
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
  print_error(p->source, start, len, "Variable `%.*s` not found", len, &p->source[start]);
  exit(1);
var_found:
  Parser_remove_output_node(p, scope, p->var_arr[id].node);
  Parser_add_node_output(p, scope, new_value);
  p->var_arr[id].node = new_value;
}

NodeId Parser_duplicate_scopes(Parser *p, uint16_t offset) {
  NodeId scope = p->scope;
  NodeId ret = 0;
  while (scope) {
    Node node = p->node_arr[scope];
    uint16_t new_start = node.value.scope.var_start + offset;
    uint16_t var_count = node.value.scope.var_count;
    node.value.scope.var_start += offset;
    NodeId new_scope = Parser_create_node(p, node);
    if (!ret) ret = new_scope;

    uint16_t var_end = new_start + var_count; 
    for (int i = var_end - 1; i >= new_start; --i) {
      Var var = p->var_arr[i];
      Parser_add_node_output(p, new_scope, var.node);
    }
    scope = p->node_arr[scope].value.scope.prev_scope;
  }
  return ret;
}

void Parser_create_phi_nodes(Parser *p, uint16_t copy_start, NodeId ctrl, NodeId new_scope) {
  NodeId scope = p->scope;
  while (scope) {
    assert(new_scope);
    uint16_t var_count = p->node_arr[scope].value.scope.var_count;
    uint16_t var_start = p->node_arr[scope].value.scope.var_start;
    uint16_t var_end = var_start + var_count;
    for (int i = var_end - 1; i >= var_start; --i) {
      NodeId lnode = p->var_arr[i].node;
      NodeId rnode = p->var_arr[i + copy_start].node;
      if (lnode == rnode) continue;
      NodeId phi = Parser_create_node(p, (Node){
        .tag = NODE_PHI,
        .value.phi.ctrl = ctrl,
        .value.phi.left = lnode,
        .value.phi.right = rnode,
      });
      Parser_add_node_output(p, phi, ctrl);
      Parser_add_node_output(p, phi, lnode);
      Parser_add_node_output(p, phi, rnode);

      printf("left: %d\n", p->node_arr[lnode].value.i64);
      printf("right: %d\n", p->node_arr[rnode].value.i64);
      LinkId link = p->node_arr[lnode].outputs;

      // TOOD: why it's not an input of scope?
      Parser_remove_output_node(p, scope, lnode);
      Parser_remove_output_node(p, new_scope, rnode);
      Parser_add_node_output(p, scope, phi);
      p->var_arr[i].node = phi;
    }
    scope = p->node_arr[scope].value.scope.prev_scope;
    // Parser_remove_node(p, new_scope);
    new_scope = p->node_arr[new_scope].value.scope.prev_scope;
  }
}

Token Parser_expect_token(Parser *p, TokenTag tag) {
  Token tok = p->token_arr[p->pos++];
  if (tok.tag == tag) return tok;
  print_error(p->source, tok.start, tok.len,
    "Expected `%s`, got `%s`", TOK_NAMES[tag], TOK_NAMES[tok.tag]);
  exit(1);
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
    case TOK_TRUE:
      node = Parser_create_node(p, (Node){
        .tag = NODE_CONSTANT,
        .type = TYPE_INT,
        .value.boolean = true,
      });
      break;
    case TOK_FALSE:
      node = Parser_create_node(p, (Node){
        .tag = NODE_CONSTANT,
        .type = TYPE_INT,
        .value.boolean = false,
      });
      break;
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
    NodeId node = Parser_create_node(p, (Node){
      .tag = op,
      .value.unary.node = inner,
      .type = TYPE_INT,
    });
    Parser_add_node_output(p, node, inner);
    return node;
  }
  int64_t value = p->node_arr[inner].value.i64;
  Parser_remove_node(p, inner);
  switch (op) {
    case NODE_MINUS: value = -value; break;
    case NODE_NOT: value = !value; break;
    default: assert(0);
  }
  NodeId node = Parser_create_node(p, (Node){
    .tag = NODE_CONSTANT,
    .value.i64 = value,
    .type = TYPE_INT,
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
      NodeId node = Parser_create_node(p, (Node){
        .tag = op,
        .value.binary.left = left,
        .value.binary.right = right,
        .type = TYPE_INT,
      });
      Parser_add_node_output(p, node, left);
      Parser_add_node_output(p, node, right);
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
    left = Parser_create_node(p, (Node){
      .tag = NODE_CONSTANT,
      .value.i64 = l,
      .type = TYPE_INT,
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
    if (!tok.tag) {
      print_error(p->source, start.start, start.len, "No matching `}` found before EOF");
      exit(1);
    }
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
    case TOK_IF:
      // duplicate the scope
      Parser_expect_token(p, TOK_LPAREN);
      NodeId cond = Parser_parse_expression(p);
      Parser_expect_token(p, TOK_RPAREN);
      NodeId ctrl = Parser_resolve_ctrl(p);
      node = Parser_create_node(p, (Node){
        .tag = NODE_IF,
        .value.if_.ctrl = ctrl,
        .value.if_.cond = cond,
      });
      NodeId then_block = Parser_create_node(p, (Node){
        .tag = NODE_PROJ,
        .value.proj.select = 0,
        .value.proj.ctrl = node,
      });
      NodeId else_block = Parser_create_node(p, (Node){
        .tag = NODE_PROJ,
        .value.proj.select = 1,
        .value.proj.ctrl = node,
      });
      NodeId region = Parser_create_node(p, (Node){
        .tag = NODE_REGION,
      });
      Parser_add_node_output(p, node, ctrl);
      Parser_add_node_output(p, node, cond);
      // TODO: resolve the ctrl nodes instead of adding them manually
      Parser_add_node_output(p, then_block, node);
      Parser_add_node_output(p, else_block, node);
      Parser_update_ctrl(p, node);

      // TODO: We don't need a copy of start and len, only node
      uint16_t right_start = p->var_len;
      uint16_t left_start = p->var_offset;
      uint16_t var_len = right_start - left_start;
      assert((p->var_len += var_len) < MAX_VAR_DEPTH);
      memcpy(&p->var_arr[right_start], &p->var_arr[left_start], var_len * sizeof(Var));
      NodeId new_scope = Parser_duplicate_scopes(p, right_start);
      NodeId prev_scope = p->scope;

      // Then block
      p->scope = new_scope;
      p->var_offset = right_start;
      Parser_push_scope(p);
      Parser_update_ctrl(p, then_block);
      Parser_parse_statement(p);
      then_block = Parser_resolve_ctrl(p);
      p->node_arr[region].value.region.left_block = then_block;
      Parser_add_node_output(p, region, then_block);
      Parser_pop_scope(p);
      p->scope = prev_scope;
      p->var_offset = left_start;

      // Else block
      if (p->token_arr[p->pos].tag == TOK_ELSE) {
        p->pos++;
        Parser_push_scope(p);
        Parser_update_ctrl(p, else_block);
        Parser_parse_statement(p);
        else_block = Parser_resolve_ctrl(p);
        Parser_pop_scope(p);
      }
      p->node_arr[region].value.region.right_block = else_block;
      Parser_add_node_output(p, region, else_block);

      p->node_arr[p->scope].value.scope.ctrl = region;
      Parser_create_phi_nodes(p, right_start, region, new_scope);
      p->var_len = right_start;
      Parser_update_ctrl(p, region);
      break;
    case TOK_IDENT:
      // TODO: add debug flag
      if (tok.len == GRAPH_BUILTIN_NAME.len &&
          !strncmp(GRAPH_BUILTIN_NAME.ptr, &p->source[tok.start], tok.len)) {
        output_graphviz_file(GRAPH_FILENAME, p);
        Parser_expect_token(p, TOK_LPAREN);
        Parser_expect_token(p, TOK_RPAREN);
        Parser_expect_token(p, TOK_SEMICOLON);
        break;
      }
      if (tok.len == PRINT_AST_BUILTIN_NAME.len &&
          !strncmp(PRINT_AST_BUILTIN_NAME.ptr, &p->source[tok.start], tok.len)) {
        print_nodes(p);
        Parser_expect_token(p, TOK_LPAREN);
        Parser_expect_token(p, TOK_RPAREN);
        Parser_expect_token(p, TOK_SEMICOLON);
        break;
      }
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
      ctrl = Parser_resolve_ctrl(p);
      node = Parser_create_node(p, (Node){
        .tag = NODE_RETURN,
        .value.ret.predecessor = ctrl,
        .value.ret.value = value,
      });
      Parser_add_node_output(p, node, ctrl);
      Parser_add_node_output(p, node, value);
      Parser_add_node_output(p, STOP_NODE, node);
      Parser_update_ctrl(p, 0);
      break;
    default: 
      print_error(p->source, tok.start, tok.len,
        "Expected statement, got `%s`", TOK_NAMES[tok.tag]);
      exit(1);
  }
  return node;
}

NodeId Parser_parse_top_level(Parser *p) {
  Parser_push_scope(p);
  p->node_arr[p->scope].value.scope.ctrl = START_NODE;
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
