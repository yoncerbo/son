#include "parser.h"
#include "tokenizer.h"

const char *GRAPH_FILENAME = "out/graph.dot";
const Str GRAPH_BUILTIN_NAME = STR("graph");
const Str PRINT_AST_BUILTIN_NAME = STR("nodes");

NodeId Parser_parse_statement(Parser *p);

NodeId Parser_parse_block(Parser *p) {
  Parser_push_scope(p);
  assert(p->pos);
  Token start = p->token_arr[p->pos - 1];
  NodeId node = 0;
  while (p->token_arr[p->pos].tag != TOK_RBRACE) {
    Token tok = p->token_arr[p->pos];
    if (!tok.tag) {
      print_error(p->source, start.start, start.len, "No matching `}` found before EOF");
      exit(1);
    }
    NodeId elem = Parser_parse_statement(p);
    if (elem) node = elem;
  }
  p->pos++;
  Parser_pop_scope(p);
  return node;
}

NodeId Parser_parse_statement(Parser *p) {
  NodeId node = 0, value = 0;
  Token tok = p->token_arr[p->pos++];
  switch (tok.tag) {
    case TOK_IF:
      // duplicate the scope
      Parser_expect_token(p, TOK_LPAREN);
      NodeId cond = Parser_parse_expression(p);
      Parser_expect_token(p, TOK_RPAREN);
      node = Parser_create_if_node(p, cond);
      NodeId then_block = Parser_create_proj_node(p, node, 0);
      NodeId else_block = Parser_create_proj_node(p, node, 1);

      // TODO: We don't need a copy of start and len, only node
      uint16_t right_start = p->var_len;
      uint16_t left_start = p->var_offset;
      uint16_t var_len = right_start - left_start;
      assert((p->var_len += var_len) < MAX_VAR_DEPTH);
      memcpy(&p->var_arr[right_start], &p->var_arr[left_start], var_len * sizeof(Var));
      NodeId new_scope = Parser_duplicate_scopes(p, right_start);
      NodeId prev_scope = p->scope;

      // Then block
      p->scope = new_scope;
      p->var_offset = right_start;
      Parser_push_scope(p);
      Parser_update_ctrl(p, then_block);
      Parser_parse_statement(p);
      then_block = Parser_resolve_ctrl(p);
      Parser_pop_scope(p);
      p->scope = prev_scope;
      p->var_offset = left_start;

      // Else block
      if (p->token_arr[p->pos].tag == TOK_ELSE) {
        p->pos++;
        Parser_push_scope(p);
        Parser_update_ctrl(p, else_block);
        Parser_parse_statement(p);
        else_block = Parser_resolve_ctrl(p);
        Parser_pop_scope(p);
      }
      NodeId region = Parser_create_region_node(p, then_block, else_block);

      p->node_arr[p->scope].value.scope.ctrl = region;
      Parser_create_phi_nodes(p, right_start, region, new_scope);
      p->var_len = right_start;
      Parser_update_ctrl(p, region);
      break;
    case TOK_IDENT:
      // TODO: add debug flag
      if (tok.len == GRAPH_BUILTIN_NAME.len &&
          !strncmp(GRAPH_BUILTIN_NAME.ptr, &p->source[tok.start], tok.len)) {
        output_graphviz_file(GRAPH_FILENAME, p);
        Parser_expect_token(p, TOK_LPAREN);
        Parser_expect_token(p, TOK_RPAREN);
        Parser_expect_token(p, TOK_SEMICOLON);
        break;
      }
      if (tok.len == PRINT_AST_BUILTIN_NAME.len &&
          !strncmp(PRINT_AST_BUILTIN_NAME.ptr, &p->source[tok.start], tok.len)) {
        print_nodes(p);
        Parser_expect_token(p, TOK_LPAREN);
        Parser_expect_token(p, TOK_RPAREN);
        Parser_expect_token(p, TOK_SEMICOLON);
        break;
      }
      Parser_expect_token(p, TOK_EQ);
      value = Parser_parse_expression(p);
      Parser_update_var(p, tok.start, tok.len, value);
      break;
    case TOK_INT:
      tok = Parser_expect_token(p, TOK_IDENT);
      Parser_expect_token(p, TOK_EQ);
      value = Parser_parse_expression(p);
      Parser_push_var(p, tok.start, tok.len, value);
      break;
    case TOK_SEMICOLON:
      break;
    case TOK_LBRACE:
      node = Parser_parse_block(p);
      break;
    case TOK_RETURN:
      if (p->token_arr[p->pos].tag != TOK_SEMICOLON) {
        value = Parser_parse_expression(p);
        Parser_expect_token(p, TOK_SEMICOLON);
      } else p->pos++;
      return Parser_create_return_node(p, value);
      break;
    default: 
      print_error(p->source, tok.start, tok.len,
        "Expected statement, got `%s`", TOK_NAMES[tok.tag]);
      exit(1);
  }
  return node;
}

NodeId Parser_parse_top_level(Parser *p) {
  Parser_push_scope(p);
  p->node_arr[p->scope].value.scope.ctrl = START_NODE;
  NodeId node = 0;
  while (p->token_arr[p->pos].tag != TOK_NONE) {
    NodeId elem = Parser_parse_statement(p);
    if (elem) node = elem;
  }
  Parser_pop_scope(p);
  return node;
}

