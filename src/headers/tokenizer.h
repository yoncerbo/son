#ifndef INCLUDE_TOKENIZER
#define INCLUDE_TOKENIZER

#include "common.h"
#include <stdint.h>

typedef enum {
  TOK_NONE,

  TOK_DECIMAL,
  TOK_IDENT,

  TOK_SEMICOLON,
  TOK_LBRACE,
  TOK_RBRACE,
  TOK_LPAREN,
  TOK_RPAREN,
  TOK_EQ,
  TOK_PLUS,
  TOK_MINUS,
  TOK_STAR,
  TOK_SLASH,

#define KEYWORDS_START TOK_RETURN
  TOK_RETURN,
  TOK_INT,
#define KEYWORDS_COUNT (TOK_COUNT - TOK_RETURN)
  
  TOK_COUNT,
} TokenTag;

TokenTag TOK_LOOKUP[256] = {
  [';'] = TOK_SEMICOLON,
  ['+'] = TOK_PLUS,
  ['-'] = TOK_MINUS,
  ['*'] = TOK_STAR,
  ['/'] = TOK_SLASH,
  ['{'] = TOK_LBRACE,
  ['}'] = TOK_RBRACE,
  ['='] = TOK_EQ,
  ['('] = TOK_LPAREN,
  [')'] = TOK_RPAREN,
};

// NOTE: using array intilizers, to not update them
// when we change the order
const char *TOK_NAMES[TOK_COUNT] = {
  [TOK_NONE] = "<none>",
  [TOK_IDENT] = "<identfier>",
  [TOK_RETURN] = "return",
  [TOK_DECIMAL] = "<decimal>",
  [TOK_SEMICOLON] = ";",
  [TOK_PLUS] = "+",
  [TOK_MINUS] = "-",
  [TOK_STAR] = "*",
  [TOK_SLASH] = "/",
  [TOK_LBRACE] = "{",
  [TOK_RBRACE] = "}",
  [TOK_INT] = "int",
  [TOK_EQ] = "=",
  [TOK_LPAREN] = "(",
  [TOK_RPAREN] = ")",
};

Str KEYWORDS[KEYWORDS_COUNT] = {
  [TOK_RETURN - KEYWORDS_START] = STR("return"),
  [TOK_INT - KEYWORDS_START] = STR("int"),
};

typedef struct {
  TokenTag tag;
  uint16_t len;
  uint32_t start;
} Token;

#define MAX_TOKENS 256
#define IS_NUMERIC(ch) ((ch) >= '0' && (ch) <= '9')
#define IS_ALPHA(ch) \
    (((ch) >= 'a' && (ch) <= 'z') || ((ch) >= 'A' && (ch) <= 'Z'))

void tokenize(const char *source, Token token_arr[MAX_TOKENS]);
void print_tokens(const Token *token_arr);

#endif
