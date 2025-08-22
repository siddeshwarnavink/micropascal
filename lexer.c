#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"

/* Check if CH is a white space. */
static U8 _is_whitespace (char ch);

/* Check if CH is known symbol or operator. */
static U8 _is_symbol (char ch);

/* Check if CH is a digit. */
static U8 _is_digit (char ch);

int
lex_init (lex *ctx, char *path)
{
  FILE *file;
  char ch;

  ctx->line = 1;
  ctx->col = 1;

  sbinit (&ctx->sb, &ctx->ar);

  sbappend (&ctx->sb, path);
  ctx->path = sbflush (&ctx->sb);

  file = fopen (path, "r");
  if (!file)
    {
      fprintf (stderr, "Error: Failed to open source code.\n");
      return 1;
    }

  while ((ch = fgetc (file)) != EOF)
    sbappendch (&ctx->sb, ch);

  fclose (file);

  ctx->src = sbflush (&ctx->sb);
  ctx->pos = 0;

  return 0;
}

int
lex_next_token (lex *ctx)
{
  char *num;
  U8 flags = 0;

  /* Reset string buffer. */
  if (ctx->str && ctx->str->size > 0)
    {
      arfree (ctx->str->data);
      arfree (ctx->str);
    }

  while (ctx->pos < ctx->src->size)
    {
      ++ctx->col;

      /* Ignore block comment. */
      if (flags & READ_BLOCK_COMMENT)
        {
          if (ctx->src->data[ctx->pos] == '}')
            flags &= ~READ_BLOCK_COMMENT;
          ++ctx->pos;
          continue;
        }

      /* Reading String literal. */
      if (flags & READ_STR_LIT)
        {
          if (ctx->src->data[ctx->pos] != '\'')
            {
              sbappendch (&ctx->sb, ctx->src->data[ctx->pos++]);
              continue;
            }
          else
            {
              ctx->str = sbflush (&ctx->sb);
              ++ctx->pos;
              flags &= ~READ_STR_LIT;
              return TOKEN_STRLIT;
            }
        }

      /* Reading Integer literal. */
      if (flags & READ_INT_LIT)
        {
          if (_is_digit (ctx->src->data[ctx->pos]))
            {
              sbappendch (&ctx->sb, ctx->src->data[ctx->pos++]);
              continue;
            }
          else if (!(flags & READ_FLOAT_LIT)
                   && ctx->src->data[ctx->pos] == '.')
            {
              flags |= READ_FLOAT_LIT;
              sbappendch (&ctx->sb, ctx->src->data[ctx->pos++]);
              continue;
            }
          else
            {
              num = sbflush (&ctx->sb)->data;
              flags &= ~READ_INT_LIT;

              if (flags & READ_FLOAT_LIT)
                {
                  flags &= ~READ_FLOAT_LIT;
                  ctx->float_num = atof (num);
                  arfree (num);
                  return TOKEN_FLOATLIT;
                }

              ctx->int_num = atoi (num);
              arfree (num);
              return TOKEN_INTLIT;
            }
        }

      /* Skip whitespace. */
      if (_is_whitespace (ctx->src->data[ctx->pos]))
        {

          if (ctx->src->data[ctx->pos] == '\n')
            {
              ++ctx->line;
              ctx->col = 1;
            }

          ++ctx->pos;
          LEX_FLUSH_IDENTF ();
        }

      else if (_is_symbol (ctx->src->data[ctx->pos]))
        {
          LEX_FLUSH_IDENTF ();

          if (!(flags & READ_STR_LIT) && ctx->src->data[ctx->pos] == '\'')
            {
              flags |= READ_STR_LIT;
              ++ctx->pos;
              continue;
            }

          if (ctx->src->data[ctx->pos] == '{')
            {
              flags |= READ_BLOCK_COMMENT;
              continue;
            }

          if (ctx->pos + 1 < ctx->src->size)
            {
              LEX_HANDLE_OP (ctx, ':', '=', TOKEN_INFEQ);
              LEX_HANDLE_OP (ctx, '>', '=', TOKEN_GEQ);
              LEX_HANDLE_OP (ctx, '<', '=', TOKEN_LEQ);
              LEX_HANDLE_OP (ctx, '<', '>', TOKEN_NEQ);
            }

          sbreset (&ctx->sb);
          return ctx->src->data[ctx->pos++];
        }
      else
        {
          if (!(flags & READ_INT_LIT) && _is_digit (ctx->src->data[ctx->pos]))
            {
              flags |= READ_INT_LIT;
              sbreset (&ctx->sb);
              sbappendch (&ctx->sb, ctx->src->data[ctx->pos++]);
              continue;
            }

          sbappendch (&ctx->sb, ctx->src->data[ctx->pos++]);
        }
    }

  if (ctx->str->size > 0)
    arfree (ctx->str);

  return TOKEN_END;
}

