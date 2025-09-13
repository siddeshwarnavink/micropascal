#include "ast.h"
#include "utils.h"

/* Parse an expression. */
static void *_ast_parse_expression (ast *ctx, lex *lexer);

/* Append expression to current block */
static void _append_to_block (ast *ctx, ast_node *new);

/* Get strategy given AST type. */
static const ast_strategy *_ast_get_strategy (enum ast_type type);

/* Reverse node AST linked-list. */
static inline ast_node *_reverse_ast_list (ast_node *head);

/* Get operator precedence for given operator. */
static int _get_precedence (char op);

/* Create new AST node. */
static ast_node *_ast_new_node (ast *ctx, short type);

/* Create new AST operation node. */
static ast_node *_create_op_node (ast *ctx, char op, da *value_stk);

/* Print datatype for given AST type. */
static void _ast_print_datatype (U16 dtype);

/* Print AST node. */
static void _ast_print_node (ast_node *n);

/* Check if a token is operator. */
static int _is_token_op (int tok);

/* Check if token is a valid expression terminator */
static int _is_expression_terminator (int tok);

// [ Program Name ] {{{
static ast_node *
_create_progname (ast *ctx, void *args)
{
  (void)args;

  int token = lex_next_token (ctx->lexer);
  AST_ERROR_IF (token != TOKEN_PROGRAM,
                "Expected \"program\" statement at beginning of program.");

  token = lex_next_token (ctx->lexer);
  AST_ERROR_IF (token != TOKEN_IDENTF, "Expected program name.");

  token = lex_next_token (ctx->lexer);
  AST_EXPECT_SEMICOLON ();

  ast_node *new = _ast_new_node (ctx, AST_PROGNAME);
  new->data = stringcpy (&ctx->ar, ctx->lexer->str);
  return new;

ast_err_exit:
  AST_LOG ("Program name error exit.");
  return NULL;
}

static void
_print_progname (ast_node *node)
{
  printf ("(program %s)", ((string *)node->data)->data);
}

const ast_strategy ast_progname_strategy
    = { .create = _create_progname, .print = _print_progname };
// }}}
// [ VAR declaration ] {{{
static ast_node *
_create_var_declare (ast *ctx, void *args)
{
  (void)args;

  ast_node *new;
  ast_data_var_declare *data;
  string *var_name, *datatype;
  int token;

  var_name = stringcpy (&ctx->ar, ctx->lexer->str);

  token = lex_next_token (ctx->lexer);
  AST_ERROR_IF (token != ':', "Expected ':'");

  token = lex_next_token (ctx->lexer);
  AST_ERROR_IF (token != TOKEN_IDENTF, "Expected data type of variable.");
  datatype = stringcpy (&ctx->ar, ctx->lexer->str);

  new = _ast_new_node (ctx, AST_VAR_DECLARE);
  data = aralloc (&ctx->ar, sizeof (ast_data_var_declare));
  data->name = var_name;
  new->data = data;

  if (streq (datatype->data, "integer"))
    data->datatype = AST_INTLIT;
  else if (streq (datatype->data, "real"))
    data->datatype = AST_FLOATLIT;
  else if (streq (datatype->data, "string"))
    data->datatype = AST_STRLIT;
  else if (streq (datatype->data, "boolean"))
    data->datatype = AST_BOOL;
  else
    AST_ERROR_IF (true, "Unknown datatype.");

  token = lex_next_token (ctx->lexer);
  if (token == '[')
    {
      token = lex_next_token (ctx->lexer);
      AST_ERROR_IF (token != TOKEN_INTLIT, "Expected integer for array size.");
      data->arsize = ctx->lexer->int_num;

      token = lex_next_token (ctx->lexer);
      AST_ERROR_IF (token != ']', "Expected ']'");

      token = lex_next_token (ctx->lexer);
    }

  AST_EXPECT_SEMICOLON ();

  stput (ctx->ident_table, data->name->data, &new);

  if (strcmp (datatype->data, "string") == 0 && data->arsize < 1)
    {
      data->arsize = 256;
    }

  return new;

ast_err_exit:
  AST_LOG ("VAR declaration error exit.");
  return NULL;
}

static void
_print_var_declare (ast_node *node)
{
  ast_data_var_declare *data = node->data;
  printf ("(var %s ", data->name->data);
  _ast_print_datatype (data->datatype);
  printf (")");
}

