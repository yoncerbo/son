#include "codegen.h"

void parse(const char *source, const Token *tokens, Codegen *cg) {
  *cg = (Codegen){
    .source = source,
    .token_arr = tokens,
  };
}
