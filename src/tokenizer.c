#include "tokenizer.h"
#include "common.h"
#include "error_reporting.c"
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

void tokenize(const char *source, Token token_arr[MAX_TOKENS]) {
  const char *ch = source;
  int tokens_len = 0;

  // for every token
  while (*ch) {
    // Skip whitespaces
    while (*ch == ' ' || *ch == '\n' || *ch == '\t') ch++;
    if (!*ch) break;

    if (*ch == '/' && ch[1] == '/') {
      ch += 2;
      while (*ch != '\n' && *ch != 0) ch++;
      continue;
    }

    TokenTag tt = TOK_LOOKUP[(uint8_t)*ch];
    if (tt && tt < TOK_TWO_CHAR_COUNT) {
      assert(TOK_SECOND_CHAR[tt].tag);
      if (ch[1] == TOK_SECOND_CHAR[tt].ch) {
        assert(tokens_len < MAX_TOKENS);
        tt = TOK_SECOND_CHAR[tt].tag;
        token_arr[tokens_len++] = (Token){ tt, 2, ch - source };
        ch += 2;
        continue;
      }
    }

    if (tt) {
      assert(tokens_len < MAX_TOKENS);
      token_arr[tokens_len++] = (Token){ tt, 1, ch - source };
      ch++;
      continue;
    }

    if (IS_NUMERIC(*ch)) {
      const char *start = ch++;
      while (IS_NUMERIC(*ch)) ch++;
      assert(tokens_len < MAX_TOKENS);
      token_arr[tokens_len++] = (Token){ TOK_DECIMAL, ch - start, start - source };
      continue;
    }

    if (*ch == '_' || IS_ALPHA(*ch)) {
      const char *start = ch++;
      while (*ch == '_' || IS_ALPHA(*ch) || IS_NUMERIC(*ch)) ch++;

      uint32_t len = ch - start;
      TokenTag tt = TOK_IDENT;
      for (int i = 0; i < KEYWORDS_COUNT; ++i) {
        if (KEYWORDS[i].len != len) continue;
        if (strncmp(KEYWORDS[i].ptr, start, len)) continue;
        tt = KEYWORDS_START + i;
        break;
      }
      assert(tokens_len < MAX_TOKENS);
      token_arr[tokens_len++] = (Token){ tt, len, start - source };
      continue;
    }

    print_error(source, ch - source, 1, "Unkown character: '%c'", *ch);
  }
  // EOF token
  assert(tokens_len < MAX_TOKENS);
  token_arr[tokens_len++] = (Token){0};
}

void print_tokens(const Token *tokens) {
  while (tokens->tag) {
    printf("% 3d:%02d %s\n", tokens->start, tokens->len, TOK_NAMES[tokens->tag]);
    tokens++;
  }
}
