#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>
#include "parser.h"

const char *GRAPHVIZ_PREAMBLE =
  "digraph G {\n"
  "  bgcolor=\"#181818\";\n"
  "  node [fontoclor=\"#e6e6e6\", style=filled, color=\"#e6e6e6\", fillcolor=\"#333333\"];\n"
  "  edge [color=\"#e6e6e6\", fontcolor=\"#e6e6e6\"]\n";

void output_graphviz_file(const char *filename, const Parser *p) {
  FILE *fp = fopen(filename, "w");
  if (fp == NULL) {
    print_error_message("Failed to open '%s' for writing", filename);
    return;
  }
  fprintf(fp, "%s", GRAPHVIZ_PREAMBLE);

  for (int i = 0; i < p->node_len; ++i) {
    Node node = p->node_arr[i];
    if (!node.tag) continue;
    // if (!node.outputs) continue;
    switch (node.tag) {
      case NODE_SCOPE:
        fprintf(fp, "  %d [shape=none,label=<\n", i, node.value.i64);
        fprintf(fp, "<TABLE BORDER=\"0\" CELLSPACING=\"0\" CELLBORDER=\"1\">\n");
        fprintf(fp, "  <TR><TD>scope</TD></TR>\n");
        // TODO: print scope's ctrl
        for (int j = 0; j < node.value.scope.var_count; ++j) {
          VarId id = p->var_offset + node.value.scope.var_start + j;
          Var var = p->var_arr[id];
          fprintf(fp, "  <TR><TD PORT=\"%d\">%.*s</TD></TR>\n", j, var.len, &p->source[var.start]);
        }
        fprintf(fp, "</TABLE>>];\n");
        for (int j = 0; j < node.value.scope.var_count; ++j) {
          VarId id = p->var_offset + node.value.scope.var_start + j;
          Var var = p->var_arr[id];
          fprintf(fp, "  %d -> %d:%d", var.node, i, j);
        }
        break;
      case NODE_IF:
        fprintf(fp, "  %d [shape=none,label=<\n"
          "<TABLE BORDER=\"0\" CELLSPACING=\"0\" CELLBORDER=\"1\">\n"
          "  <TR><TD COLSPAN=\"3\">if</TD></TR>\n"
          "  <TR><TD PORT=\"0\">then</TD><TD PORT=\"1\">else</TD></TR>\n"
          "</TABLE>>];\n", i);
        break;
      case NODE_CONSTANT:
        fprintf(fp, "  %d [shape=oval,label=\"#%d\"];\n", i, node.value.i64);
        break;
      case NODE_PROJ:
        break;
      default:
        const char *shape = node.tag >= DATA_NODES_START ? "oval" : "box";
        fprintf(fp, "  %d [shape=%s,label=\"%s\"];\n", i, shape, NODE_NAME[node.tag]);

    }
    if (node.tag != NODE_PROJ) fprintf(fp, "  %d -> {", i);
    else fprintf(fp, "  %d:%d -> {", node.value.proj.ctrl, node.value.proj.select);
    LinkId id = node.outputs;
    while (id) {
      NodeId nid = p->link_arr[id].node;
      if (p->node_arr[nid].tag != NODE_SCOPE
          && p->node_arr[nid].tag != NODE_PROJ) fprintf(fp, "%d", p->link_arr[id].node);
      if (!p->link_arr[id].next) break;
      fprintf(fp, " ");
      id = p->link_arr[id].next;
    }
    fprintf(fp, "};\n");
  }
  fprintf(fp, "}");
}
