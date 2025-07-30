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

  sbinit (&ctx->sb, &ctx->ar);

  file = fopen (path, "r");
  if (!file)
    {
      fprintf (stderr, "Error: Failed to open source code.\n");
      return 1;
    }

  while ((ch = fgetc (file)) != EOF)
    sbappendch (&ctx->sb, ch);

  fclose (file);

  ctx->src_len = ctx->sb.size;
  ctx->src = sbflush (&ctx->sb);
  ctx->str_len = 0;
  ctx->pos = 0;

  return 0;
}

int
lex_next_token (lex *ctx)
{
  char *num;
  unsigned short rcomment = 0, rstr = 0, rint = 0, rfloat = 0;

  if (ctx->str_len > 0)
    {
      arfree (ctx->str);
      ctx->str_len = 0;
    }

  while (ctx->pos < ctx->src_len)
    {
      if (rcomment)
        {
          if (ctx->pos + 1 < ctx->src_len && ctx->src[ctx->pos] == '*'
              && ctx->src[ctx->pos + 1] == ')')
            {
              ++ctx->pos;
              rcomment = 0;
            }
          ++ctx->pos;
          continue;
        }

      if (rstr)
        {
          if (ctx->src[ctx->pos] != '\'')
            {
              sbappendch (&ctx->sb, ctx->src[ctx->pos++]);
              continue;
            }
          else
            {
              ctx->str_len = ctx->sb.size;
              ctx->str = sbflush (&ctx->sb);
              ++ctx->pos;
              rstr = 0;
              return TOKEN_STRLIT;
            }
        }

      if (rint)
        {
          if (_is_digit (ctx->src[ctx->pos]))
            {
              sbappendch (&ctx->sb, ctx->src[ctx->pos++]);
              continue;
            }
          else if (!rfloat && ctx->src[ctx->pos] == '.')
            {
              rfloat = 1;
              sbappendch (&ctx->sb, ctx->src[ctx->pos++]);
              continue;
            }
          else
            {
              num = sbflush (&ctx->sb);
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

      if (_is_whitespace (ctx->src[ctx->pos]))
        {
          ++ctx->pos;
          if (ctx->sb.size > 0)
            {
              ctx->str_len = ctx->sb.size;
              ctx->str = sbflush (&ctx->sb);
              return TOKEN_IDENTF;
            }
          sbreset (&ctx->sb);
        }
      else if (_is_symbol (ctx->src[ctx->pos]))
        {
          if (ctx->sb.size > 0)
            {
              ctx->str_len = ctx->sb.size;
              ctx->str = sbflush (&ctx->sb);
              return TOKEN_IDENTF;
            }

          sbreset (&ctx->sb);

          if (!rstr && ctx->src[ctx->pos] == '\'')
            {
              rstr = 1;
              ++ctx->pos;
              continue;
            }

          if (ctx->pos + 1 < ctx->src_len)
            {
              if (ctx->src[ctx->pos] == '(' && ctx->src[ctx->pos + 1] == '*')
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
          return ctx->src[ctx->pos++];
        }
      else
        {
          if (!rint && _is_digit (ctx->src[ctx->pos]))
            {
              rint = 1;
              sbreset (&ctx->sb);
              sbappendch (&ctx->sb, ctx->src[ctx->pos++]);
              continue;
            }

          sbappendch (&ctx->sb, ctx->src[ctx->pos++]);
        }
    }

  if (ctx->str_len > 0)
    {
      arfree (ctx->str);
      ctx->str_len = 0;
    }

  return TOKEN_END;
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
         || ch == ';' || ch == '\'';
}

static unsigned int
_is_digit (char ch)
{
  return ch >= '0' && ch <= '9';
}
