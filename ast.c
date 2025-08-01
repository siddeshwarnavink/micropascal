#include "ast.h"

static int _get_precedence (char op);
static ast_node *_ast_new_node (ast *ctx, short type);
static ast_node *_create_op_node (ast *ctx, char op, da *value_stk);
static void _ast_print_node (ast_node *n);
static int _is_token_op (int tok);

void
ast_parse (ast *ctx, lex *lexer)
{
  ctx->root = ast_parse_expression (ctx, lexer);
  _ast_print_node (ctx->root);
  arfold (&ctx->ar);
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

  while ((token = lex_next_token (lexer)) != TOKEN_END)
    {
      if (token == TOKEN_INTLIT)
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
    }

  while (op_stk.size > 0)
    {
      top_op = *(char *)dapop (&op_stk);
      node = _create_op_node (ctx, top_op, &value_stk);
      dapush (&value_stk, node);
    }

  if (value_stk.size > 0)
    return dageti (&value_stk, 0);

  return (void *)0;
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
  ast_data_op *op_data;

  switch (n->type)
    {
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
    }
}

static int
_is_token_op (int tok)
{
  return tok == '+' || tok == '-' || tok == '*' || tok == '/';
}
