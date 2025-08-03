#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"

static unsigned int _is_whitespace (char ch);

static unsigned int _is_symbol (char ch);

static unsigned int _is_digit (char ch);

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
  unsigned short rcomment = 0, rstr = 0, rint = 0, rfloat = 0;

  if (ctx->str && ctx->str->size > 0)
    {
      arfree (ctx->str->data);
      arfree (ctx->str);
    }

  while (ctx->pos < ctx->src->size)
    {
      ++ctx->col;

      if (rcomment)
        {
          if (ctx->pos + 1 < ctx->src->size
              && ctx->src->data[ctx->src->size] == '*'
              && ctx->src->data[ctx->pos + 1] == ')')
            {
              ++ctx->pos;
              rcomment = 0;
            }
          ++ctx->pos;
          continue;
        }

      if (rstr)
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
              rstr = 0;
              return TOKEN_STRLIT;
            }
        }

      if (rint)
        {
          if (_is_digit (ctx->src->data[ctx->pos]))
            {
              sbappendch (&ctx->sb, ctx->src->data[ctx->pos++]);
              continue;
            }
          else if (!rfloat && ctx->src->data[ctx->pos] == '.')
            {
              rfloat = 1;
              sbappendch (&ctx->sb, ctx->src->data[ctx->pos++]);
              continue;
            }
          else
            {
              num = sbflush (&ctx->sb)->data;
              rint = 0;

              if (rfloat)
                {
                  rfloat = 0;
                  ctx->float_num = atof (num);
                  arfree (num);
                  return TOKEN_FLOATLIT;
                }

              ctx->int_num = atoi (num);
              arfree (num);
              return TOKEN_INTLIT;
            }
        }

      if (_is_whitespace (ctx->src->data[ctx->pos]))
        {

          if (ctx->src->data[ctx->pos] == '\n')
            {
              ++ctx->line;
              ctx->col = 1;
            }

          ++ctx->pos;
          if (ctx->sb.size > 0)
            {
              ctx->str = sbflush (&ctx->sb);
              return TOKEN_IDENTF;
            }
          sbreset (&ctx->sb);
        }
      else if (_is_symbol (ctx->src->data[ctx->pos]))
        {
          if (ctx->sb.size > 0)
            {
              ctx->str = sbflush (&ctx->sb);
              return TOKEN_IDENTF;
            }

          sbreset (&ctx->sb);

          if (!rstr && ctx->src->data[ctx->pos] == '\'')
            {
              rstr = 1;
              ++ctx->pos;
              continue;
            }

          if (ctx->pos + 1 < ctx->src->size)
            {
              if (ctx->src->data[ctx->pos] == '('
                  && ctx->src->data[ctx->pos + 1] == '*')
                {
                  rcomment = 1;
                  continue;
                }

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
          if (!rint && _is_digit (ctx->src->data[ctx->pos]))
            {
              rint = 1;
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
  unsigned int saved_pos = ctx->pos;
  unsigned int saved_line = ctx->line;
  unsigned int saved_col = ctx->col;
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

static unsigned int
_is_whitespace (char ch)
{
  return ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r';
}

static unsigned int
_is_symbol (char ch)
{
  return ch == ',' || ch == ':' || ch == '=' || ch == '.' || ch == '('
         || ch == ')' || ch == '[' || ch == ']' || ch == '{' || ch == '}'
         || ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '%'
         || ch == ';' || ch == '\'' || ch == '!';
}

static unsigned int
_is_digit (char ch)
{
  return ch >= '0' && ch <= '9';
}
