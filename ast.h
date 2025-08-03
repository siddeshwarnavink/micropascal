#ifndef AST_H
#define AST_H

#include "lexer.h"

#define AST_ERROR_IF(cond, msg)                                                 \
  if ((cond))                                                                 \
    {                                                                         \
      ast_fold (ctx);                                                         \
      lex_error (lexer, (msg));                                               \
      goto ast_err_exit;                                                      \
    }

#define AST_EXPECT_SEMICOLON() AST_ERROR_IF (token != ';', "Expected semicolon (;)");

enum ast_type
{
  AST_PROGNAME = 0,
  AST_MAIN_BLOCK,
  AST_VAR_DECLARE,
  AST_BLOCK,
  AST_FUNCALL,
  AST_STRLIT,
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

struct ast_data_var_declare
{
  string *name;
  unsigned short datatype;
  ast_node *value;
};
typedef struct ast_data_var_declare ast_data_var_declare;

struct ast_data_funcall
{
  string *name;
  ast_node *args_head;
};
typedef struct ast_data_funcall ast_data_funcall;

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
  ht ident_table;
  ast_node *root;
};
typedef struct ast ast;

ast_node *ast_parse (ast *ctx, lex *lexer);

void *ast_parse_expression (ast *ctx, lex *lexer);

void ast_fold (ast *ctx);

#endif /* AST_H */