int
lex_peek (lex *ctx)
{
  U32 saved_pos = ctx->pos;
  U32 saved_line = ctx->line;
  U32 saved_col = ctx->col;
  int next_token = lex_next_token (ctx);

  ctx->pos = saved_pos;
  ctx->line = saved_line;
  ctx->col = saved_col;

  return next_token;
}

void
lex_print_token (lex *ctx, int tok)
{
  printf ("token - ");
  switch (tok)
    {
    case TOKEN_THEN:
      printf ("TOKEN_THEN");
      break;
    case TOKEN_ELSE:
      printf ("TOKEN_ELSE");
      break;
    case TOKEN_END:
      printf ("TOKEN_END");
      break;
    case TOKEN_IDENTF:
      printf ("TOKEN_IDENTF");
      if (ctx->str)
        printf (" (%s)", ctx->str->data);
      break;
    case TOKEN_STRLIT:
      printf ("TOKEN_STRLIT");
      if (ctx->str)
        printf (" (%s)", ctx->str->data);
      break;
    case TOKEN_FLOATLIT:
      printf ("TOKEN_FLOATLIT");
      break;
    case TOKEN_INTLIT:
      printf ("TOKEN_INTLIT (%ld)", ctx->int_num);
      break;
    case TOKEN_INFEQ:
      printf ("TOKEN_INFEQ");
      break;
    case TOKEN_GEQ:
      printf ("TOKEN_GEQ");
      break;
    case TOKEN_LEQ:
      printf ("TOKEN_LEQ");
      break;
    case TOKEN_NEQ:
      printf ("TOKEN_NEQ");
      break;
    default:
      printf ("'%c'", (char)tok);
      break;
    }
  printf ("\n");
}

void
lex_error (lex *ctx, char *msg)
{
  string *output;
  char buf[32];

  sbreset (&ctx->sb);

  sbappend (&ctx->sb, ctx->path->data);
  sbappendch (&ctx->sb, ':');

  sprintf (buf, "%d", ctx->line);
  sbappend (&ctx->sb, buf);
  sbappendch (&ctx->sb, ':');

  sprintf (buf, "%d", ctx->col);
  sbappend (&ctx->sb, buf);
  sbappend (&ctx->sb, ": ");

  sbappend (&ctx->sb, msg);
  sbappendch (&ctx->sb, '\n');

  output = sbflush (&ctx->sb);

  fprintf (stderr, "%s", output->data);

  lex_fold (ctx);
}

void
lex_fold (lex *ctx)
{
  arfold (&ctx->ar);
}

static U8
_is_whitespace (char ch)
{
  return ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r';
}

static U8
_is_symbol (char ch)
{
  return ch == ',' || ch == ':' || ch == '=' || ch == '.' || ch == '('
         || ch == ')' || ch == '[' || ch == ']' || ch == '{' || ch == '}'
         || ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '%'
         || ch == ';' || ch == '\'' || ch == '!' || ch == '>' || ch == '<';
}

static U8
_is_digit (char ch)
{
  return ch >= '0' && ch <= '9';
}
