#include "ir.h"

void _ir_parse (ir *ctx, ast_node *root);
static ir_tac *_new_tac (ir *ctx, unsigned short op);
static ir_operand *_new_op (ir *ctx, unsigned int type);
static void _ir_var_assign (ir *ctx, ast_node *n);
static ir_operand *_to_ir_type (ir *ctx, ast_node *n);
static unsigned short _op_ast_to_ir (ast_node *n);
static ir_operand *_temp_var (ir *ctx, ast_node *n);
static void _print_exp (ir_operand *op);
static void _print_op (ir_tac *itm);
static char *_ir_datatype (ir_operand *op);

void
ir_init (ir *ctx, ast_node *root)
{
  dainit (&ctx->ops, &ctx->ar, sizeof (ir_tac *), 64);
  htinit (&ctx->var_table, &ctx->ar, 16, sizeof (ir_tac *));
  _ir_parse (ctx, root);
}

void
_ir_parse (ir *ctx, ast_node *root)
{
  ast_node *ptr = root;
  ir_tac *new, *tac_ptr;
  clomy_stdata *std;
  unsigned int i;

  sbinit (&ctx->sb, &ctx->ar);

  while (ptr)
    {
      switch (ptr->type)
        {
        case AST_MAIN_BLOCK:
          new = _new_tac (ctx, IR_LABEL);
          new->a = _new_op (ctx, IR_OP_LABEL);
          sbappend (&ctx->sb, "_start");
          new->a->str_data = sbflush (&ctx->sb)->data;
          daappend (&ctx->ops, &new);

          for (i = 0; i < ctx->var_table.capacity; ++i)
            {
              std = ctx->var_table.data[i];
              while (std)
                {
                  tac_ptr = *(ir_tac **)std->data;
                  daappend (&ctx->ops, &tac_ptr);
                  std = std->next;
                }
            }

          stfold (&ctx->var_table);
          htinit (&ctx->var_table, &ctx->ar, 16, sizeof (ir_tac *));

          _ir_parse (ctx, ((ast_data_block *)ptr->data)->next);
          break;
        case AST_VAR_DECLARE:
          new = _new_tac (ctx, IR_DECLARE);
          new->a = _new_op (ctx, IR_OP_VAR);
          new->a->str_data = ((ast_data_var_declare *)ptr->data)->name->data;
          new->b = _new_op (ctx, IR_OP_CONST_INT);
          stput (&ctx->var_table, new->a->str_data, &new);
          break;
        case AST_VAR_ASSIGN:
          _ir_var_assign (ctx, ptr);
          break;
        }
      ptr = ptr->next;
    }
  sbfold (&ctx->sb);
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
        }
      new->a = _new_op (ctx, IR_OP_VAR);
      new->a->str_data = vard_data->name->data;
      if (((ast_node *)op_data->left)->type == AST_OP)
        new->b = _temp_var (ctx, op_data->left);
      else
        new->b = _to_ir_type (ctx, op_data->left);

      if (((ast_node *)op_data->right)->type == AST_OP)
        new->c = _temp_var (ctx, op_data->right);
      else
        new->c = _to_ir_type (ctx, op_data->right);
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
    case IR_OP_CONST_INT:
      printf ("%ld", op->int_data);
      break;
    case IR_OP_CONST_STR:
      printf ("\"%s\"", op->str_data);
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
  ir_tac *new = _new_tac (ctx, _op_ast_to_ir (n));
  string *var_name;

  sbappendch (&ctx->sb, 't');
  sbappendch (&ctx->sb, (char)(ctx->tempvar_count++ + '0'));
  var_name = sbflush (&ctx->sb);

  new->a = _new_op (ctx, IR_OP_VAR);
  new->a->str_data = var_name->data;

  if (((ast_node *)op_data->left)->type == AST_OP)
    new->b = _temp_var (ctx, op_data->left);
  else
    new->b = _to_ir_type (ctx, op_data->left);

  if (((ast_node *)op_data->right)->type == AST_OP)
    new->c = _temp_var (ctx, op_data->right);
  else
    new->c = _to_ir_type (ctx, op_data->right);

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

  switch (n->type)
    {
    case AST_INTLIT:
      op->type = IR_OP_CONST_INT;
      op->int_data = *(long *)n->data;
      break;
    case AST_STRLIT:
      op->type = IR_OP_CONST_STR;
      op->str_data = (char *)n->data;
      break;
    case AST_VAR_DECLARE:
      op->type = IR_OP_VAR;
      op->str_data = ((ast_data_var_declare *)n->data)->name->data;
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
    default:
      return "unk";
    }
}
