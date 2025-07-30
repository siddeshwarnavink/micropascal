#ifndef LEXER_H
#define LEXER_H

#include "clomy.h"

#define LEX_HANDLE_OP(ctx, ch1, ch2, token)                                   \
  if ((ctx)->src[(ctx)->pos] == (ch1) && (ctx)->src[(ctx)->pos + 1] == (ch2)) \
    {                                                                         \
      sbreset (&(ctx)->sb);                                                   \
      (ctx)->pos += 2;                                                        \
      return (token);                                                         \
    }

struct lex
{
  arena ar;
  stringbuilder sb;
  char *src; /* Source code. */
  unsigned int src_len;
  char *str;
  unsigned int str_len;
  unsigned int pos;
  double float_num;
  long int_num;
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
};

int lex_init (lex *ctx, char *path);

int lex_next_token (lex *ctx);

void lex_fold (lex *ctx);

#endif /* not LEXER_H */