const ast_strategy ast_var_declare_strategy
    = { .create = _create_var_declare, .print = _print_var_declare };
// }}}
// [ VAR assign ] {{{
static ast_node *
_create_var_assign (ast *ctx, void *args)
{
  ast_node *new, *exp;
  ast_data_var_assign *data;
  ast_var_assign_arg *arg_data = args;
  int token = arg_data->token;

  AST_ERROR_IF (arg_data->var->type != AST_VAR_DECLARE,
                "Expected identifier variable.");
  AST_ERROR_IF (token != TOKEN_INFEQ, "Expected ':='");

  new = _ast_new_node (ctx, AST_VAR_ASSIGN);
  data = aralloc (&ctx->ar, sizeof (ast_data_var_assign));
  data->var = arg_data->var;

  exp = _ast_parse_expression (ctx, ctx->lexer);
  if (exp)
    {
      token = lex_next_token (ctx->lexer);
      AST_EXPECT_SEMICOLON ();

      /* TODO: Report error if datatype mismatch. */

      data->value = exp;
      new->data = data;
      _append_to_block (ctx, new);
    }
  else
    {
      goto ast_err_exit;
    }

  return new;

ast_err_exit:
  AST_LOG ("VAR assign error exit.");
  return NULL;
}

static void
_print_var_assign (ast_node *node)
{
  ast_data_var_assign *assign_data = node->data;
  ast_data_var_declare *data = assign_data->var->data;
  printf ("(var %s ", data->name->data);
  _ast_print_node (assign_data->value);
  printf (")");
}

const ast_strategy ast_var_assign_strategy
    = { .create = _create_var_assign, .print = _print_var_assign };
// }}}
//  [ Block - BEGIN and END ] {{{
static ast_node *
_create_block (ast *ctx, void *args)
{
  ast_node *new, *block, *cond;
  ast_data_block *data;
  ast_data_block *block_data;
  U32 i;

  /* BEGIN */
  if (args == (void *)0x0)
    {
      ctx->flags &= ~AST_FLAG_READ_VAR;
      new = _ast_new_node (ctx, AST_BLOCK);
      AST_LOG ("Block %p begin.", new);

      data = aralloc (&ctx->ar, sizeof (ast_data_block));
      data->parent = ctx->currentIndent;
      new->data = data;

      _append_to_block (ctx, new);

      dapush (ctx->block_stk, &new);
    }
  /* END */
  else
    {
      AST_ERROR_IF (ctx->block_stk->size == 0, "Invalid use of \"end\".");
      block = *(ast_node **)dageti (ctx->block_stk, 0);
      AST_LOG ("End of Block %p.", block);
      block_data = block->data;
      block_data->next = _reverse_ast_list (block_data->next);

      /* Remove all child conditions of the block. */
      new = block_data->next;
      while (new)
        {
          if (new->type == AST_COND)
            {
              for (i = 0; i < ctx->cond_stk->size; ++i)
                {
                  cond = *(ast_node **)dageti (ctx->cond_stk, i);
                  if (cond == new)
                    {
                      AST_LOG ("Remove child condition from stack.");
                      if (ctx->flags & AST_FLAG_DEBUG)
                        ast_print_tree (cond, " ");
                      dadel (ctx->cond_stk, i);
                    }
                }
            }
          new = new->next;
        }

      dadel (ctx->block_stk, 0);
    }

  return new;

ast_err_exit:
  AST_LOG ("Block error exit.");
  return NULL;
}

static void
_print_block (ast_node *node)
{
  ast_data_block *block_data = node->data;
  printf ("(block\n  ");
  ast_print_tree (block_data->next, "\n  ");
  printf (")");
}

const ast_strategy ast_block_strategy
    = { .create = _create_block, .print = _print_block };
