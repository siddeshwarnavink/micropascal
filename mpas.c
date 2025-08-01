#include <stdio.h>

#define CLOMY_IMPLEMENTATION
#include "clomy.h"

#include "ast.h"

int compiler_main (char *path);

int
main (int argc, char **argv)
{
  if (argc > 1)
    {
      return compiler_main (argv[1]);
    }
  else
    {
      fprintf (stderr, "Error: No source code provided.\n");
      fprintf (stderr, "Usage: %s [FILE]\n", argv[0]);
      return 1;
    }

  return 0;
}

int
compiler_main (char *path)
{
  lex lexer = { 0 };
  ast tree = { 0 };

  if (lex_init (&lexer, path) == 1)
    return 1;

  ast_parse (&tree, &lexer);

  lex_fold (&lexer);

  return 0;
}
