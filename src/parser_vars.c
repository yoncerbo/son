#include "parser.h"

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
      NodeId phi = Parser_create_phi_node(p, ctrl, lnode, rnode);

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
