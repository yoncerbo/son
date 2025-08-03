#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include "parser.h"

const char *GRAPHVIZ_PREAMBLE =
  "digraph G {\n"
  "  bgcolor=\"#181818\";\n"
  "  node [fontoclor=\"#e6e6e6\", style=filled, color=\"#e6e6e6\", fillcolor=\"#333333\"];\n"
  "  edge [color=\"#e6e6e6\", fontcolor=\"#e6e6e6\"]\n";


#define MAX_FILE_SIZE 4096

typedef struct {
  char buffer[MAX_FILE_SIZE];
  uint32_t pos;
} WritingBuffer;

static inline void bprintf(WritingBuffer *wb, const char *fmt, ...) {
  uint32_t max_len = MAX_FILE_SIZE - wb->pos;
  va_list args;
  va_start(args, fmt);
  uint32_t written = vsnprintf(&wb->buffer[wb->pos], max_len, fmt, args);
  // TODO: maybe flush the buffer to the file or use multiple buffers
  assert(written <= max_len);
  va_end(args);
  wb->pos += written;
}

void bwrite(WritingBuffer *wb, const char *filename) {
  int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0777);
  if (fd < 0) {
    perror("Failed to open the file");
    exit(1);
  }
  if (write(fd, wb->buffer, wb->pos) < 0) {
    perror("Failed to write to the file");
    exit(1);
  }
}

void output_graphviz_file(const char *filename, const Parser *p) {
  WritingBuffer *wb = malloc(sizeof *wb);
  bprintf(wb, GRAPHVIZ_PREAMBLE);

  for (int i = 0; i < p->node_len; ++i) {
    Node node = p->node_arr[i];
    if (!node.tag) continue;
    // if (!node.outputs) continue;
    switch (node.tag) {
      case NODE_SCOPE:
        bprintf(wb, "  %d [shape=none,label=<\n", i, node.value.i64);
        bprintf(wb, "<TABLE BORDER=\"0\" CELLSPACING=\"0\" CELLBORDER=\"1\">\n");
        bprintf(wb, "  <TR><TD>scope</TD></TR>\n");
        // TODO: print scope's ctrl
        for (int j = 0; j < node.value.scope.var_count; ++j) {
          VarId id = p->var_offset + node.value.scope.var_start + j;
          Var var = p->var_arr[id];
          printf("  %.*s: %d\n", var.len, &p->source[var.start], var.node);
          bprintf(wb, "  <TR><TD PORT=\"%d\">%.*s</TD></TR>\n", j, var.len, &p->source[var.start]);
        }
        bprintf(wb, "</TABLE>>];\n");
        for (int j = 0; j < node.value.scope.var_count; ++j) {
          VarId id = p->var_offset + node.value.scope.var_start + j;
          Var var = p->var_arr[id];
          bprintf(wb, "  %d -> %d:%d", var.node, i, j);
        }
        break;
      case NODE_IF:
        bprintf(wb, "  %d [shape=none,label=<\n", i, node.value.i64);
        bprintf(wb, "<TABLE BORDER=\"0\" CELLSPACING=\"0\" CELLBORDER=\"1\">\n");
        bprintf(wb, "  <TR><TD COLSPAN=\"3\">if</TD></TR>\n");
        bprintf(wb, "  <TR><TD PORT=\"0\">then</TD><TD PORT=\"1\">else</TD></TR>\n");
        bprintf(wb, "</TABLE>>];\n");
        break;
      case NODE_CONSTANT:
        bprintf(wb, "  %d [shape=oval,label=\"#%d\"];\n", i, node.value.i64);
        break;
      case NODE_PROJ:
        break;
      default:
        const char *shape = node.tag >= DATA_NODES_START ? "oval" : "box";
        bprintf(wb, "  %d [shape=%s,label=\"%s\"];\n", i, shape, NODE_NAME[node.tag]);

    }
    if (node.tag != NODE_PROJ) bprintf(wb, "  %d -> {", i);
    else bprintf(wb, "  %d:%d -> {", node.value.proj.ctrl, node.value.proj.select);
    LinkId id = node.outputs;
    while (id) {
      NodeId nid = p->link_arr[id].node;
      if (p->node_arr[nid].tag != NODE_SCOPE
          && p->node_arr[nid].tag != NODE_PROJ) bprintf(wb, "%d", p->link_arr[id].node);
      if (!p->link_arr[id].next) break;
      bprintf(wb, " ");
      id = p->link_arr[id].next;
    }
    bprintf(wb, "};\n");
  }
  bprintf(wb, "}");
  bwrite(wb, filename);
}
