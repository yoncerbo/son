#ifndef INCLUDE_NODES
#define INCLUDE_NODES

#include "tokenizer.h"
#include <stdint.h>

typedef uint16_t NodeId;
typedef uint16_t LinkId;

// NOTE: for now a linked list, as
// I don't really know what the requirements
// are gonna be
typedef struct {
  LinkId next;
  NodeId node;
} Link;

typedef enum {
  // Control nodes
  NODE_START,
  NODE_RETURN, // in: cnode predecessor, dnode value

  // Data nodes
  NODE_CONSTANT, // in: start node
} NodeTag;

typedef union {
  int64_t i64;
  NodeId node;
} NodeValue;

typedef struct {
  NodeTag tag;
  NodeValue value;
  LinkId inputs;
  LinkId outputs;
} Node;

typedef struct {
#define MAX_NODES 256
  Node node_arr[MAX_NODES];
#define MAX_LINKS 512
  Link link_arr[MAX_LINKS];

  const char *source;
  const Token *token_arr;
  uint16_t node_len;
  uint16_t link_len;
} Parser;

#endif // !INCLUDE_NODES
