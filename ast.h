#ifndef AST_H
#define AST_H

#include "clomy_test.h"
#include "lexer.h"

/* Debug print */
#define AST_LOG(format, ...)                                                  \
  do                                                                          \
    {                                                                         \
      if (ctx->debug)                                                         \
        fprintf (stdout, "[AST] " format "\n", ##__VA_ARGS__);                \
    }                                                                         \
  while (0)

/* Append AST item NEW to the current block. */
#define AST_APPEND_TO_BLOCK()                                                 \
  do                                                                          \
    {                                                                         \
      if (ctx->debug)                                                         \
        {                                                                     \
          ast_print_tree (new, " ");                                          \
          printf ("\n");                                                      \
        }                                                                     \
      /* Append to WHILE loop. */                                             \
      if (loop)                                                               \
        {                                                                     \
          AST_LOG ("Appending to loop.");                                     \
          while_data = loop->data;                                            \
          while_data->next = new;                                             \
          parent = loop;                                                      \
          loop = (void *)0;                                                   \
        }                                                                     \
      /* Append to IF condition. */                                           \
      else if (cond_stk.size > 0)                                             \
        {                                                                     \
          parent = *(ast_node **)dageti (&cond_stk, 0);                       \
          cond_data = parent->data;                                           \
          /* Append to NO branch. */                                          \
          if (cond_data->no == (void *)0xDEADBEEF)                            \
            {                                                                 \
              AST_LOG ("Appending to condition NO.");                         \
              cond_data->no = new;                                            \
            }                                                                 \
          /* Append to block NO branch. */                                    \
          else if (cond_data->no && cond_data->no != (void *)0xDEADBEEF       \
                   && cond_data->yes->type == AST_BLOCK)                      \
            {                                                                 \
              AST_LOG ("Appending to condition NO branch.");                  \
              blk_data = cond_data->no->data;                                 \
              new->next = blk_data->next;                                     \
              blk_data->next = new;                                           \
            }                                                                 \
          /* Append to YES branch. */                                         \
          else if (!cond_data->yes)                                           \
            {                                                                 \
              AST_LOG ("Appending to condition YES.");                        \
              cond_data->yes = new;                                           \
            }                                                                 \
          /* Append to block YES branch. */                                   \
          else if (cond_data->yes && cond_data->yes->type == AST_BLOCK)       \
            {                                                                 \
              AST_LOG ("Appending to condition YES block.");                  \
              blk_data = cond_data->yes->data;                                \
              new->next = blk_data->next;                                     \
              blk_data->next = new;                                           \
            }                                                                 \
          else                                                                \
            {                                                                 \
              CLOMY_FAIL ("[AST] Unreachable IF placement.");                 \
            }                                                                 \
        }                                                                     \
      /* Append to current block. */                                          \
      else if (block_stk.size > 0)                                            \
        {                                                                     \
          parent = *(ast_node **)dageti (&block_stk, 0);                      \
          AST_LOG ("Appending to block %p.", parent);                         \
          if (parent)                                                         \
            {                                                                 \
              blk_data = parent->data;                                        \
              new->next = blk_data->next;                                     \
              blk_data->next = new;                                           \
            }                                                                 \
          else                                                                \
            {                                                                 \
              CLOMY_FAIL (                                                    \
                  "[AST] Block stack shouldn't contain null pointer.");       \
            }                                                                 \
        }                                                                     \
      /* Append to root. */                                                   \
      else                                                                    \
        {                                                                     \
          AST_LOG ("Appending to root.");                                     \
          new->next = ctx->root;                                              \
          ctx->root = new;                                                    \
          parent = ctx->root;                                                 \
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
  AST_COND,
  AST_WHILE
};

struct ast_node
{
  enum ast_type type;
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
  ast_node *parent;
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

struct ast_data_while
{
  ast_node *cond;
  ast_node *next;
};
typedef struct ast_data_while ast_data_while;

struct ast
{
  arena ar;
  ht ident_table;
  ast_node *root;
  unsigned short debug;
};
typedef struct ast ast;

void ast_print_tree (ast_node *root, char *delim);

ast_node *ast_parse (ast *ctx, lex *lexer, unsigned short debug);

void *ast_parse_expression (ast *ctx, lex *lexer);

void ast_fold (ast *ctx);

#endif /* AST_H */
