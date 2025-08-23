#include "ast.h"
#include "utils.h"

static int _get_precedence (char op);
static ast_node *_ast_new_node (ast *ctx, short type);
static ast_node *_create_op_node (ast *ctx, char op, da *value_stk);
static void _ast_print_datatype (U16 dtype);
static void _ast_print_node (ast_node *n);
static int _is_token_op (int tok);
static int _is_expression_terminator (int tok);

static inline ast_node *
reverse_ast_list (ast_node *head)
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

/* TODO: We need to hava some sort of stack-based approach
   for handling loop to support nested loop. */
ast_node *
ast_parse (ast *ctx, lex *lexer, U16 debug)
{
  da block_stk = { 0 }, cond_stk = { 0 };
  ast_node *new, *parent, *blk, *cond, *exp, *var, *loop = (void *)0;
  string *str_data, *str_data2;
  void *ptr;
  U32 i;
  int token;
  U8 flags = 0;

  ast_data_var_declare *var_declare_data;
  ast_data_var_assign *var_assign_data;
  ast_data_funcall *funcall_data;
  ast_data_cond *cond_data;
  ast_data_while *while_data;
  ast_data_block *block_data;

  ctx->debug = debug;
  ctx->root = (void *)0;

  dainit (&block_stk, &ctx->ar, 16, sizeof (ast_node *));
  dainit (&cond_stk, &ctx->ar, 16, sizeof (ast_node *));
  htinit (&ctx->ident_table, &ctx->ar, 16, sizeof (ast_node *));

  /* Program name. */
  token = lex_next_token (lexer);
  AST_ERROR_IF (token != TOKEN_PROGRAM,
                "Expected \"program\" statement at beginning of program.");

  token = lex_next_token (lexer);
  AST_ERROR_IF (token != TOKEN_IDENTF, "Expected program name.");

  token = lex_next_token (lexer);
  AST_EXPECT_SEMICOLON ();

  new = _ast_new_node (ctx, AST_PROGNAME);
  str_data = stringcpy (&ctx->ar, lexer->str);
  new->data = str_data;
  ctx->root = new;
  new = (void *)0;

  while ((token = lex_next_token (lexer)) != TOKEN_END)
    {
      if (cond_stk.size > 0)
        {
          // {{{ Handle end of condition.
          cond_data = (*(ast_node **)dageti (&cond_stk, 0))->data;
          AST_ERROR_IF (
              !cond_data,
              "Unreachable. Condition stack shouldn't have null pointer.");

          /* Remove IF if it is completed. */
          if (cond_data->yes && cond_data->no
              && cond_data->no != (void *)0xDEADBEEF)
            {
              AST_LOG ("Getting rid of IF (else found).");
              if (ctx->debug)
                lex_print_token (lexer, token);
              dadel (&cond_stk, 0);
            }
          // }}}
        }
      if (token == TOKEN_ELSE)
        {
          // {{{ Handle ELSE
          if (cond_stk.size == 0)
            AST_ERROR_IF (1, "Invalid usage of \"else\".");

          cond = *(ast_node **)dageti (&cond_stk, 0);
          cond_data = cond->data;
          cond_data->no = (void *)0xDEADBEEF;

          if (!cond_data->yes)
            AST_ERROR_IF (1, "Invalid usage of \"else\".");

          AST_LOG ("Found ELSE for IF.");
          if (ctx->debug)
            ast_print_tree (cond, " ");

          continue;
          // }}}
        }
      if (token == TOKEN_VAR)
        {
          // {{{ Handle VAR
          AST_ERROR_IF (block_stk.size > 0,
                        "Variable declaration not allowed inside block.");
          flags |= READ_VAR;
          // }}}
        }
      else if (token == TOKEN_BEGIN)
        {
          // {{{ Handle block BEGIN.
          flags &= ~READ_VAR;
          new = _ast_new_node (ctx, AST_BLOCK);
          AST_LOG ("Block %p begin.", new);
          block_data = aralloc (&ctx->ar, sizeof (ast_data_block));
          new->data = block_data;

          AST_APPEND_TO_BLOCK ();
          block_data->parent = parent;
          dapush (&block_stk, &new);
          continue;
          // }}}
        }
      else if (token == TOKEN_BLOCK_END)
        {
          // {{{ Handle block END.
          AST_ERROR_IF (block_stk.size == 0, "Invalid use of \"end\".");
          blk = *(ast_node **)dageti (&block_stk, 0);
          AST_LOG ("End of Block %p.", blk);
          block_data = blk->data;
          block_data->next = reverse_ast_list (block_data->next);

          /* Check what's up with the parent. */
          if (block_data->parent)
            {
              AST_LOG ("End Block parent.");
              if (ctx->debug)
                ast_print_tree (block_data->parent, " ");

              if (block_data->parent->type == AST_COND)
                {
                  cond_data = block_data->parent->data;
                  if (cond_data->yes && !cond_data->no
                      && lex_peek (lexer) != TOKEN_ELSE)
                    {
                      AST_LOG ("Getting rid of IF (no else found).");
                      if (ctx->debug)
                        lex_print_token (lexer, token);
                      dadel (&cond_stk, 0);
                    }
                }
            }

          /* Remove all child conditions of the block. */
          new = block_data->next;
          while (new)
            {
              if (new->type == AST_COND)
                {
                  for (i = 0; i < cond_stk.size; ++i)
                    {
                      cond = *(ast_node **)dageti (&cond_stk, i);
                      if (cond == new)
                        {
                          AST_LOG ("Remove child condition from stack.");
                          if (ctx->debug)
                            ast_print_tree (cond, " ");
                          dadel (&cond_stk, i);
                        }
                    }
                }
              new = new->next;
            }

          dadel (&block_stk, 0);
          // }}}
        }
      else if ((flags & READ_VAR) && token == TOKEN_IDENTF)
        {
          // {{{ Handle variable(s) declaration(s).
          str_data = stringcpy (&ctx->ar, lexer->str);

          token = lex_next_token (lexer);
          AST_ERROR_IF (token != ':', "Expected ':'");

          token = lex_next_token (lexer);
          AST_ERROR_IF (token != TOKEN_IDENTF,
                        "Expected data type of variable.");
          str_data2 = stringcpy (&ctx->ar, lexer->str);

          new = _ast_new_node (ctx, AST_VAR_DECLARE);
          var_declare_data = aralloc (&ctx->ar, sizeof (ast_data_var_declare));
          var_declare_data->name = str_data;
          new->data = var_declare_data;

          if (streq (str_data2->data, "integer"))
            var_declare_data->datatype = AST_INTLIT;
          else if (streq (str_data2->data, "real"))
            var_declare_data->datatype = AST_FLOATLIT;
          else if (streq (str_data2->data, "string"))
            var_declare_data->datatype = AST_STRLIT;
          else if (streq (str_data2->data, "boolean"))
            var_declare_data->datatype = AST_BOOL;
          else
            AST_ERROR_IF (true, "Unknown datatype.");

          token = lex_next_token (lexer);
          if (token == '[')
            {
              token = lex_next_token (lexer);
              AST_ERROR_IF (token != TOKEN_INTLIT,
                            "Expected integer for array size.");
              var_declare_data->arsize = lexer->int_num;

              token = lex_next_token (lexer);
              AST_ERROR_IF (token != ']', "Expected ']'");

              token = lex_next_token (lexer);
            }

          AST_EXPECT_SEMICOLON ();

          stput (&ctx->ident_table, var_declare_data->name->data, &new);

          if (strcmp (str_data2->data, "string") == 0
              && var_declare_data->arsize < 1)
            {
              var_declare_data->arsize = 256;
            }

          new->next = ctx->root;
          ctx->root = new;
          // }}}
        }
      else if (token == TOKEN_WHILE)
        {
          // Handle WHILE {{{
          AST_ERROR_IF (block_stk.size < 1,
                        "WHILE should be used inside a block.");
          new = _ast_new_node (ctx, AST_WHILE);
          while_data = aralloc (&ctx->ar, sizeof (ast_data_while));
          exp = ast_parse_expression (ctx, lexer);
          if (exp)
            {
              while_data->cond = exp;
              token = lex_next_token (lexer);
              AST_ERROR_IF (token != TOKEN_DO, "Expected \"do\" after while.");
            }
          else
            {
              AST_ERROR_IF (true, "Expected condition expression.");
            }

          new->data = while_data;
          AST_APPEND_TO_BLOCK ();
          loop = new;
          continue;
          // }}}
        }
      else if (token == TOKEN_IF)
        {
          // Handle IF condition. {{{
          AST_ERROR_IF (block_stk.size < 1,
                        "IF should be used inside a block.");
          new = _ast_new_node (ctx, AST_COND);
          cond_data = aralloc (&ctx->ar, sizeof (ast_data_cond));
          exp = ast_parse_expression (ctx, lexer);
          if (exp)
            {
              cond_data->cond = exp;
              token = lex_next_token (lexer);
              AST_ERROR_IF (token != TOKEN_THEN,
                            "Expected \"then\" after condition.");
            }
          else
            {
              AST_ERROR_IF (true,
                            "Expected condition expression after \"if\".");
            }

          new->data = cond_data;
          AST_APPEND_TO_BLOCK ();
          dapush (&cond_stk, &new);
          continue;
          // }}}
        }
      else if (block_stk.size > 0 && token == TOKEN_IDENTF)
        {
          str_data = stringcpy (&ctx->ar, lexer->str);
          token = lex_next_token (lexer);

          if (token == '(')
            {
              // Handle Function call {{{
              new = _ast_new_node (ctx, AST_FUNCALL);
              funcall_data = aralloc (&ctx->ar, sizeof (ast_data_funcall));
              funcall_data->name = str_data;
              funcall_data->args_head = (void *)0;
              new->data = funcall_data;

              token = lex_peek (lexer);
              while (!_is_expression_terminator (token))
                {
                  exp = ast_parse_expression (ctx, lexer);
                  if (exp)
                    {
                      exp->next = funcall_data->args_head;
                      funcall_data->args_head = exp;
                    }
                  else
                    {
                      goto ast_err_exit;
                    }

                  token = lex_peek (lexer);
                  if (token == ',')
                    {
                      lex_next_token (lexer);
                      token = lex_peek (lexer);
                    }
                }

              if (token == ')')
                {
                  lex_next_token (lexer);
                  token = lex_peek (lexer);
                }

              if (!(cond_stk.size > 0 && token == TOKEN_ELSE))
                AST_EXPECT_SEMICOLON ();

              funcall_data->args_head
                  = reverse_ast_list (funcall_data->args_head);
              AST_APPEND_TO_BLOCK ();
              // }}}
            }
          else if ((ptr = stget (&ctx->ident_table, str_data->data))
                   != (void *)0)
            {
              // Handle Variable assignment. {{{
              var = *((ast_node **)ptr);

              AST_ERROR_IF (var->type != AST_VAR_DECLARE,
                            "Expected identifier variable.");
              AST_ERROR_IF (token != TOKEN_INFEQ, "Expected ':='");

              new = _ast_new_node (ctx, AST_VAR_ASSIGN);
              var_assign_data
                  = aralloc (&ctx->ar, sizeof (ast_data_var_assign));
              var_assign_data->var = var;

              exp = ast_parse_expression (ctx, lexer);
              if (exp)
                {
                  token = lex_next_token (lexer);
                  AST_EXPECT_SEMICOLON ();

                  /* TODO: Report error if datatype mismatch. */

                  var_assign_data->value = exp;
                  new->data = var_assign_data;
                  AST_APPEND_TO_BLOCK ();
                }
              else
                {
                  goto ast_err_exit;
                }
              // }}}
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
              if (!(flags & FOUND_ENTRY) && ctx->root->type == AST_BLOCK)
                {
                  flags |= FOUND_ENTRY;
                  ctx->root->type = AST_MAIN_BLOCK;
                }
              break;
            }
        }
    }

  AST_ERROR_IF (!flags & FOUND_ENTRY, "Cannot find entry point.");

  if (ctx->root)
    ctx->root = reverse_ast_list (ctx->root);

  dafold (&block_stk);
  dafold (&cond_stk);
  stfold (&ctx->ident_table);

  /* TODO: Report unclosed blocks. */

  return ctx->root;

ast_err_exit:
  return (void *)0;
}

