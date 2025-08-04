#ifndef INCLUDE_NODES
#define INCLUDE_NODES

#include "common.h"
#include <stdbool.h>

typedef uint16_t NodeId;
typedef uint16_t LinkId;
typedef uint16_t VarId;

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
    VarId var_count;
    NodeId prev_scope;
    NodeId ctrl;
  } scope;
  struct NodeIf {
    NodeId ctrl;
    NodeId cond;
  } if_;
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
  NodeTag tag;
  NodeValue value;
  LinkId outputs;
  TypeTag type;
} Node;

#endif