// }}}
// [ Funcation call ] {{{
static ast_node *
_create_funcall (ast *ctx, void *args)
{
  string *str_data = args;
  ast_node *new, *expression;
  ast_data_funcall *data;
  int token;

  new = _ast_new_node (ctx, AST_FUNCALL);
  data = aralloc (&ctx->ar, sizeof (ast_data_funcall));
  data->name = str_data;
  data->args_head = (void *)0;
  new->data = data;

  token = lex_peek (ctx->lexer);
  while (!_is_expression_terminator (token))
    {
      expression = _ast_parse_expression (ctx, ctx->lexer);
      if (expression)
        {
          expression->next = data->args_head;
          data->args_head = expression;
        }
      else
        {
          AST_ERROR_IF (true, "Invalid expression.");
        }

      token = lex_peek (ctx->lexer);
      if (token == ',')
        {
          lex_next_token (ctx->lexer);
          token = lex_peek (ctx->lexer);
        }
    }

  if (token == ')')
    {
      lex_next_token (ctx->lexer);
      token = lex_peek (ctx->lexer);
    }

  if (!(ctx->cond_stk->size > 0 && token == TOKEN_ELSE))
    AST_EXPECT_SEMICOLON ();

  data->args_head = _reverse_ast_list (data->args_head);
  _append_to_block (ctx, new);

  return new;

ast_err_exit:
  AST_LOG ("Function call error exit.");
  return NULL;
}

static void
_print_funcall (ast_node *node)
{
  ast_data_funcall *funcall_data = node->data;
  printf ("(%s ", funcall_data->name->data);
  ast_print_tree (funcall_data->args_head, " ");
  printf (")");
}

const ast_strategy ast_funcall_strategy
    = { .create = _create_funcall, .print = _print_funcall };
// }}}
// [ IF - ELSE condition {{{
static ast_node *
_create_cond (ast *ctx, void *args)
{
  ast_node *new, *expression, *cond;
  ast_data_cond *data;
  int token;

  /* IF */
  if (args == (void *)0x0)
    {
      AST_ERROR_IF (ctx->block_stk->size < 1,
                    "IF should be used inside a block.");
      new = _ast_new_node (ctx, AST_COND);
      data = aralloc (&ctx->ar, sizeof (ast_data_cond));
      expression = _ast_parse_expression (ctx, ctx->lexer);
      if (expression)
        {
          data->cond = expression;
          token = lex_next_token (ctx->lexer);
          AST_ERROR_IF (token != TOKEN_THEN,
                        "Expected \"then\" after condition.");
        }
      else
        {
          AST_ERROR_IF (true, "Expected condition expression after \"if\".");
        }
      new->data = data;
      _append_to_block (ctx, new);
      dapush (ctx->cond_stk, &new);
    }
  /* ELSE */
  else
    {
      if (ctx->cond_stk->size == 0)
        AST_ERROR_IF (1, "Invalid usage of \"else\".");

      cond = *(ast_node **)dageti (ctx->cond_stk, 0);
      data = cond->data;
      data->no = (void *)0xDEADBEEF;

      if (!data->yes)
        AST_ERROR_IF (1, "Invalid usage of \"else\".");

      AST_LOG ("Found ELSE for IF.");
      if (ctx->flags & AST_FLAG_DEBUG)
        ast_print_tree (cond, " ");
    }

  return new;

ast_err_exit:
  AST_LOG ("Condition error exit.");
  return NULL;
}

static void
_print_cond (ast_node *node)
{
  ast_data_cond *data = node->data;
  printf ("(if ");
  _ast_print_node (data->cond);
  printf ("\n    ");
  if (data->yes)
    ast_print_tree (data->yes, "\n  ");
  if (data->no)
    {
      printf ("\n    ");
      if (data->no == (void *)0xDEADBEEF)
        printf ("0xDEADBEEF");
      else
        ast_print_tree (data->no, "\n  ");
    }
  printf (")");
}

const ast_strategy ast_cond_strategy
    = { .create = _create_cond, .print = _print_cond };
// }}}
// [ WHILE ] {{{
static ast_node *
_create_while (ast *ctx, void *args)
{
  ast_node *new, *exp;
  ast_data_while *data;
  int token;

  (void)args;

  AST_ERROR_IF (ctx->block_stk->size < 1,
                "WHILE should be used inside a block.");
  new = _ast_new_node (ctx, AST_WHILE);
  data = aralloc (&ctx->ar, sizeof (ast_data_while));
  exp = _ast_parse_expression (ctx, ctx->lexer);
  if (exp)
    {
      data->cond = exp;
      token = lex_next_token (ctx->lexer);
      AST_ERROR_IF (token != TOKEN_DO, "Expected \"do\" after while.");
    }
  else
    {
      AST_ERROR_IF (true, "Expected condition expression.");
    }

  new->data = data;
  _append_to_block (ctx, new);

  dapush (ctx->loop_stk, new);

  return new;

ast_err_exit:
  AST_LOG ("While error exit");
  return NULL;
}

