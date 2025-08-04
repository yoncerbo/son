#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error_reporting.c"
#include "graphviz.c"
#include "fs.c"

#include "parser_nodes.c"
#include "parser_expressions.c"
#include "parser_statements.c"
#include "parser_vars.c"
#include "parser.h"
#include "tokenizer.c"
#include "parser.c"

int main(int argc, char *argv[]) {
  assert(argc == 2);
  const char *filename = argv[1];

  printf("Reading file '%s'\n", filename);
  const char *file = map_file_readonly(filename);
  if (file == NULL) {
    print_error_message("Failed to read input file "
        ANSI_BLUE "%s" ANSI_RESET ": %s\n", filename, strerror(errno));
    exit(1);
  }

  printf("\nTokenizing:\n");
  Token *tokens = malloc(sizeof(*tokens) * MAX_TOKENS);
  tokenize(file, tokens);
  print_tokens(tokens);

  printf("\nCodegen:\n");
  Parser *p = malloc(sizeof(*p));
  parse(file, tokens, p);
  print_nodes(p);

  return 0;
}
