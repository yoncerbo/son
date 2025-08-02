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
  bprintf(wb, "digraph G {\n");

  for (int i = 0; i < p->node_len; ++i) {
    Node node = p->node_arr[i];
    if (!node.tag) continue;
    switch (node.tag) {
      case NODE_SCOPE:
        bprintf(wb, "  %d [shape=none,label=<\n", i, node.value.i64);
        bprintf(wb, "<TABLE BORDER=\"0\" CELLSPACING=\"0\" CELLBORDER=\"1\">\n");
        bprintf(wb, "  <TR><TD>scope</TD></TR>\n");
        for (int j = 0; j < node.value.scope.var_count; ++j) {
          VarId id = node.value.scope.var_start + j;
          Var var = {0};
          if (p->flags & BRANCHL) var = p->lvar_arr[id];
          else var = p->rvar_arr[id];
          printf("  %.*s: %d\n", var.len, &p->source[var.start], var.node);
          bprintf(wb, "  <TR><TD PORT=\"%d\">%.*s</TD></TR>\n", j, var.len, &p->source[var.start]);
        }
        bprintf(wb, "</TABLE>>];\n");
        for (int j = 0; j < node.value.scope.var_count; ++j) {
          VarId id = node.value.scope.var_start + j;
          Var var = {0};
          if (p->flags & BRANCHL) var = p->lvar_arr[id];
          else var = p->rvar_arr[id];
          bprintf(wb, "  %d -> %d:%d", var.node, i, j);
        }
        break;
      case NODE_CONSTANT:
        bprintf(wb, "  %d [shape=oval,label=\"%d\"];\n", i, node.value.i64);
        break;
      case NODE_PROJ:
        bprintf(wb, "  %d [shape=box,label=\"proj %d\"];\n", i, node.value.proj.select);
        break;
      default:
        bprintf(wb, "  %d [shape=box,label=\"%s\"];\n", i, NODE_NAME[node.tag]);

    }
    bprintf(wb, "  %d -> {", i);
    LinkId id = node.outputs;
    while (id) {
      NodeId nid = p->link_arr[id].node;
      if (p->node_arr[nid].tag != NODE_SCOPE) bprintf(wb, "%d", p->link_arr[id].node);
      if (!p->link_arr[id].next) break;
      bprintf(wb, " ");
      id = p->link_arr[id].next;
    }
    bprintf(wb, "};\n");
  }
  bprintf(wb, "}");
  bwrite(wb, filename);
}
