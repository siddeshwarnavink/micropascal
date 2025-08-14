#include "clomy_test.h"
#include "ir.h"

static unsigned short _ir_expr_datatype (ir *ctx, ir_operand *itm);
static void _ir_print_vartable (ir *ctx);
static void _ir_parse (ir *ctx, ast_node *root);
static unsigned short _is_assign (ir_tac *n);
static void _ir_eval_optimize (ir *ctx);
static ir_tac *_new_tac (ir *ctx, unsigned short op);
static ir_operand *_new_op (ir *ctx, unsigned int type);
static void _ir_var_assign (ir *ctx, ast_node *n);
static void _ir_func_call (ir *ctx, ast_node *n);
static ir_tac *_ir_block (ir *ctx, ast_node *n, unsigned short depth);
static void _ir_cond (ir *ctx, ast_node *n);
static ir_operand *_to_ir_type (ir *ctx, ast_node *n);
static unsigned short _op_ast_to_ir (ast_node *n);
static ir_operand *_temp_var (ir *ctx, ast_node *n);
static void _print_exp (ir_operand *op);
static void _print_op (ir_tac *itm);
static char *_ir_datatype (ir_operand *op);

void
ir_init (ir *ctx, ast_node *root, unsigned short debug)
{
  ctx->debug = debug;
  dainit (&ctx->ops, &ctx->ar, sizeof (ir_tac *), 64);
  htinit (&ctx->var_table, &ctx->ar, 16, sizeof (ir_tac *));
  sbinit (&ctx->sb, &ctx->ar);

  _ir_parse (ctx, root);
  if (debug)
    {
      printf (";; IR before optimization.\n");
      _ir_print_vartable (ctx);
      ir_print (ctx);
    }

  _ir_eval_optimize (ctx);

  if (debug)
    _ir_print_vartable (ctx);
}

void
_ir_eval_optimize (ir *ctx)
{
  ir_vartable_item *vt_itm;
  ir_tac *ptr;
  unsigned int i;

  /* Compile Time Evaluation. */
  for (i = 0; i < ctx->ops.size; ++i)
    {
      ptr = *(ir_tac **)dageti (&ctx->ops, i);

      if (_is_assign (ptr))
        {
          /* Checking for value of B is already known. */
          IR_CHECK_ALREADY_KNOW (ptr->b);

          /* Checking for value of C is already known. */
          IR_CHECK_ALREADY_KNOW (ptr->c);

          /* Static value. */
          if (ptr->op == IR_ASSIGN)
            {
              vt_itm = *(ir_vartable_item **)stget (&ctx->var_table,
                                                    ptr->a->str_data);
              vt_itm->static_value = 1;

              switch (vt_itm->var->b->type)
                {
                case IR_OP_CONST_STR:
                  vt_itm->str_data = ptr->b->str_data;
                  break;
                case IR_OP_CONST_FLOAT:
                  vt_itm->float_data = ptr->b->float_data;
                  break;
                case IR_OP_CONST_INT:
                  vt_itm->int_data = ptr->b->int_data;
                  break;
                case IR_OP_CONST_BOOL:
                  vt_itm->bool_data = ptr->b->bool_data;
                  break;
                default:
                  CLOMY_FAIL (
                      "Unreachable. This error should've been caught by AST.");
                  break;
                }
            }
          /* Static boolean NOT expression. */
          else if (ptr->op == IR_ASSIGN_NOT
                   && ptr->c->type == IR_OP_CONST_BOOL)
            {
              vt_itm = *(ir_vartable_item **)stget (&ctx->var_table,
                                                    ptr->a->str_data);
              vt_itm->static_value = 1;

              ptr->b = _new_op (ctx, IR_OP_CONST_BOOL);
              ptr->b->bool_data = ptr->c->bool_data == 1 ? 0 : 1;
              arfree (ptr->c);
              ptr->c = (void *)0;

              ptr->op = IR_ASSIGN;
              vt_itm->bool_data = ptr->b->bool_data;
            }
          /* Static arithmetic expression. */
          else if (ptr->b && ptr->b->type != IR_OP_VAR && ptr->c
                   && ptr->c->type != IR_OP_VAR)
            {
              vt_itm = *(ir_vartable_item **)stget (&ctx->var_table,
                                                    ptr->a->str_data);
              vt_itm->static_value = 1;

              switch (vt_itm->var->b->type)
                {
                case IR_OP_CONST_INT:
                  IR_PERFORM_OP (vt_itm->int_data, ptr->b->int_data,
                                 ptr->c->int_data);
                  ptr->b = _new_op (ctx, IR_OP_CONST_INT);
                  ptr->b->int_data = vt_itm->int_data;
                  break;
                case IR_OP_CONST_FLOAT:
                  IR_PERFORM_OP (vt_itm->float_data, ptr->b->float_data,
                                 ptr->c->float_data);
                  ptr->b = _new_op (ctx, IR_OP_CONST_FLOAT);
                  ptr->b->float_data = vt_itm->float_data;
                  break;
                default:
                  CLOMY_FAIL ("Unreachable. Unknown static expression.");
                  break;
                }

              ptr->op = IR_ASSIGN;
              arfree (ptr->c);
              ptr->c = (void *)0;
            }
        }
    }

  /* Remove useless variables. */
  for (i = 0; i < ctx->ops.size; ++i)
    {
      ptr = *(ir_tac **)dageti (&ctx->ops, i);

      if (_is_assign (ptr))
        {
          vt_itm = *(ir_vartable_item **)stget (&ctx->var_table,
                                                ptr->a->str_data);
          /* Remove if unused. */
          if (vt_itm->usage < 1)
            {
              dadel (&ctx->ops, i--);
            }
          /* Remove assigning non-final values. */
          else if (ptr->op == IR_ASSIGN)
            {
              if (ptr->b->type == IR_OP_CONST_INT
                  && ptr->b->int_data != vt_itm->int_data)
                dadel (&ctx->ops, i--);
              else if (ptr->b->type == IR_OP_CONST_FLOAT
                       && ptr->b->float_data != vt_itm->float_data)
                dadel (&ctx->ops, i--);
              else if (ptr->b->type == IR_OP_CONST_BOOL
                       && ptr->b->bool_data != vt_itm->bool_data)
                dadel (&ctx->ops, i--);
              else if (ptr->b->type == IR_OP_CONST_STR
                       && strcmp (ptr->b->str_data, vt_itm->str_data) != 0)
                dadel (&ctx->ops, i--);
            }
        }
    }
}

