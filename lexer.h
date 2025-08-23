#ifndef LEXER_H
#define LEXER_H

#include "clomy.h"
#include "utils.h"

/* Lexer flags. */
#define READ_BLOCK_COMMENT (1 << 0)
#define READ_STR_LIT (1 << 1)
#define READ_INT_LIT (1 << 2)
#define READ_FLOAT_LIT (1 << 3)

/* Handle operators which takes 2 characters. */
#define LEX_HANDLE_OP(ctx, ch1, ch2, token)                                   \
  if ((ctx)->src->data[(ctx)->pos] == (ch1)                                   \
      && (ctx)->src->data[(ctx)->pos + 1] == (ch2))                           \
    {                                                                         \
      sbreset (&(ctx)->sb);                                                   \
      (ctx)->pos += 2;                                                        \
      return (token);                                                         \
    }

/* Return known keywords. */
#define LEX_FLUSH_IDENTF()                                                    \
  do                                                                          \
    {                                                                         \
      if (ctx->sb.size > 0)                                                   \
        {                                                                     \
          ctx->str = sbflush (&ctx->sb);                                      \
          if (streq (ctx->str->data, "program"))                            \
            return TOKEN_PROGRAM;                                                \
          else if (streq (ctx->str->data, "then"))                                 \
            return TOKEN_THEN;                                                \
          else if (streq (ctx->str->data, "else"))                            \
            return TOKEN_ELSE;                                                \
          else if (streq (ctx->str->data, "do"))                              \
            return TOKEN_DO;                                                  \
          else if (streq (ctx->str->data, "var"))                           \
            return TOKEN_VAR;                                               \
          else if (streq (ctx->str->data, "begin"))                           \
            return TOKEN_BEGIN;                                               \
          else if (streq (ctx->str->data, "end"))                             \
            return TOKEN_BLOCK_END;                                           \
          else if (streq (ctx->str->data, "while"))                           \
            return TOKEN_WHILE;                                               \
          else if (streq (ctx->str->data, "if"))                              \
            return TOKEN_IF;                                                  \
          return TOKEN_IDENTF;                                                \
        }                                                                     \
      sbreset (&ctx->sb);                                                     \
    }                                                                         \
  while (0);

typedef struct lex
{
  arena ar;
  stringbuilder sb;
  string *path;
  string *src;
  string *str;
  double float_num;
  long int_num;
  U32 line;
  U32 col;
  U32 pos;
} lex;

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
  TOKEN_PROGRAM,
  TOKEN_THEN,
  TOKEN_ELSE,
  TOKEN_DO,
  TOKEN_VAR,
  TOKEN_BEGIN,
  TOKEN_BLOCK_END,
  TOKEN_WHILE,
  TOKEN_IF
};

/* Initialize the lexer. */
int lex_init (lex *ctx, char *path);

/* Move the lexer forward and get next token. */
int lex_next_token (lex *ctx);

/* Peek the next token without moving forward. */
int lex_peek (lex *ctx);

/* Print token in Human-friendly way. */
void lex_print_token (lex *ctx, int tok);

/* Report error at current lexer position. */
void lex_error (lex *ctx, char *msg);

/* Cleanup the lexer. */
void lex_fold (lex *ctx);

#endif /* not LEXER_H */
