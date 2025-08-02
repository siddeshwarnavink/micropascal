#include "ast.h"

static int _get_precedence (char op);
static ast_node *_ast_new_node (ast *ctx, short type);
static ast_node *_create_op_node (ast *ctx, char op, da *value_stk);
static void _ast_print_node (ast_node *n);
static void _ast_print_tree (ast_node *n, char *delim);
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

ast_node *
ast_parse (ast *ctx, lex *lexer)
{
  ast_node *new, *block = (ast_node *)0, *exp;
  string *str_data;
  ast_data_funcall *funcall_data;
  int token;
  unsigned short found_entry = 0;

  ctx->root = (void *)0;

  /* Program name. */
  token = lex_next_token (lexer);
  AST_EXPECT (token != TOKEN_IDENTF
                  || strcmp (lexer->str->data, "program") != 0,
              "Expected \"program\" statement.");

  token = lex_next_token (lexer);
  AST_EXPECT (token != TOKEN_IDENTF, "Expected program name.");

  token = lex_next_token (lexer);
  AST_EXPECT_SEMI ()

  new = _ast_new_node (ctx, AST_PROGNAME);
  str_data = stringcpy (&ctx->ar, lexer->str);
  new->data = str_data;
  ctx->root = new;
  new = (void *)0;

  while ((token = lex_next_token (lexer)) != TOKEN_END)
    {
      /* Handle block. */
      if (token == TOKEN_IDENTF && strcmp (lexer->str->data, "begin") == 0)
        {
          AST_EXPECT (block != (ast_node *)0, "Nested blocks not allowed.");
          block = _ast_new_node (ctx, AST_BLOCK);
          block->data = (void *)0;
        }

      /* Handle block end. */
      else if (token == TOKEN_IDENTF && strcmp (lexer->str->data, "end") == 0)
        {
          AST_EXPECT (block == (ast_node *)0, "Invalid use of end.");
          block->data = reverse_ast_list ((ast_node *)block->data);
          block->next = ctx->root;
          ctx->root = block;
          block = (void *)0;
        }
      else if (block && token == TOKEN_IDENTF)
        {
          str_data = stringcpy (&ctx->ar, lexer->str);
          token = lex_next_token (lexer);
          switch (token)
            {
            case '(':
              /* Function call */
              {
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

                AST_EXPECT_SEMI ();

                funcall_data->args_head
                    = reverse_ast_list (funcall_data->args_head);

                if (block)
                  {
                    new->next = block->data;
                    block->data = new;
                  }

                new = (void *)0;
              }
              break;
            }
        }
      else
        {
          switch (token)
            {
            /* Main block. */
            case '.':
              if (!found_entry && ctx->root->type == AST_BLOCK)
                {
                  found_entry = 1;
                  ctx->root->type = AST_MAIN_BLOCK;
                }
              break;
            }
        }
    }

  AST_EXPECT (!found_entry, "Cannot find entry point.");

  if (ctx->root)
    {
      ctx->root = reverse_ast_list (ctx->root);
      /* _ast_print_tree (ctx->root, "\n"); */
    }

ast_exit:
  return ctx->root;
}

void *
ast_parse_expression (ast *ctx, lex *lexer)
{
  da value_stk = { 0 }, op_stk = { 0 };
  ast_node *new, *node;
  long *int_data;
  int token, current_prec, top_prec;
  char current_op, top_op;

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
      else if (token == TOKEN_INTLIT)
        {
          new = _ast_new_node (ctx, AST_INTLIT);
          int_data = aralloc (&ctx->ar, sizeof (long));
          *int_data = lexer->int_num;
          new->data = int_data;
          dapush (&value_stk, new);
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
  op_data->left = dapop (value_stk);
  new_node->data = op_data;

  return new_node;
}

static void
_ast_print_node (ast_node *n)
{
  string *str_data;
  ast_data_op *op_data;
  ast_data_funcall *funcall_data;

  switch (n->type)
    {
    case AST_PROGNAME:
      printf ("(program %s)", ((string *)n->data)->data);
      break;
    case AST_MAIN_BLOCK:
      printf ("(main-block\n  ");
      _ast_print_tree ((ast_node *)n->data, "\n  ");
      printf (")");
      break;
    case AST_BLOCK:
      printf ("(block\n  ");
      _ast_print_tree ((ast_node *)n->data, "\n  ");
      printf (")");
      break;
    case AST_FUNCALL:
      funcall_data = n->data;
      printf ("(%s ", funcall_data->name->data);
      _ast_print_tree (funcall_data->args_head, " ");
      printf (")");
      break;
    case AST_STRLIT:
      str_data = n->data;
      printf ("\"%s\"", str_data->data);
      break;
    case AST_INTLIT:
      printf ("%ld", *(long *)n->data);
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

static void
_ast_print_tree (ast_node *root, char *delim)
{
  while (root)
    {
      _ast_print_node (root);
      if (root->next)
        printf ("%s", delim);

      root = root->next;
    }
}

static int
_is_token_op (int tok)
{
  return tok == '+' || tok == '-' || tok == '*' || tok == '/';
}

static int
_is_expression_terminator (int tok)
{
  return tok == ',' || tok == ')' || tok == ';' || tok == TOKEN_END;
}
