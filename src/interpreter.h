#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "env.h"
#include "ast.h"

void interp_eval(Env *env, ASTNode *node);
void interp_register_builtins(Env *env);

#endif
