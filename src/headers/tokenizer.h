#ifndef INCLUDE_TOKENIZER
#define INCLUDE_TOKENIZER

#include "common.h"
#include <stdint.h>

typedef enum {
  TOK_NONE,

  TOK_EQ,
  TOK_BANG,
  TOK_LT,
  TOK_GT,
#define TOK_TWO_CHAR_COUNT TOK_DEQ
  TOK_DEQ,
  TOK_NE,
  TOK_GE,
  TOK_LE,
  TOK_SEMICOLON,
  TOK_LBRACE,
  TOK_RBRACE,
  TOK_LPAREN,
  TOK_RPAREN,
  TOK_PLUS,
  TOK_MINUS,
  TOK_STAR,
  TOK_SLASH,

  TOK_DECIMAL,
  TOK_IDENT,

#define KEYWORDS_START TOK_RETURN
  TOK_RETURN,
  TOK_INT,
  TOK_TRUE,
  TOK_FALSE,
  TOK_IF,
  TOK_ELSE,
  // Debug keywords
  // TODO: maybe add special flag for them
#define KEYWORDS_COUNT (TOK_COUNT - TOK_RETURN)
  
  TOK_COUNT,
} TokenTag;

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
  [TOK_BANG] = "!",
  [TOK_LT] = "<",
  [TOK_GT] = ">",
  [TOK_DEQ] = "==",
  [TOK_NE] = "!=",
  [TOK_GE] = ">=",
  [TOK_LE] = "<=",
  [TOK_TRUE] = "true",
  [TOK_FALSE] = "false",
  [TOK_IF] = "if",
  [TOK_ELSE] = "else",
};

typedef struct {
  TokenTag tag;
  uint16_t len;
  uint32_t start;
} Token;

#define MAX_TOKENS 256

void tokenize(const char *source, Token token_arr[MAX_TOKENS]);
void print_tokens(const Token *token_arr);

#endif