void
_ir_parse (ir *ctx, ast_node *root)
{
  ast_node *ptr = root;
  ir_tac *new;
  ir_vartable_item *vt_itm;

  while (ptr)
    {
      switch (ptr->type)
        {
        case AST_PROGNAME:
          break;
        case AST_MAIN_BLOCK:
          new = _new_tac (ctx, IR_LABEL);
          new->a = _new_op (ctx, IR_OP_LABEL);
          sbappend (&ctx->sb, "_start");
          new->a->str_data = sbflush (&ctx->sb)->data;
          daappend (&ctx->ops, &new);
          _ir_parse (ctx, ((ast_data_block *)ptr->data)->next);
          break;
        case AST_VAR_DECLARE:
          new = _new_tac (ctx, IR_DECLARE);
          new->a = _new_op (ctx, IR_OP_VAR);
          new->a->str_data = ((ast_data_var_declare *)ptr->data)->name->data;

          switch (((ast_data_var_declare *)ptr->data)->datatype)
            {
            case AST_INTLIT:
              new->b = _new_op (ctx, IR_OP_CONST_INT);
              break;
            case AST_FLOATLIT:
              new->b = _new_op (ctx, IR_OP_CONST_FLOAT);
              break;
            case AST_STRLIT:
              new->b = _new_op (ctx, IR_OP_CONST_STR);
              break;
            case AST_BOOL:
              new->b = _new_op (ctx, IR_OP_CONST_BOOL);
              break;
            default:
              CLOMY_FAIL ("Unreachable. Unknown literal.");
              break;
            }

          vt_itm = aralloc (&ctx->ar, sizeof (ir_vartable_item));
          vt_itm->var = new;

          stput (&ctx->var_table, new->a->str_data, &vt_itm);
          break;
        case AST_VAR_ASSIGN:
          _ir_var_assign (ctx, ptr);
          break;
        case AST_FUNCALL:
          _ir_func_call (ctx, ptr);
          break;
        case AST_BLOCK:
          _ir_block (ctx, ptr, 1);
          break;
        case AST_COND:
          _ir_cond (ctx, ptr);
          break;
        default:
          printf ("[INFO] ptr->type = %d\n", ptr->type);
          CLOMY_FAIL ("Unreachable.");
          break;
        }
      ptr = ptr->next;
    }
}

void
ir_print (ir *ctx)
{
  ir_tac *ptr;
  unsigned int i;

  for (i = 0; i < ctx->ops.size; ++i)
    {
      ptr = *(ir_tac **)dageti (&ctx->ops, i);
      _print_op (ptr);
    }
}

