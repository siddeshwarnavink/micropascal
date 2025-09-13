#ifndef AST_H
#define AST_H

#include "clomy_test.h"
#include "lexer.h"

/* AST flags. */
#define AST_FLAG_DEBUG (1 << 0)
#define AST_FLAG_FOUND_ENTRY (1 << 1)
#define AST_FLAG_READ_VAR (1 << 2)

/* Debug print */
#define AST_LOG(format, ...)                                                  \
  do                                                                          \
    {                                                                         \
      if (ctx->flags & AST_FLAG_DEBUG)                                        \
        fprintf (stdout, "[AST] " format "\n", ##__VA_ARGS__);                \
    }                                                                         \
  while (0)

#define AST_ERROR_IF(cond, msg)                                               \
  if ((cond))                                                                 \
    {                                                                         \
      ast_fold (ctx);                                                         \
      lex_error (ctx->lexer, (msg));                                          \
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
      sbappend (&_err_sb, ctx->lexer->str->data);                             \
      sbappendch (&_err_sb, '"');                                             \
      _err = sbflush (&_err_sb);                                              \
      lex_error (ctx->lexer, _err->data);                                     \
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
  AST_WHILE,
  AST_STRATEGY_COUNT
};

typedef struct ast_node
{
  enum ast_type type;
  struct ast_node *next;
  void *data;
} ast_node;

typedef struct ast_data_var_declare
{
  string *name;
  U16 datatype;
  U32l arsize;
} ast_data_var_declare;

typedef struct ast_data_var_assign
{
  ast_node *var;
  ast_node *value;
} ast_data_var_assign;

typedef struct ast_data_block
{
  ast_node *parent;
  ast_node *next;
} ast_data_block;

typedef struct ast_data_funcall
{
  string *name;
  ast_node *args_head;
} ast_data_funcall;

typedef struct ast_data_op
{
  U8 op;
  void *left;
  void *right;
} ast_data_op;

typedef struct ast_data_cond
{
  ast_node *cond;
  ast_node *yes;
  ast_node *no;
} ast_data_cond;

typedef struct ast_data_while
{
  ast_node *cond;
  ast_node *next;
} ast_data_while;

typedef struct
{
  arena ar;
  lex *lexer;
  ast_node *root;
  ast_node *currentIndent;
  da *block_stk;
  da *cond_stk;
  da *loop_stk;
  ht *ident_table;
  U8 flags;
} ast;

typedef struct
{
  U8 (*create_condition) (ast *ctx);
  ast_node *(*create) (ast *ctx, void *args);
  void (*print) (ast_node *node);
} ast_strategy;

typedef struct
{
  int token;
  ast_node *var;
} ast_var_assign_arg;

int ast_init (ast *ctx);

ast_node *ast_parse (ast *ctx);

void ast_print_tree (ast_node *root, char *delim);

void ast_fold (ast *ctx);

#endif /* AST_H */
