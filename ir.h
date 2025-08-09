#ifndef IR_H
#define IR_H

#include "ast.h"

#define IR_CHECK_ALREADY_KNOW(ptr)                                            \
  if ((ptr) && (ptr)->type == IR_OP_VAR)                                      \
    {                                                                         \
      vt_itm                                                                  \
          = *(ir_vartable_item **)stget (&ctx->var_table, (ptr)->str_data);   \
      if (vt_itm->static_value)                                               \
        {                                                                     \
          (ptr) = _new_op (ctx, _ir_expr_datatype (ctx, (ptr)));              \
          switch ((ptr)->type)                                                \
            {                                                                 \
            case IR_OP_CONST_STR:                                             \
              (ptr)->str_data = vt_itm->str_data;                             \
              break;                                                          \
            case IR_OP_CONST_FLOAT:                                           \
              (ptr)->float_data = vt_itm->float_data;                         \
              break;                                                          \
            case IR_OP_CONST_INT:                                             \
              (ptr)->int_data = vt_itm->int_data;                             \
              break;                                                          \
            default:                                                          \
              CLOMY_FAIL ("Unreachable.");                                    \
              break;                                                          \
            }                                                                 \
        }                                                                     \
    }

#define IR_PERFORM_OP(vd, a, b)                                               \
  switch (ptr->op)                                                            \
    {                                                                         \
    case IR_ASSIGN_ADD:                                                       \
      (vd) = (a) + (b);                                                       \
      break;                                                                  \
    case IR_ASSIGN_SUB:                                                       \
      (vd) = (a) - (b);                                                       \
      break;                                                                  \
    case IR_ASSIGN_MUL:                                                       \
      (vd) = (a) * (b);                                                       \
      break;                                                                  \
    case IR_ASSIGN_DIV:                                                       \
      (vd) = (a) / (b);                                                       \
      break;                                                                  \
    default:                                                                  \
      CLOMY_FAIL ("Unreachable.");                                            \
      break;                                                                  \
    }

enum ir_optype
{
  IR_DECLARE = 0,
  IR_ASSIGN,
  IR_ASSIGN_ADD,
  IR_ASSIGN_SUB,
  IR_ASSIGN_MUL,
  IR_ASSIGN_DIV,
  IR_ADD,
  IR_PUSH_ARG,
  IR_CALL,
  IR_LABEL,
};

enum ir_operand_type
{
  IR_OP_VAR = 0,
  IR_OP_FUN,
  IR_OP_LABEL,
  IR_OP_CONST_INT,
  IR_OP_CONST_FLOAT,
  IR_OP_CONST_STR,
};

struct ir_operand
{
  unsigned short type;
  union
  {
    char *str_data;
    long int_data;
    double float_data;
  };
};
typedef struct ir_operand ir_operand;

struct ir_tac
{
  unsigned short op;
  ir_operand *a;
  ir_operand *b;
  ir_operand *c;
  struct ir_tac *next;
};
typedef struct ir_tac ir_tac;

struct ir_vartable_item
{
  ir_tac *var;
  unsigned short static_value;
  unsigned int usage;
  union
  {
    char *str_data;
    long int_data;
    double float_data;
  };
};
typedef struct ir_vartable_item ir_vartable_item;

struct ir
{
  arena ar;
  stringbuilder sb;
  ht var_table;
  da ops;
  unsigned int tempvar_count;
};
typedef struct ir ir;

void ir_init (ir *ctx, ast_node *root);

void ir_print (ir *ctx);

void ir_fold (ir *ctx);

#endif /* not IR_H */
