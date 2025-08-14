#include <stdio.h>

#define CLOMY_IMPLEMENTATION
#include "clomy.h"

#include "codegen.h"
#include "ir.h"

int compiler_main (char *path, unsigned short debug, unsigned short target);

void usage (char *prog);

int
main (int argc, char **argv)
{
  int i;
  unsigned short rtarget = 0, target = TARGET_C, debug = 0;

  if (argc > 1)
    {
      for (i = 0; i < argc; ++i)
        {
          if (rtarget)
            {
              if (strcmp (argv[i], "ast") == 0)
                {
                  target = TARGET_AST;
                }
              else if (strcmp (argv[i], "ir") == 0)
                {
                  target = TARGET_IR;
                }
              else if (strcmp (argv[i], "c") == 0)
                {
                  target = TARGET_C;
                }
              else
                {
                  printf ("Error: Unknown target \"%s\".\n", argv[i]);
                  usage (argv[0]);
                  return 1;
                }
              rtarget = 0;
            }
          else if (argv[i][0] == '-')
            {
              switch (argv[i][1])
                {
                case 't':
                  rtarget = 1;
                  break;
                case 'd':
                  debug = 1;
                  break;
                default:
                  printf ("Error: Unknown flag \"-%c\".\n", argv[i][1]);
                  usage (argv[0]);
                  return 1;
                }
            }
        }

      return compiler_main (argv[1], debug, target);
    }
  else
    {
      fprintf (stderr, "Error: No FILE path provided.\n");
      usage (argv[0]);
      return 1;
    }

  return 0;
}

int
compiler_main (char *path, unsigned short debug, unsigned short target)
{
  lex lexer = { 0 };
  ast tree = { 0 };
  cg cgctx = { 0 };
  ast_node *root;
  ir tac = { 0 };
  string *code;
  FILE *f;

  if (lex_init (&lexer, path) == 1)
    return 1;

  root = ast_parse (&tree, &lexer, debug);
  if (!root)
    {
      ast_fold (&tree);
      lex_fold (&lexer);
      return 1;
    }

  if (target == TARGET_AST)
    {
      ast_print_tree (tree.root, "\n");
      printf ("\n;; vi: ft=lisp\n");
    }
  else if (target == TARGET_IR)
    {
      ir_init (&tac, tree.root);
      ir_print (&tac);
      ir_fold (&tac);
    }
  else
    {
      code = codegen (&cgctx, root);

      f = fopen ("a.c", "w");
      fprintf (f, "%s", code->data);
      fclose (f);

      system ("cc a.c");
      codegen_fold (&cgctx);
    }
  ast_fold (&tree);
  lex_fold (&lexer);

  return 0;
}

void
usage (char *prog)
{
  fprintf (stderr, "Usage: %s [FILE] [FLAGS]\n", prog);
  fprintf (stderr, "    -t     target (ast, ir, c)\n");
  fprintf (stderr, "    -d     show debug\n");
}
