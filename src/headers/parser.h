#ifndef INCLUDE_PARSER
#define INCLUDE_PARSER

#include "tokenizer.h"
#include <stdint.h>
#include <stdbool.h>

typedef uint16_t NodeId;
typedef uint16_t LinkId;
typedef uint16_t VarId;

const char *GRAPH_FILENAME = "out/graph.dot";

typedef enum {
  TYPE_BOT, // ALL, empty type, default
  TYPE_TOP, // ANY
  TYPE_INT,
  TYPE_INT_BOT,
  TYPE_INT_TOP,
  TYPE_TUPLE,
} TypeTag;

// NOTE: for now a linked list, as
// I don't really know what the requirements
// are gonna be.
// If we won't need to edit it that much
// flat array will do fine
// And we're wasting quite a bit of space with
// this approach. If'm gonna decide
// to keep it a list, then I'm gonna use blocks
typedef struct {
  LinkId next;
  NodeId node;
} Link;

typedef enum {
  NODE_NONE,

  NODE_SCOPE,

  // Control nodes
  NODE_START,
  NODE_STOP,
  NODE_RETURN, // in: cnode predecessor, dnode value
  NODE_REGION,
  NODE_IF,
  NODE_PROJ,

#define DATA_NODES_START NODE_PHI
  // Data nodes
  NODE_PHI,
  NODE_CONSTANT, // in: start node
#define NODE_BINARY_START NODE_ADD
  NODE_ADD,
  NODE_SUB,
  NODE_MUL,
  NODE_DIV,
  NODE_MINUS,
  NODE_NOT,
  NODE_EQ,
  NODE_NE,
  NODE_LT,
  NODE_LE,
  NODE_GT,
  NODE_GE,
#define NODE_BINARY_COUNT (NODE_COUNT - NODE_ADD)
  NODE_COUNT
} NodeTag;

const char *NODE_NAME[] = {
  [NODE_NONE] = "none",
  [NODE_SCOPE] = "scope",
  [NODE_START] = "start",
  [NODE_RETURN] = "return",
  [NODE_CONSTANT] = "const",
  [NODE_ADD] = "add",
  [NODE_SUB] = "sub",
  [NODE_MUL] = "mul",
  [NODE_DIV] = "div",
  [NODE_MINUS] = "minus",
  [NODE_NOT] = "not",
  [NODE_REGION] = "region",
  [NODE_IF] = "if",
  [NODE_PHI] = "phi",
  [NODE_PROJ] = "proj",
  [NODE_EQ] = "eq",
  [NODE_NE] = "ne",
  [NODE_GT] = "gt",
  [NODE_GE] = "ge",
  [NODE_LT] = "lt",
  [NODE_LE] = "le",
  [NODE_NOT] = "not",
  [NODE_STOP] = "stop",
};

typedef struct {
  uint32_t start;
  uint16_t len;
  NodeId node;
} Var;

typedef union {
  int64_t i64;
  bool boolean;
  // Is it necessary? Can't we just use inputs?
  struct NodeReturn {
    NodeId predecessor;
    NodeId value;
  } ret;
  struct NodeUnary {
    NodeId node;
  } unary;
  struct NodeBinary {
    NodeId left;
    NodeId right;
  } binary;
  struct NodeScope {
    VarId var_start;
    uint16_t var_count;
    NodeId prev_scope;
    NodeId ctrl;
  } scope;
  struct NodeIf {
    NodeId ctrl;
    NodeId cond;
  } if_;
  struct NodeBlock {
    NodeId next_block;
  } block;
  struct NodeProj {
    uint16_t select;
    NodeId ctrl;
  } proj;
  struct NodeRegion {
    NodeId left_block;
    NodeId right_block;
  } region;
  struct NodePhi {
    NodeId ctrl;
    NodeId left;
    NodeId right;
  } phi;
} NodeValue;

typedef struct {
  TypeTag tag;
  uint16_t elem_start;
} Type;

typedef struct {
  NodeTag tag;
  NodeValue value;
  LinkId outputs;
  NodeId next_sibling; // used in blocks
  TypeTag type;
} Node;

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

NodeId parse(const char *source, const Token *tokens, Parser *p);
void print_nodes(const Parser *p);

#endif // !INCLUDE_NODES
