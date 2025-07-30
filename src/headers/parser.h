#ifndef INCLUDE_PARSER
#define INCLUDE_PARSER

#include "tokenizer.h"
#include <stdint.h>

typedef uint16_t NodeId;
typedef uint16_t LinkId;

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
  // Control nodes
  NODE_START,
  NODE_RETURN, // in: cnode predecessor, dnode value

  // Data nodes
  NODE_CONSTANT, // in: start node
  NODE_ADD,
  NODE_SUB,
  NODE_MUL,
  NODE_DIV,
  NODE_MINUS,
} NodeTag;

const char *NODE_NAME[] = {
  [NODE_START] = "start",
  [NODE_RETURN] = "return",

  [NODE_CONSTANT] = "const",
};

typedef union {
  int64_t i64;
  // Is it necessary? Can't we just use inputs?
  struct NodeReturn {
    NodeId predecessor;
    NodeId value;
  } ret;
} NodeValue;

typedef struct {
  NodeTag tag;
  NodeValue value;
  LinkId outputs;
} Node;

typedef struct {
#define START_NODE ((NodeId)0)
#define MAX_NODES 256
  Node node_arr[MAX_NODES];
#define NULL_LINK ((LinkId)0)
#define MAX_LINKS 512
  Link link_arr[MAX_LINKS];

  const char *source;
  const Token *token_arr;
  uint16_t node_len;
  uint16_t link_len;
  uint16_t pos;
} Parser;

NodeId parse(const char *source, const Token *tokens, Parser *p);
void print_nodes(const Parser *p);

#endif // !INCLUDE_NODES