void
ir_fold (ir *ctx)
{
  sbfold (&ctx->sb);
  stfold (&ctx->var_table);
  dafold (&ctx->ops);
  arfold (&ctx->ar);
}

ir_tac *
_new_tac (ir *ctx, unsigned short op)
{
  ir_tac *tac = aralloc (&ctx->ar, sizeof (ir_tac));
  tac->op = op;
  return tac;
}

ir_operand *
_new_op (ir *ctx, unsigned int type)
{
  ir_operand *op = aralloc (&ctx->ar, sizeof (ir_operand));
  op->type = type;
  return op;
}

void
_ir_var_assign (ir *ctx, ast_node *n)
{
  ir_tac *new;
  ast_data_var_assign *var_data = n->data;
  ast_data_var_declare *vard_data = var_data->var->data;
  ast_data_op *op_data;

  if (var_data->value->type == AST_OP)
    {
      op_data = var_data->value->data;
      switch (op_data->op)
        {
        case '+':
          new = _new_tac (ctx, IR_ASSIGN_ADD);
          break;
        case '-':
          new = _new_tac (ctx, IR_ASSIGN_SUB);
          break;
        case '*':
          new = _new_tac (ctx, IR_ASSIGN_MUL);
          break;
        case '/':
          new = _new_tac (ctx, IR_ASSIGN_DIV);
          break;
        case '!':
          new = _new_tac (ctx, IR_ASSIGN_NOT);
          break;
        default:
          printf ("[INFO] op_data->op = %c\n", op_data->op);
          printf ("[INFO] AST Dump:\n");
          ast_print_tree (n, "\n");
          CLOMY_FAIL ("Unreachable.");
          break;
        }
      new->a = _new_op (ctx, IR_OP_VAR);
      new->a->str_data = vard_data->name->data;
      if (op_data->left)
        {
          if (((ast_node *)op_data->left)->type == AST_OP)
            new->b = _temp_var (ctx, op_data->left);
          else
            new->b = _to_ir_type (ctx, op_data->left);
        }
      if (op_data->right)
        {
          if (((ast_node *)op_data->right)->type == AST_OP)
            new->c = _temp_var (ctx, op_data->right);
          else
            new->c = _to_ir_type (ctx, op_data->right);
        }
    }
  else
    {
      new = _new_tac (ctx, IR_ASSIGN);
      new->a = _new_op (ctx, IR_OP_VAR);
      new->a->str_data = vard_data->name->data;
      new->b = _to_ir_type (ctx, var_data->value);
    }
  daappend (&ctx->ops, &new);
}

void
_ir_func_call (ir *ctx, ast_node *n)
{
  ast_data_funcall *fun_data = n->data;
  ast_data_var_declare *vard_data;
  ast_node *ptr = fun_data->args_head;
  ir_tac *new;
  ir_vartable_item *vt_itm;

  while (ptr)
    {
      new = _new_tac (ctx, IR_PUSH_ARG);
      if (ptr->type != AST_VAR_DECLARE)
        {
          new->a = _temp_var (ctx, ptr);
        }
      else
        {
          vard_data = ptr->data;
          new->a = _new_op (ctx, IR_OP_VAR);
          new->a->str_data = vard_data->name->data;
        }

      daappend (&ctx->ops, &new);

      vt_itm = *(ir_vartable_item **)stget (&ctx->var_table, new->a->str_data);
      ++vt_itm->usage;

      ptr = ptr->next;
    }

  new = _new_tac (ctx, IR_CALL);
  new->a = _new_op (ctx, IR_OP_FUN);
  new->a->str_data = fun_data->name->data;
  daappend (&ctx->ops, &new);
}

ir_tac *
_ir_block (ir *ctx, ast_node *n, unsigned short depth)
{
  ir_tac *new;
  ast_data_block *data = n->data;
  IR_NEW_BLOCK (new);
  daappend (&ctx->ops, &new);

  _ir_parse (ctx, data->next);

  if (depth)
    {
      IR_NEW_BLOCK (new);
      daappend (&ctx->ops, &new);
    }
  return new;
}

