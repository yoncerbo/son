#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "graphviz.c"
// NOTE: I'm using unity build, so I don't need
// header files and it's been a bother splitting
// files into header and source and also navigating them
// but it was necessary for clangd langauge server.
// I'm gonna try this method and I'll see if it works
// well enough with clangd.
#include "error_reporting.c"
#include "parser.h"
#include "tokenizer.c"
#include "parser.c"

int main(int argc, char *argv[]) {
  assert(argc == 2);
  const char *filename = argv[1];
  printf("Reading file '%s'\n", filename);

  int fd = open(filename, O_RDONLY);
  assert(fd >= 0);

  struct stat st;
  assert(fstat(fd, &st) >= 0);

  const char *file = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  assert(file != MAP_FAILED);

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
