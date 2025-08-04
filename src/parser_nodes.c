#include "parser.h"
#include <stdint.h>
#include <assert.h>

void Parser_add_node_output(Parser *p, NodeId user, NodeId used) {
  assert(p->link_len < MAX_LINKS);
  LinkId id = p->link_len++;
  LinkId prev = p->node_arr[used].outputs;
  p->link_arr[id] = (Link){ prev, user };
  p->node_arr[used].outputs = id;
}

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


NodeId Parser_create_node(Parser *p, Node node) {
  assert(p->node_len < MAX_NODES);
  NodeId id = p->node_len++;
  p->node_arr[id] = node;
  return id;
}

NodeId Parser_create_constant(Parser *p, int64_t value) {
  NodeId node = Parser_create_node(p, (Node){
    .tag = NODE_CONSTANT,
    .type = TYPE_INT,
    .value.i64 = value,
  });
  // Parser_add_node_output(p, START_NODE, node);
  return node;
}

NodeId Parser_create_unary_node(Parser *p, NodeTag op, NodeId inner) {
  NodeId node = Parser_create_node(p, (Node){
    .tag = op,
    .value.unary.node = inner,
    .type = TYPE_INT,
  });
  Parser_add_node_output(p, node, inner);
  return node;
}

NodeId Parser_create_binary_node(Parser *p, NodeTag op, NodeId left, NodeId right) {
  NodeId node = Parser_create_node(p, (Node){
    .tag = op,
    .value.binary.left = left,
    .value.binary.right = right,
    .type = TYPE_INT,
  });
  Parser_add_node_output(p, node, left);
  Parser_add_node_output(p, node, right);
  return node;
}

NodeId Parser_create_proj_node(Parser *p, NodeId ctrl, uint16_t select) {
  NodeId node = Parser_create_node(p, (Node){
    .tag = NODE_PROJ,
    .value.proj.ctrl = ctrl,
    .value.proj.select = select,
  });
  Parser_add_node_output(p, node, ctrl);
  return node;
}

// TODO: wouldn't it be better to have node creation funcitons
// based on the argument count
NodeId Parser_create_if_node(Parser *p, NodeId cond) {
  NodeId ctrl = Parser_resolve_ctrl(p);
  NodeId node = Parser_create_node(p, (Node){
    .tag = NODE_IF,
    .value.if_.ctrl = ctrl,
    .value.if_.cond = cond,
  });
  Parser_add_node_output(p, node, ctrl);
  Parser_add_node_output(p, node, cond);
  Parser_update_ctrl(p, node);
  return node;
}

NodeId Parser_create_region_node(Parser *p, NodeId left, NodeId right) {
  NodeId node =  Parser_create_node(p, (Node){
    .tag = NODE_REGION,
    .value.region.left_block = left,
    .value.region.right_block = right,
  });
  Parser_add_node_output(p, node, left);
  Parser_add_node_output(p, node, right);
  return node;
}

NodeId Parser_create_return_node(Parser *p, NodeId value) {
  NodeId ctrl = Parser_resolve_ctrl(p);
  NodeId node = Parser_create_node(p, (Node){
    .tag = NODE_RETURN,
    .value.ret.predecessor = ctrl,
    .value.ret.value = value,
  });
  Parser_add_node_output(p, node, ctrl);
  Parser_add_node_output(p, node, value);
  Parser_add_node_output(p, STOP_NODE, node);
  Parser_update_ctrl(p, node);
  return node;
}

NodeId Parser_create_phi_node(Parser *p, NodeId region, NodeId left, NodeId right) {
  NodeId node = Parser_create_node(p, (Node){
    .tag = NODE_PHI,
    .value.phi.ctrl = region,
    .value.phi.left = left,
    .value.phi.right = right,
  });
  Parser_add_node_output(p, node, region);
  Parser_add_node_output(p, node, left);
  Parser_add_node_output(p, node, right);
  return node;
}