static void
_print_while (ast_node *node)
{
  ast_data_while *data = node->data;
  printf ("(while ");
  _ast_print_node (data->cond);
  printf ("\n    ");
  ast_print_tree (data->next, "\n  ");
  printf (")");
}

const ast_strategy ast_while_strategy
    = { .create = _create_while, .print = _print_while };
// }}}

static const ast_strategy *strategy_registry[AST_STRATEGY_COUNT]
    = { [AST_PROGNAME] = &ast_progname_strategy,
        [AST_MAIN_BLOCK] = &ast_block_strategy,
        [AST_BLOCK] = &ast_block_strategy,
        [AST_VAR_DECLARE] = &ast_var_declare_strategy,
        [AST_VAR_ASSIGN] = &ast_var_assign_strategy,
        [AST_FUNCALL] = &ast_funcall_strategy,
        [AST_COND] = &ast_cond_strategy,
        [AST_WHILE] = &ast_while_strategy };

int
ast_init (ast *ctx)
{
  ctx->block_stk = aralloc (&ctx->ar, sizeof (da));
  ctx->cond_stk = aralloc (&ctx->ar, sizeof (da));
  ctx->loop_stk = aralloc (&ctx->ar, sizeof (da));
  ctx->ident_table = aralloc (&ctx->ar, sizeof (ht));

  dainit (ctx->block_stk, &ctx->ar, 16, sizeof (ast_node *));
  dainit (ctx->cond_stk, &ctx->ar, 16, sizeof (ast_node *));
  dainit (ctx->loop_stk, &ctx->ar, 16, sizeof (ast_node *));
  htinit (ctx->ident_table, &ctx->ar, 16, sizeof (ast_node *));

  return 0;
}

