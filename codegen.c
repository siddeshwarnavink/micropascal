#include "codegen.h"

void _parse_exp (stringbuilder *sb, ast_node *ptr);
void _codegen_gcc_parse (stringbuilder *sb, ast_node *ptr);
void _codegen_gcc (stringbuilder *sb, ast_node *ptr);

string *
codegen (arena *ar, ast_node *root)
{
  stringbuilder sb = { 0 };
  ast_node *ptr = root;

  sbinit (&sb, ar);

  _codegen_gcc (&sb, ptr);

  return sbflush (&sb);
}

void
_parse_exp (stringbuilder *sb, ast_node *ptr)
{
  switch (ptr->type)
    {
    case AST_STRLIT:
      sbappendch (sb, '"');
      sbappend (sb, ((string *)ptr->data)->data);
      sbappendch (sb, '"');
      break;
    }
}

void
_codegen_gcc_parse (stringbuilder *sb, ast_node *ptr)
{
  ast_node *arg;

  while (ptr)
    {
      switch (ptr->type)
        {
        case AST_PROGNAME:
          sbappend (sb, "/* Program: ");
          sbappend (sb, ((string *)ptr->data)->data);
          sbappend (sb, "*/\n");
          break;

        case AST_MAIN_BLOCK:
          sbappend (sb, "int main() {\n");
          _codegen_gcc_parse (sb, (ast_node *)ptr->data);
          sbappend (sb, "return 0;\n");
          sbappend (sb, "}\n");
          break;

        case AST_FUNCALL:
          sbappend (sb, ((ast_data_funcall *)ptr->data)->name->data);
          sbappendch (sb, '(');
          arg = ((ast_data_funcall *)ptr->data)->args_head;
          while (arg)
            {
              _parse_exp (sb, arg);
              arg = arg->next;
            }
          sbappend (sb, ");\n");
          break;
        }
      ptr = ptr->next;
    }
}

void
_codegen_gcc (stringbuilder *sb, ast_node *ptr)
{

  sbappend (sb, "#include <stdio.h>\n");
  sbappend (sb, "void writeln(char *str){\n");
  sbappend (sb, "printf (\"%s\\n\", str);\n");
  sbappend (sb, "}\n");
  _codegen_gcc_parse (sb, ptr);
}
