#ifndef AST_H
#define AST_H

#include "lexer.h"

#define AST_APPEND_TO_BLOCK()                                                 \
  do                                                                          \
    {                                                                         \
      if (cond)                                                               \
        {                                                                     \
          cond_data = cond->data;                                             \
          if (!cond_data->yes)                                                \
            {                                                                 \
              cond_data->yes = new;                                           \
            }                                                                 \
          else if (cond_data->yes && !cond_else)                              \
            {                                                                 \
              blk_data = cond_data->yes->data;                                \
              new->next = blk_data->next;                                     \
              blk_data->next = new;                                           \
            }                                                                 \
          else if (cond_else)                                                 \
            {                                                                 \
              if (cond_else)                                                  \
                {                                                             \
                  cond_data->no = new;                                        \
                  cond_else = 0;                                              \
                }                                                             \
              cond = (void *)0;                                               \
            }                                                                 \
        }                                                                     \
      else                                                                    \
        {                                                                     \
          blk = *(ast_node **)dageti (&block_stk, block_stk.size - 1);        \
          blk_data = blk->data;                                               \
          new->next = blk_data->next;                                         \
          blk_data->next = new;                                               \
        }                                                                     \
    }                                                                         \
  while (0);

#define AST_ERROR_IF(cond, msg)                                               \
  if ((cond))                                                                 \
    {                                                                         \
      ast_fold (ctx);                                                         \
      lex_error (lexer, (msg));                                               \
      goto ast_err_exit;                                                      \
    }

#define AST_EXPECT_SEMICOLON()                                                \
  AST_ERROR_IF (token != ';', "Expected semicolon (;)");

#define AST_EXPECT_IDENTF()                                                   \
  do                                                                          \
    {                                                                         \
      stringbuilder _err_sb = { 0 };                                          \
      string *_err;                                                           \
      sbinit (&_err_sb, &ctx->ar);                                            \
      sbappend (&_err_sb, "Unknown identifier \"");                           \
      sbappend (&_err_sb, lexer->str->data);                                  \
      sbappendch (&_err_sb, '"');                                             \
      _err = sbflush (&_err_sb);                                              \
      lex_error (lexer, _err->data);                                          \
      goto ast_err_exit;                                                      \
    }                                                                         \
  while (0);

enum ast_type
{
  AST_PROGNAME = 0,
  AST_MAIN_BLOCK,
  AST_VAR_DECLARE,
  AST_VAR_ASSIGN,
  AST_BLOCK,
  AST_FUNCALL,
  AST_STRLIT,
  AST_FLOATLIT,
  AST_INTLIT,
  AST_BOOL,
  AST_OP,
  AST_COND
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
  unsigned long arsize;
};
typedef struct ast_data_var_declare ast_data_var_declare;

struct ast_data_var_assign
{
  ast_node *var;
  ast_node *value;
};
typedef struct ast_data_var_assign ast_data_var_assign;

struct ast_data_block
{
  unsigned short appended;
  ast_node *next;
};
typedef struct ast_data_block ast_data_block;

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

struct ast_data_cond
{
  ast_node *cond;
  ast_node *yes;
  ast_node *no;
};
typedef struct ast_data_cond ast_data_cond;

struct ast
{
  arena ar;
  ht ident_table;
  ast_node *root;
};
typedef struct ast ast;

void ast_print_tree (ast_node *root, char *delim);

ast_node *ast_parse (ast *ctx, lex *lexer);

void *ast_parse_expression (ast *ctx, lex *lexer);

void ast_fold (ast *ctx);

#endif /* AST_H */
