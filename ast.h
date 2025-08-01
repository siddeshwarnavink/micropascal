#ifndef AST_H
#define AST_H

#include "lexer.h"

enum ast_type
{
  AST_STRLIT = 0,
  AST_FLOATLIT,
  AST_INTLIT,
  AST_OP
};

struct ast_node
{
  unsigned short type;
  struct ast_node *next;
  void *data;
};
typedef struct ast_node ast_node;

struct ast_data_op
{
  unsigned char op;
  void *left;
  void *right;
};
typedef struct ast_data_op ast_data_op;

struct ast
{
  arena ar;
  ast_node *root;
};
typedef struct ast ast;

void ast_parse (ast *ctx, lex *lexer);

void *ast_parse_expression (ast *ctx, lex *lexer);

#endif /* AST_H */
