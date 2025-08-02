#include "codegen.h"

void _parse_exp (stringbuilder *sb, ast_node *ptr);
void _codegen_cc_parse (stringbuilder *sb, ast_node *ptr);
void _codegen_cc (stringbuilder *sb, ast_node *ptr);

string *
codegen (arena *ar, ast_node *root)
{
  stringbuilder sb = { 0 };
  ast_node *ptr = root;

  sbinit (&sb, ar);

  _codegen_cc (&sb, ptr);

  return sbflush (&sb);
}

void
_parse_exp (stringbuilder *sb, ast_node *ptr)
{
  char buf[32];
  switch (ptr->type)
    {
    case AST_STRLIT:
      sbappendch (sb, '"');
      sbappend (sb, ((string *)ptr->data)->data);
      sbappendch (sb, '"');
      break;
    case AST_INTLIT:
      sprintf (buf, "%ld", *((long *)ptr->data));
      sbappend (sb, buf);
      break;
    case AST_OP:
      _parse_exp (sb, ((ast_data_op *)ptr->data)->left);
      sbappendch (sb, ((ast_data_op *)ptr->data)->op);
      _parse_exp (sb, ((ast_data_op *)ptr->data)->right);
      break;
    }
}

void
_codegen_cc_parse (stringbuilder *sb, ast_node *ptr)
{
  ast_node *arg;

  while (ptr)
    {
      switch (ptr->type)
        {
        case AST_MAIN_BLOCK:
          sbappend (sb, "int main() {\n");
          _codegen_cc_parse (sb, (ast_node *)ptr->data);
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
              if (arg->next)
                sbappendch (sb, ',');
              arg = arg->next;
            }
          sbappend (sb, ");\n");
          break;
        }
      ptr = ptr->next;
    }
}

void
_codegen_cc (stringbuilder *sb, ast_node *ptr)
{

  sbappend (sb, "#include <stdio.h>\n");
  sbappend (sb, "#include <stdarg.h>\n");
  sbappend (sb, "void writeln(const char *format, ...) {\n");
  sbappend (sb, "va_list args;\n");
  sbappend (sb, "va_start(args, format);\n");
  sbappend (sb, "vprintf(format, args);\n");
  sbappend (sb, "printf(\"\\n\");\n");
  sbappend (sb, "va_end(args);\n");
  sbappend (sb, "}\n");
  _codegen_cc_parse (sb, ptr);
}
