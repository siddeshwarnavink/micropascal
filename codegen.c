#include "codegen.h"

void _ident_prefix (cg *ctx);
void _parse_exp (cg *ctx, ast_node *ptr);
void _codegen_cc_parse (cg *ctx, ast_node *ptr);
void _codegen_cc (cg *ctx, ast_node *ptr);

string *
codegen (cg *ctx, ast_node *root)
{
  dainit (&ctx->var_declares, &ctx->ar, 32, sizeof (ast_node *));
  sbinit (&ctx->sb, &ctx->ar);
  _codegen_cc (ctx, root);
  return sbflush (&ctx->sb);
}

void
codegen_fold (cg *ctx)
{
  dafold (&ctx->var_declares);
  arfold (&ctx->ar);
}

void
_ident_prefix (cg *ctx)
{
  sbappend (&ctx->sb, "_P");
}

void
_parse_exp (cg *ctx, ast_node *ptr)
{
  char buf[32];
  ast_data_op *op_data;

  switch (ptr->type)
    {
    case AST_VAR_DECLARE:
      _ident_prefix (ctx);
      sbappend (&ctx->sb, ((ast_data_var_declare *)ptr->data)->name->data);
      break;
    case AST_STRLIT:
      sbappendch (&ctx->sb, '"');
      sbappend (&ctx->sb, ((string *)ptr->data)->data);
      sbappendch (&ctx->sb, '"');
      break;
    case AST_INTLIT:
      sprintf (buf, "%ld", *((long *)ptr->data));
      sbappend (&ctx->sb, buf);
      break;
    case AST_BOOL:
      sprintf (buf, "%d", *((unsigned short *)ptr->data));
      sbappend (&ctx->sb, buf);
      break;
    case AST_FLOATLIT:
      sprintf (buf, "%f", *((double *)ptr->data));
      sbappend (&ctx->sb, buf);
      break;
    case AST_OP:
      op_data = ptr->data;
      if (op_data->left)
        _parse_exp (ctx, op_data->left);
      sbappendch (&ctx->sb, op_data->op);
      if (op_data->right)
        _parse_exp (ctx, op_data->right);
      break;
    }
}

void
_codegen_cc_parse (cg *ctx, ast_node *ptr)
{
  ast_node *arg;
  ast_data_var_declare *var;
  ast_data_var_assign *va_data;
  ast_data_cond *cond_data;
  ast_data_block *blk_data;
  ast_data_while *while_data;
  char buf[32];
  int i;

  while (ptr)
    {
      switch (ptr->type)
        {
        case AST_WHILE:
          while_data = ptr->data;
          sbappend (&ctx->sb, "while(");
          _parse_exp (ctx, while_data->cond);
          sbappendch (&ctx->sb, ')');
          if (while_data->next)
            _codegen_cc_parse (ctx, while_data->next);
          break;
        case AST_COND:
          cond_data = ptr->data;
          sbappend (&ctx->sb, "if(");
          _parse_exp (ctx, cond_data->cond);
          sbappendch (&ctx->sb, ')');
          if (cond_data->yes)
            _codegen_cc_parse (ctx, cond_data->yes);
          if (cond_data->no)
            {
              sbappend (&ctx->sb, "else ");
              _codegen_cc_parse (ctx, cond_data->no);
            }
          break;
        case AST_BLOCK:
          blk_data = ptr->data;
          sbappend (&ctx->sb, "{\n");
          _codegen_cc_parse (ctx, blk_data->next);
          sbappend (&ctx->sb, "\n}");
          break;
        case AST_MAIN_BLOCK:
          blk_data = ptr->data;
          sbappend (&ctx->sb, "int main() {\n");

          for (i = ctx->var_declares.size - 1; i >= 0; --i)
            {
              arg = *(ast_node **)dageti (&ctx->var_declares, i);
              var = arg->data;

              switch (var->datatype)
                {
                case AST_INTLIT:
                  sbappend (&ctx->sb, "long");
                  break;
                case AST_FLOATLIT:
                  sbappend (&ctx->sb, "double");
                  break;
                case AST_STRLIT:
                  sbappend (&ctx->sb, "char");
                  break;
                case AST_BOOL:
                  sbappend (&ctx->sb, "unsigned short");
                  break;
                default:
                  continue;
                }

              sbappendch (&ctx->sb, ' ');
              _ident_prefix (ctx);
              sbappend (&ctx->sb, var->name->data);
              if (var->arsize > 0)
                {
                  sprintf (buf, "%ld", var->arsize);
                  sbappendch (&ctx->sb, '[');
                  sbappend (&ctx->sb, buf);
                  sbappendch (&ctx->sb, ']');
                }

              sbappend (&ctx->sb, ";\n");
            }

          _codegen_cc_parse (ctx, blk_data->next);
          sbappend (&ctx->sb, "return 0;\n");
          sbappend (&ctx->sb, "}\n");
          break;
        case AST_VAR_DECLARE:
          dapush (&ctx->var_declares, &ptr);
          break;
        case AST_VAR_ASSIGN:
          va_data = ptr->data;
          var = va_data->var->data;

          if (var->datatype == AST_STRLIT)
            {
              sbappend (&ctx->sb, "strcpy(");
              _ident_prefix (ctx);
              sbappend (&ctx->sb, var->name->data);
              sbappendch (&ctx->sb, ',');
              _parse_exp (ctx, va_data->value);
              sbappend (&ctx->sb, ");\n");
            }
          else
            {
              _ident_prefix (ctx);
              sbappend (&ctx->sb, var->name->data);
              sbappendch (&ctx->sb, '=');
              _parse_exp (ctx, va_data->value);
              sbappend (&ctx->sb, ";\n");
            }

          break;
        case AST_FUNCALL:
          _ident_prefix (ctx);
          sbappend (&ctx->sb, ((ast_data_funcall *)ptr->data)->name->data);
          sbappendch (&ctx->sb, '(');
          arg = ((ast_data_funcall *)ptr->data)->args_head;
          while (arg)
            {
              _parse_exp (ctx, arg);
              if (arg->next)
                sbappendch (&ctx->sb, ',');
              arg = arg->next;
            }
          sbappend (&ctx->sb, ");\n");
          break;
        }
      ptr = ptr->next;
    }
}

void
_codegen_cc (cg *ctx, ast_node *ptr)
{

  sbappend (&ctx->sb, "#include <stdio.h>\n");
  sbappend (&ctx->sb, "#include <string.h>\n");
  sbappend (&ctx->sb, "#include <stdarg.h>\n");
  sbappend (&ctx->sb, "void _Pwriteln(const char *format, ...) {\n");
  sbappend (&ctx->sb, "va_list args;\n");
  sbappend (&ctx->sb, "va_start(args, format);\n");
  sbappend (&ctx->sb, "vprintf(format, args);\n");
  sbappend (&ctx->sb, "printf(\"\\n\");\n");
  sbappend (&ctx->sb, "va_end(args);\n");
  sbappend (&ctx->sb, "}\n");
  _codegen_cc_parse (ctx, ptr);
}
