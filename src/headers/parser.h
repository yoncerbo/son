#ifndef INCLUDE_PARSER
#define INCLUDE_PARSER

#include "tokenizer.h"
#include "nodes.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
  uint32_t start;
  uint16_t len;
  NodeId node;
} Var;

typedef struct {
  TypeTag tag;
  uint16_t elem_start;
} Type;

typedef struct {
#define NULL_NODE ((NodeId)0)
#define START_NODE ((NodeId)1)
#define STOP_NODE ((NodeId)2)
#define MAX_NODES 256
  Node node_arr[MAX_NODES];
#define NULL_LINK ((LinkId)0)
#define MAX_LINKS 512
  Link link_arr[MAX_LINKS];
#define MAX_VAR_DEPTH 256
  Var var_arr[MAX_VAR_DEPTH];
#define MAX_TYPES 256
  Type type_arr[MAX_TYPES];
  char filename_buffer[256];

  const char *source;
  const Token *token_arr;
  uint16_t node_len;
  uint16_t link_len;
  uint16_t var_len;
  uint16_t var_offset;
  uint16_t scope_len;
  uint16_t type_len;
  uint16_t pos;
  NodeId scope;
} Parser;

// parser.c
NodeId parse(const char *source, const Token *tokens, Parser *p);
void print_nodes(const Parser *p);
Token Parser_expect_token(Parser *p, TokenTag tag);

// parser_nodes.c
void Parser_add_node_output(Parser *p, NodeId user, NodeId used);
void Parser_remove_node(Parser *p, NodeId id);
void Parser_remove_output_node(Parser *p, NodeId user, NodeId used);
NodeId Parser_create_node(Parser *p, Node node);
NodeId Parser_create_constant(Parser *p, int64_t value);
NodeId Parser_create_unary_node(Parser *p, NodeTag op, NodeId inner);
NodeId Parser_create_binary_node(Parser *p, NodeTag op, NodeId left, NodeId right);
NodeId Parser_create_proj_node(Parser *p, NodeId ctrl, uint16_t select);
NodeId Parser_create_if_node(Parser *p, NodeId cond);
NodeId Parser_create_region_node(Parser *p, NodeId left, NodeId right);
NodeId Parser_create_return_node(Parser *p, NodeId value);
NodeId Parser_create_phi_node(Parser *p, NodeId region, NodeId left, NodeId right);

// parser_vars.c
VarId Parser_resolve_var(Parser *p, uint32_t start, uint16_t len);
VarId Parser_push_var(Parser *p, uint32_t start, uint16_t len, NodeId node);
NodeId Parser_duplicate_scopes(Parser *p, uint16_t offset);
void Parser_create_phi_nodes(Parser *p, uint16_t copy_start, NodeId ctrl, NodeId new_scope);
void Parser_update_var(Parser *p, uint32_t start, uint16_t len, NodeId new_value);
void Parser_pop_scope(Parser *p);
void Parser_push_scope(Parser *p);
NodeId Parser_resolve_ctrl(Parser *p);
void Parser_update_ctrl(Parser *p, NodeId new_ctrl);

// parser_expressions.c
NodeId Parser_parse_expression(Parser *p);

// parser_statements.c
NodeId Parser_parse_statement(Parser *p);
NodeId Parser_parse_top_level(Parser *p);

#endif // !INCLUDE_NODES
