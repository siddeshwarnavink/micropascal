#ifndef LEXER_H
#define LEXER_H

#include "clomy.h"

#define LEX_HANDLE_OP(ctx, ch1, ch2, token)                                   \
  if ((ctx)->src->data[(ctx)->pos] == (ch1)                                   \
      && (ctx)->src->data[(ctx)->pos + 1] == (ch2))                           \
    {                                                                         \
      sbreset (&(ctx)->sb);                                                   \
      (ctx)->pos += 2;                                                        \
      return (token);                                                         \
    }

#define LEX_FLUSH_IDENTF()                                                    \
  do                                                                          \
    {                                                                         \
      if (ctx->sb.size > 0)                                                   \
        {                                                                     \
          ctx->str = sbflush (&ctx->sb);                                      \
          if (strcmp (ctx->str->data, "then") == 0)                           \
            return TOKEN_THEN;                                                \
          return TOKEN_IDENTF;                                                \
        }                                                                     \
      sbreset (&ctx->sb);                                                     \
    }                                                                         \
  while (0);

struct lex
{
  arena ar;
  stringbuilder sb;
  string *path;
  string *src;
  string *str;
  double float_num;
  long int_num;
  unsigned int line;
  unsigned int col;
  unsigned int pos;
};
typedef struct lex lex;

enum lex_token
{
  TOKEN_END = 256,
  TOKEN_IDENTF,
  TOKEN_STRLIT,
  TOKEN_FLOATLIT,
  TOKEN_INTLIT,
  TOKEN_INFEQ,
  TOKEN_GEQ,
  TOKEN_LEQ,
  TOKEN_NEQ,
  TOKEN_THEN
};

int lex_init (lex *ctx, char *path);

int lex_next_token (lex *ctx);

int lex_peek (lex *ctx);

void lex_print_token (lex *ctx, int tok);

void lex_error (lex *ctx, char *msg);

void lex_fold (lex *ctx);

#endif /* not LEXER_H */
