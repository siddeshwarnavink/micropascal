#include <stdio.h>

#define CLOMY_IMPLEMENTATION
#include "clomy.h"

#include "lexer.h"

int
main (int argc, char **argv)
{
  lex lexer = { 0 };
  int token;

  if (argc > 1)
    {
      if (lex_init (&lexer, argv[1]) == 1)
        return 1;

      while ((token = lex_next_token (&lexer)) != TOKEN_END)
        {
          switch (token)
            {
            case TOKEN_IDENTF:
              printf ("TOKEN_IDENTF: %s\n", lexer.str);
              break;
            case TOKEN_STRLIT:
              printf ("TOKEN_STRLIT: \"%s\"\n", lexer.str);
              break;
            case TOKEN_FLOATLIT:
              printf ("TOKEN_FLOATLIT: %.2f\n", lexer.float_num);
              break;
            case TOKEN_INTLIT:
              printf ("TOKEN_INTLIT: %ld\n", lexer.int_num);
              break;
            default:
              printf ("%c\n", token);
              break;
            }
        }

      lex_fold (&lexer);
    }
  else
    {
      fprintf (stderr, "Error: No source code provided.\n");
      fprintf (stderr, "Usage: %s [FILE]\n", argv[0]);
      return 1;
    }

  return 0;
}