void
_ir_cond (ir *ctx, ast_node *n)
{
  ir_tac *new = _new_tac (ctx, IR_IF), *after, *block, *block_after;
  ast_data_cond *data = n->data;
  new->a = _to_ir_type (ctx, data->cond);
  daappend (&ctx->ops, &new);

  /* YES branch block. */
  if (data->yes->type != AST_BLOCK)
    {
      IR_NEW_BLOCK (block);
      daappend (&ctx->ops, &block);
      _ir_parse (ctx, data->yes);
    }
  else
    {
      block = _ir_block (ctx, data->yes, 0);
    }

  /* Block to jump after condition. */
  IR_NEW_BLOCK (block_after);
  if (data->no)
    {
      after = _new_tac (ctx, IR_JUMP);
      after->a = _new_op (ctx, IR_OP_LABEL);
      after->a->str_data = block_after->a->str_data;
      daappend (&ctx->ops, &after);
    }

  new->b = _new_op (ctx, IR_OP_LABEL);
  new->b->str_data = block->a->str_data;

  if (!data->yes)
    CLOMY_FAIL ("[IR] Unreachable. Condition should have atleast YES branch");

  /* NO branch block. */
  if (data->no)
    {
      if (data->no->type == AST_BLOCK)
        {
          block = _ir_block (ctx, data->no, 0);
        }
      else
        {
          IR_NEW_BLOCK (block);
          daappend (&ctx->ops, &block);
          _ir_parse (ctx, data->no);
        }

      new->c = _new_op (ctx, IR_OP_LABEL);
      new->c->str_data = block->a->str_data;
    }
  else
    {
      new->c = _new_op (ctx, IR_OP_LABEL);
      new->c->str_data = block_after->a->str_data;
    }

  daappend (&ctx->ops, &block_after);
}

void
_print_op (ir_tac *itm)
{
  switch (itm->op)
    {
    case IR_LABEL:
      printf ("\n%s:\n", itm->a->str_data);
      break;
    case IR_DECLARE:
      printf ("  %s %s\n", _ir_datatype (itm->b), itm->a->str_data);
      break;
    case IR_ASSIGN:
      printf ("  %s = ", itm->a->str_data);
      _print_exp (itm->b);
      printf ("\n");
      break;
    case IR_JUMP:
      printf ("  jump ");
      _print_exp (itm->a);
      printf ("\n");
      break;
    case IR_IF:
      printf ("  if ");
      _print_exp (itm->a);
      if (itm->b)
        {
          printf (" then ");
          _print_exp (itm->b);
        }
      if (itm->c)
        {
          printf ("\n  else ");
          _print_exp (itm->c);
        }
      printf ("\n");
      break;
    case IR_ASSIGN_ADD:
      printf ("  %s = ", itm->a->str_data);
      _print_exp (itm->b);
      printf (" + ");
      _print_exp (itm->c);
      printf ("\n");
      break;
    case IR_ASSIGN_SUB:
      printf ("  %s = ", itm->a->str_data);
      _print_exp (itm->b);
      printf (" - ");
      _print_exp (itm->c);
      printf ("\n");
      break;
    case IR_ASSIGN_MUL:
      printf ("  %s = ", itm->a->str_data);
      _print_exp (itm->b);
      printf (" * ");
      _print_exp (itm->c);
      printf ("\n");
      break;
    case IR_ASSIGN_DIV:
      printf ("  %s = ", itm->a->str_data);
      _print_exp (itm->b);
      printf (" / ");
      _print_exp (itm->c);
      printf ("\n");
      break;
    case IR_ASSIGN_NOT:
      printf ("  %s = ", itm->a->str_data);
      printf (" !");
      _print_exp (itm->c);
      printf ("\n");
      break;
    case IR_PUSH_ARG:
      printf ("  push_arg %s\n", itm->a->str_data);
      break;
    case IR_CALL:
      printf ("  call %s\n", itm->a->str_data);
      break;
    default:
      printf ("%d?\n", itm->op);
      break;
    }
}

void
_print_exp (ir_operand *op)
{
  switch (op->type)
    {
    case IR_OP_LABEL:
      printf ("block %s", op->str_data);
      break;
    case IR_OP_CONST_FLOAT:
      printf ("%f", op->float_data);
      break;
    case IR_OP_CONST_INT:
      printf ("%ld", op->int_data);
      break;
    case IR_OP_CONST_STR:
      printf ("\"%s\"", op->str_data);
      break;
    case IR_OP_CONST_BOOL:
      printf ("%s", op->bool_data == 1 ? "true" : "false");
      break;
    case IR_OP_VAR:
      printf ("%s", op->str_data);
      break;
    default:
      printf ("%d?", op->type);
      break;
    }
}

