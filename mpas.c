#include <stdio.h>

#define CLOMY_IMPLEMENTATION
#include "clomy.h"

#include "codegen.h"

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
  arena code_ar = { 0 };
  ast_node *root;
  string *code;
  FILE *f;

  if (lex_init (&lexer, path) == 1)
    return 1;

  /*
  int token;
  while ((token = lex_next_token (&lexer)) != TOKEN_END)
    {
      lex_print_token (&lexer, token);
    }
    */

  root = ast_parse (&tree, &lexer);
  if (!root)
    {
      ast_fold (&tree);
      lex_fold (&lexer);
      return 1;
    }

  code = codegen (&code_ar, root);

  f = fopen ("a.c", "w");
  fprintf (f, "%s", code->data);
  fclose (f);

  arfold (&code_ar);

  system ("cc a.c");

  ast_fold (&tree);
  lex_fold (&lexer);

  return 0;
}
