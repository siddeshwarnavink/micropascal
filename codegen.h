#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"

struct cg
{
  arena ar;
  stringbuilder sb;
  da var_declares;
};
typedef struct cg cg;

string *codegen (cg *ctx, ast_node *root);

void codegen_fold (cg *ctx);

#endif /* not CODEGEN_H */