ir_operand *
_temp_var (ir *ctx, ast_node *n)
{
  ast_data_op *op_data = n->data;
  ir_tac *dec, *new = _new_tac (ctx, _op_ast_to_ir (n));
  ir_vartable_item *vt_itm;
  string *var_name;

  sbappendch (&ctx->sb, 't');
  sbappendch (&ctx->sb, (char)(ctx->tempvar_count++ + '0'));
  var_name = sbflush (&ctx->sb);

  new->a = _new_op (ctx, IR_OP_VAR);
  new->a->str_data = var_name->data;

  if (n->type != AST_OP)
    {
      new->b = _to_ir_type (ctx, n);
    }
  else
    {
      if (((ast_node *)op_data->left)->type == AST_OP)
        new->b = _temp_var (ctx, op_data->left);
      else
        new->b = _to_ir_type (ctx, op_data->left);

      if (((ast_node *)op_data->right)->type == AST_OP)
        new->c = _temp_var (ctx, op_data->right);
      else
        new->c = _to_ir_type (ctx, op_data->right);
    }

  dec = _new_tac (ctx, IR_DECLARE);
  dec->a = _new_op (ctx, IR_OP_VAR);
  dec->a->str_data = var_name->data;
  dec->b = _new_op (ctx, _ir_expr_datatype (ctx, new->b));

  vt_itm = aralloc (&ctx->ar, sizeof (ir_vartable_item));
  vt_itm->var = dec;
  stput (&ctx->var_table, dec->a->str_data, &vt_itm);

  daappend (&ctx->ops, &new);

  return new->a;
}

unsigned short
_op_ast_to_ir (ast_node *n)
{
  ast_data_op *op_data = n->data;

  if (n->type != AST_OP)
    return IR_ASSIGN;

  switch (op_data->op)
    {
    case '+':
      return IR_ASSIGN_ADD;
    case '-':
      return IR_ASSIGN_SUB;
    case '*':
      return IR_ASSIGN_MUL;
    case '/':
      return IR_ASSIGN_DIV;
    default:
      return IR_ASSIGN;
    }
}

ir_operand *
_to_ir_type (ir *ctx, ast_node *n)
{
  ir_operand *op = aralloc (&ctx->ar, sizeof (ir_operand));
  string *str_data;

  switch (n->type)
    {
    case AST_INTLIT:
      op->type = IR_OP_CONST_INT;
      op->int_data = *(long *)n->data;
      break;
    case AST_STRLIT:
      str_data = n->data;
      op->type = IR_OP_CONST_STR;
      op->str_data = str_data->data;
      break;
    case AST_FLOATLIT:
      op->type = IR_OP_CONST_FLOAT;
      op->float_data = *(double *)n->data;
      break;
    case AST_VAR_DECLARE:
      op->type = IR_OP_VAR;
      op->str_data = ((ast_data_var_declare *)n->data)->name->data;
      break;
    case AST_BOOL:
      op->type = IR_OP_CONST_BOOL;
      op->bool_data = *(unsigned short *)n->data;
      break;
    default:
      printf ("[INFO] n->type = %d\n", n->type);
      CLOMY_FAIL ("Unreachable.");
      break;
    }
  return op;
}

static char *
_ir_datatype (ir_operand *op)
{
  switch (op->type)
    {
    case IR_OP_CONST_INT:
      return "int";
    case IR_OP_CONST_STR:
      return "str";
    case IR_OP_CONST_FLOAT:
      return "float";
    case IR_OP_CONST_BOOL:
      return "bool";
    default:
      printf ("[INFO] Unknown datatype %d\n", op->type);
      return "unk";
    }
}

unsigned short
_is_assign (ir_tac *n)
{
  return n->op == IR_ASSIGN || n->op == IR_ASSIGN_ADD || n->op == IR_ASSIGN_SUB
         || n->op == IR_ASSIGN_MUL || n->op == IR_ASSIGN_DIV
         || n->op == IR_ASSIGN_NOT;
}

void
_ir_print_vartable (ir *ctx)
{
  ir_vartable_item *vt_itm;
  clomy_stdata *std;
  unsigned int i;

  printf (";; --------------------\n");
  for (i = 0; i < ctx->var_table.capacity; ++i)
    {
      std = ctx->var_table.data[i];
      while (std)
        {
          vt_itm = *(ir_vartable_item **)std->data;
          printf (";; %s\t%s\t%s\t%d\n", _ir_datatype (vt_itm->var->b),
                  vt_itm->var->a->str_data,
                  vt_itm->static_value ? "yes" : "no", vt_itm->usage);
          std = std->next;
        }
    }
  printf (";; --------------------\n");
}

unsigned short
_ir_expr_datatype (ir *ctx, ir_operand *itm)
{
  ir_vartable_item *vt_itm;
  if (itm->type == IR_OP_VAR && stget (&ctx->var_table, itm->str_data))
    {
      vt_itm = *(ir_vartable_item **)stget (&ctx->var_table, itm->str_data);
      return vt_itm->var->b->type;
    }
  return itm->type;
}