ast_node *
ast_parse (ast *ctx)
{
  ast_node *new;
  ast_data_cond *cond_data;
  ast_var_assign_arg var_assign_arg = { 0 };
  string *str_data;
  void *ptr;
  int token;

  ctx->root = NULL;

  /* Program name. */
  new = _ast_get_strategy (AST_PROGNAME)->create (ctx, NULL);
  if (!new)
    goto ast_err_exit;

  ctx->root = new;

  while ((token = lex_next_token (ctx->lexer)) != TOKEN_END)
    {
      /*  Handle end of condition. */
      if (ctx->cond_stk->size > 0)
        {
          cond_data = (*(ast_node **)dageti (ctx->cond_stk, 0))->data;
          AST_ERROR_IF (
              !cond_data,
              "Unreachable. Condition stack shouldn't have null pointer.");

          /* Remove IF if it is completed. */
          if (cond_data->yes && cond_data->no
              && cond_data->no != (void *)0xDEADBEEF)
            {
              AST_LOG ("Getting rid of IF (else found).");
              if (ctx->flags & AST_FLAG_DEBUG)
                lex_print_token (ctx->lexer, token);
              dadel (ctx->cond_stk, 0);
            }
        }
      if (token == TOKEN_ELSE)
        {
          /* 1 signify ELSE. */
          new = _ast_get_strategy (AST_COND)->create (ctx, (void *)0x1);
          if (!new)
            goto ast_err_exit;

          continue;
        }
      if (token == TOKEN_VAR)
        {
          AST_ERROR_IF (ctx->block_stk->size > 0,
                        "Variable declaration not allowed inside block.");
          ctx->flags |= AST_FLAG_READ_VAR;
        }
      else if (token == TOKEN_BEGIN)
        {
          /* 0 signify block begin. */
          _ast_get_strategy (AST_BLOCK)->create (ctx, (void *)0x0);
          if (!new)
            goto ast_err_exit;
        }
      else if (token == TOKEN_BLOCK_END)
        {
          /* 1 signify block end. */
          _ast_get_strategy (AST_BLOCK)->create (ctx, (void *)0x1);
          if (!new)
            goto ast_err_exit;
        }
      else if ((ctx->flags & AST_FLAG_READ_VAR) && token == TOKEN_IDENTF)
        {
          new = _ast_get_strategy (AST_VAR_DECLARE)->create (ctx, NULL);
          if (!new)
            goto ast_err_exit;

          new->next = ctx->root;
          ctx->root = new;
        }
      else if (token == TOKEN_WHILE)
        {
          new = _ast_get_strategy (AST_WHILE)->create (ctx, NULL);
          if (!new)
            goto ast_err_exit;
        }
      else if (token == TOKEN_IF)
        {
          /* 0 signify IF. */
          new = _ast_get_strategy (AST_COND)->create (ctx, (void *)0x0);
          if (!new)
            goto ast_err_exit;
        }
      else if (ctx->block_stk->size > 0 && token == TOKEN_IDENTF)
        {
          str_data = stringcpy (&ctx->ar, ctx->lexer->str);
          token = lex_next_token (ctx->lexer);

          if (token == '(')
            {
              new = _ast_get_strategy (AST_FUNCALL)->create (ctx, str_data);
              if (!new)
                goto ast_err_exit;
            }
          else if ((ptr = stget (ctx->ident_table, str_data->data))
                   != (void *)0)
            {
              var_assign_arg.token = token;
              var_assign_arg.var = *((ast_node **)ptr);
              new = _ast_get_strategy (AST_VAR_ASSIGN)
                        ->create (ctx, &var_assign_arg);
              if (!new)
                goto ast_err_exit;
            }
          else
            {
              AST_EXPECT_IDENTF ();
            }
        }
      else
        {
          switch (token)
            {
            /* Main block. */
            case '.':
              if (!(ctx->flags & AST_FLAG_FOUND_ENTRY)
                  && ctx->root->type == AST_BLOCK)
                {
                  AST_LOG ("Found entry block.");
                  ctx->flags |= AST_FLAG_FOUND_ENTRY;
                  ctx->root->type = AST_MAIN_BLOCK;
                }
              break;
            default:
              AST_LOG ("Else case token (%d) '%c'", token, (char)token);
              break;
            }
        }
    }

  AST_ERROR_IF (!(ctx->flags & AST_FLAG_FOUND_ENTRY),
                "Cannot find entry point.");

  if (ctx->root)
    ctx->root = _reverse_ast_list (ctx->root);

  /* TODO: Report unclosed blocks. */

  return ctx->root;

ast_err_exit:
  AST_LOG ("ast_parse error exit.");
  return NULL;
}

void
ast_print_tree (ast_node *root, char *delim)
{
  while (root)
    {
      _ast_print_node (root);
      if (root->next)
        printf ("%s", delim);

      root = root->next;
    }
}

void
ast_fold (ast *ctx)
{
  arfold (&ctx->ar);
}