void *
ast_parse_expression (ast *ctx, lex *lexer)
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
          if ((ptr = stget (&ctx->ident_table, lexer->str->data)) != (void *)0)
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
ast_fold (ast *ctx)
{
  arfold (&ctx->ar);
}

static int
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
  string *str_data;
  ast_data_op *op_data;
  ast_data_funcall *funcall_data;
  ast_data_var_declare *var_declare_data;
  ast_data_var_assign *var_assign_data;
  ast_data_cond *cond_data;
  ast_data_block *block_data;
  ast_data_while *while_data;

  switch (n->type)
    {
    case AST_PROGNAME:
      printf ("(program %s)", ((string *)n->data)->data);
      break;
    case AST_VAR_ASSIGN:
      var_assign_data = n->data;
      var_declare_data = var_assign_data->var->data;
      printf ("(var %s ", var_declare_data->name->data);
      _ast_print_node (var_assign_data->value);
      printf (")");
      break;
    case AST_VAR_DECLARE:
      var_declare_data = n->data;
      printf ("(var %s ", var_declare_data->name->data);
      _ast_print_datatype (var_declare_data->datatype);
      printf (")");
      break;
    case AST_MAIN_BLOCK:
      block_data = n->data;
      printf ("(main-block\n  ");
      ast_print_tree (block_data->next, "\n  ");
      printf (")");
      break;
    case AST_WHILE:
      while_data = n->data;
      printf ("(while ");
      _ast_print_node (while_data->cond);
      printf ("\n    ");
      ast_print_tree (while_data->next, "\n  ");
      printf (")");
      break;
    case AST_COND:
      cond_data = n->data;
      printf ("(if ");
      _ast_print_node (cond_data->cond);
      printf ("\n    ");
      if (cond_data->yes)
        ast_print_tree (cond_data->yes, "\n  ");
      if (cond_data->no)
        {
          printf ("\n    ");
          if (cond_data->no == (void *)0xDEADBEEF)
            printf ("0xDEADBEEF");
          else
            ast_print_tree (cond_data->no, "\n  ");
        }
      printf (")");
      break;
    case AST_BLOCK:
      block_data = n->data;
      printf ("(block\n  ");
      ast_print_tree (block_data->next, "\n  ");
      printf (")");
      break;
    case AST_FUNCALL:
      funcall_data = n->data;
      printf ("(%s ", funcall_data->name->data);
      ast_print_tree (funcall_data->args_head, " ");
      printf (")");
      break;
    case AST_STRLIT:
      str_data = n->data;
      printf ("\"%s\"", str_data->data);
      break;
    case AST_INTLIT:
      printf ("%ld", *(long *)n->data);
      break;
    case AST_BOOL:
      printf ("%s", *((U16 *)n->data) == 1 ? "true" : "false");
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
      printf ("%d?", n->type);
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