void *
_ast_parse_expression (ast *ctx, lex *lexer)
{
  da value_stk = { 0 }, op_stk = { 0 };
  ast_node *new, *node, *var;
  long *int_data;
  double *float_data;
  void *ptr;
  int token, current_prec, top_prec;
  char current_op, top_op;
  U16 *bool_data;

  dainit (&value_stk, &ctx->ar, sizeof (ast_node), 8);
  dainit (&op_stk, &ctx->ar, sizeof (char), 8);

  token = lex_next_token (lexer);

  while (!_is_expression_terminator (token))
    {
      if (token == TOKEN_STRLIT)
        {
          new = _ast_new_node (ctx, AST_STRLIT);
          new->data = stringcpy (&ctx->ar, lexer->str);

          dafold (&value_stk);
          dafold (&op_stk);

          return new;
        }
      else if (token == TOKEN_INTLIT || token == TOKEN_FLOATLIT)
        {
          if (token == TOKEN_INTLIT)
            {
              new = _ast_new_node (ctx, AST_INTLIT);
              int_data = aralloc (&ctx->ar, sizeof (long));
              *int_data = lexer->int_num;
              new->data = int_data;
            }
          else
            {
              new = _ast_new_node (ctx, AST_FLOATLIT);
              float_data = aralloc (&ctx->ar, sizeof (double));
              *float_data = lexer->float_num;
              new->data = float_data;
            }
          dapush (&value_stk, new);
        }
      else if (token == TOKEN_IDENTF)
        {
          if ((ptr = stget (ctx->ident_table, lexer->str->data)) != (void *)0)
            {
              var = *((ast_node **)ptr);

              AST_ERROR_IF (var->type != AST_VAR_DECLARE,
                            "Expected identifier variable.");
              dapush (&value_stk, var);
            }
          else if (strcmp (lexer->str->data, "true") == 0
                   || strcmp (lexer->str->data, "false") == 0)
            {
              new = _ast_new_node (ctx, AST_BOOL);
              bool_data = aralloc (&ctx->ar, sizeof (U16));
              *bool_data = strcmp (lexer->str->data, "true") == 0 ? 1 : 0;
              new->data = bool_data;
              return new;
            }
          else
            {
              AST_EXPECT_IDENTF ();
            }
        }
      else if (token == '(')
        {
          current_op = (char)token;
          dapush (&op_stk, &current_op);
        }
      else if (token == ')')
        {
          while (op_stk.size > 0)
            {
              top_op = *(char *)dageti (&op_stk, 0);

              if (top_op == '(')
                {
                  dadel (&op_stk, 0);
                  break;
                }
              else
                {
                  dadel (&op_stk, 0);
                  node = _create_op_node (ctx, top_op, &value_stk);
                  dapush (&value_stk, node);
                }
            }
        }
      else if (_is_token_op (token))
        {
          current_op = (char)token;
          current_prec = _get_precedence (current_op);

          while (op_stk.size > 0)
            {
              top_op = *(char *)dageti (&op_stk, 0);
              if (top_op == '(')
                break;

              top_prec = _get_precedence (top_op);

              if (top_prec > current_prec)
                {
                  dadel (&op_stk, 0);
                  node = _create_op_node (ctx, top_op, &value_stk);
                  dapush (&value_stk, node);
                }
              else
                {
                  break;
                }
            }

          dapush (&op_stk, &current_op);
        }
      else
        {
          break;
        }

      if (lex_peek (lexer) == ';')
        break;

      if (lex_peek (lexer) == ')' && op_stk.size == 0)
        break;

      if (lex_peek (lexer) == TOKEN_THEN || lex_peek (lexer) == TOKEN_DO)
        break;

      token = lex_next_token (lexer);
    }

  while (op_stk.size > 0)
    {
      top_op = *(char *)dapop (&op_stk);

      if (top_op != '(')
        {
          node = _create_op_node (ctx, top_op, &value_stk);
          dapush (&value_stk, node);
        }
    }

  if (value_stk.size > 0)
    return dageti (&value_stk, 0);

ast_err_exit:
  return (ast_node *)0;
}

void
_append_to_block (ast *ctx, ast_node *new)
{
  ast_data_while *while_data;
  ast_data_cond *cond_data;
  ast_data_block *block_data;

  if (ctx->flags & AST_FLAG_DEBUG)
    {
      ast_print_tree (new, " ");
      printf ("\n");
    }
  /* Append to WHILE loop. */
  else if (ctx->loop_stk->size > 0)
    {
      ctx->currentIndent = *(ast_node **)dageti (ctx->block_stk, 0);
      AST_LOG ("Appending to loop %p.", ctx->currentIndent);
      if (ctx->currentIndent)
        {
          while_data = ctx->currentIndent->data;
          new->next = while_data->next;
          while_data->next = new;
        }
      else
        {
          CLOMY_FAIL ("[AST] Block stack shouldn't contain null pointer.");
        }
    }
  /* Append to IF condition. */
  else if (ctx->cond_stk->size > 0)
    {
      ctx->currentIndent = *(ast_node **)dageti (ctx->cond_stk, 0);
      cond_data = ctx->currentIndent->data;
      /* Append to NO branch. */
      if (cond_data->no == (void *)0xDEADBEEF)
        {
          AST_LOG ("Appending to condition NO.");
          cond_data->no = new;
        }
      /* Append to block NO branch. */
      else if (cond_data->no && cond_data->no != (void *)0xDEADBEEF
               && cond_data->yes->type == AST_BLOCK)
        {
          AST_LOG ("Appending to condition NO branch.");
          block_data = cond_data->no->data;
          new->next = block_data->next;
          block_data->next = new;
        }
      /* Append to YES branch. */
      else if (!cond_data->yes)
        {
          AST_LOG ("Appending to condition YES.");
          cond_data->yes = new;
        }
      /* Append to block YES branch. */
      else if (cond_data->yes && cond_data->yes->type == AST_BLOCK)
        {
          AST_LOG ("Appending to condition YES block.");
          block_data = cond_data->yes->data;
          new->next = block_data->next;
          block_data->next = new;
        }
      else
        {
          CLOMY_FAIL ("[AST] Unreachable IF placement.");
        }
    }
  /* Append to current block. */
  else if (ctx->block_stk->size > 0)
    {
      ctx->currentIndent = *(ast_node **)dageti (ctx->block_stk, 0);
      AST_LOG ("Appending to block %p.", ctx->currentIndent);
      if (ctx->currentIndent)
        {
          block_data = ctx->currentIndent->data;
          new->next = block_data->next;
          block_data->next = new;
        }
      else
        {
          CLOMY_FAIL ("[AST] Block stack shouldn't contain null pointer.");
        }
    }
  /* Append to root. */
  else
    {
      AST_LOG ("Appending to root.");
      new->next = ctx->root;
      ctx->root = new;
      ctx->currentIndent = ctx->root;
    }
}

int
_get_precedence (char op)
{
  switch (op)
    {
    case '+':
    case '-':
      return 1;
    case '*':
    case '/':
    case '%':
      return 2;
    default:
      return 0;
    }
}

static ast_node *
_ast_new_node (ast *ctx, short type)
{
  ast_node *new = aralloc (&ctx->ar, sizeof (ast_node));
  new->type = type;
  new->next = (void *)0;
  return new;
}

static ast_node *
_create_op_node (ast *ctx, char op, da *value_stk)
{
  ast_node *new_node = _ast_new_node (ctx, AST_OP);
  ast_data_op *op_data = aralloc (&ctx->ar, sizeof (ast_data_op));

  op_data->op = op;

  op_data->right = dapop (value_stk);
  if (op != '!')
    op_data->left = dapop (value_stk);

  new_node->data = op_data;

  return new_node;
}

static void
_ast_print_datatype (U16 dtype)
{
  switch (dtype)
    {
    case AST_INTLIT:
      printf ("integer");
      break;
    case AST_STRLIT:
      printf ("string");
      break;
    case AST_FLOATLIT:
      printf ("real");
      break;
    case AST_BOOL:
      printf ("boolean");
      break;
    default:
      printf ("%d?", dtype);
      break;
    }
}

static void
_ast_print_node (ast_node *n)
{
  const ast_strategy *strat;
  ast_data_op *op_data;

  switch (n->type)
    {
    case AST_STRLIT:
      printf ("\"%s\"", ((string *)n->data)->data);
      break;
    case AST_INTLIT:
      printf ("%ld", *(long *)n->data);
      break;
    case AST_BOOL:
      printf ("%s", *((U16 *)n->data) == 1 ? "true" : "false");
      break;
    case AST_FLOATLIT:
      printf ("%f", *(double *)n->data);
      break;
    case AST_OP:
      op_data = n->data;
      printf ("(%c ", op_data->op);
      if (op_data->left)
        {
          _ast_print_node (op_data->left);
          printf (" ");
        }
      if (op_data->right)
        _ast_print_node (op_data->right);
      printf (")");
      break;
    default:
      strat = _ast_get_strategy (n->type);
      if (strat)
        strat->print (n);
      break;
    }
}

static int
_is_token_op (int tok)
{
  return tok == '+' || tok == '-' || tok == '*' || tok == '/' || tok == '!'
         || tok == '>' || tok == '<' || tok == TOKEN_GEQ || tok == TOKEN_LEQ
         || tok == TOKEN_NEQ;
}

static int
_is_expression_terminator (int tok)
{
  return tok == ',' || tok == ')' || tok == ';' || tok == TOKEN_END;
}

static inline ast_node *
_reverse_ast_list (ast_node *head)
{
  ast_node *prev = NULL, *curr = head, *next;
  while (curr)
    {
      next = curr->next;
      curr->next = prev;
      prev = curr;
      curr = next;
    }
  return prev;
}

const ast_strategy *
_ast_get_strategy (enum ast_type type)
{
  if (type >= 0 && type < AST_STRATEGY_COUNT && strategy_registry[type])
    {
      return strategy_registry[type];
    }
  printf ("[WARN] Unknown strategy %d\n", type);
  return NULL;
}

// vim:fdm=marker:
