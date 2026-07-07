#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ast.h"

#define BLOCK_INIT_CAP 8

static ASTNode *alloc_node(ASTKind kind) {
    ASTNode *n = calloc(1, sizeof(ASTNode));
    n->kind = kind;
    return n;
}

// ---- 字面量 ----
ASTNode *ast_int(long val) {
    ASTNode *n = alloc_node(AST_INT);
    n->ival = val;
    return n;
}
ASTNode *ast_float(double val) {
    ASTNode *n = alloc_node(AST_FLOAT);
    n->fval = val;
    return n;
}
ASTNode *ast_string(char *s) {
    ASTNode *n = alloc_node(AST_STRING);
    n->sval = s;  // ownership transfer
    return n;
}
ASTNode *ast_bool(int val) {
    ASTNode *n = alloc_node(AST_BOOL);
    n->bval = val;
    return n;
}
ASTNode *ast_none(void) {
    return alloc_node(AST_NIL);
}

// ---- 表达式 ----
ASTNode *ast_ident(char *name) {
    ASTNode *n = alloc_node(AST_IDENT);
    n->ident_name = name;
    return n;
}
ASTNode *ast_unary(int op, ASTNode *expr) {
    ASTNode *n = alloc_node(AST_UNARY);
    n->unary.op = op;
    n->unary.expr = expr;
    return n;
}
ASTNode *ast_binary(ASTNode *lhs, int op, ASTNode *rhs) {
    ASTNode *n = alloc_node(AST_BINARY);
    n->binary.lhs = lhs;
    n->binary.op = op;
    n->binary.rhs = rhs;
    return n;
}
ASTNode *ast_call(char *name, ASTNode **args, int arg_count) {
    ASTNode *n = alloc_node(AST_CALL);
    n->call.name = name;
    n->call.args = args;
    n->call.arg_count = arg_count;
    return n;
}

// ---- 语句 ----
ASTNode *ast_assign(char *name, ASTNode *value) {
    ASTNode *n = alloc_node(AST_ASSIGN);
    n->assign.name = name;
    n->assign.value = value;
    return n;
}
ASTNode *ast_if(ASTNode *cond, ASTNode *then_body, ASTNode *else_body) {
    ASTNode *n = alloc_node(AST_IF);
    n->if_stmt.cond = cond;
    n->if_stmt.then_body = then_body;
    n->if_stmt.else_body = else_body;
    return n;
}
ASTNode *ast_while(ASTNode *cond, ASTNode *body) {
    ASTNode *n = alloc_node(AST_WHILE);
    n->while_stmt.cond = cond;
    n->while_stmt.body = body;
    return n;
}
ASTNode *ast_return(ASTNode *expr) {
    ASTNode *n = alloc_node(AST_RETURN);
    n->ret.expr = expr;
    return n;
}

ASTNode *ast_block(void) {
    ASTNode *n = alloc_node(AST_BLOCK);
    n->block.cap = BLOCK_INIT_CAP;
    n->block.items = calloc(BLOCK_INIT_CAP, sizeof(ASTNode *));
    return n;
}
ASTNode *ast_program(void) {
    ASTNode *n = alloc_node(AST_PROGRAM);
    n->program.cap = BLOCK_INIT_CAP;
    n->program.items = calloc(BLOCK_INIT_CAP, sizeof(ASTNode *));
    return n;
}

ASTNode *ast_func_def(char *name, char **params, int param_count, ASTNode *body) {
    ASTNode *n = alloc_node(AST_FUNC_DEF);
    n->func_def.name = name;
    n->func_def.params = params;
    n->func_def.param_count = param_count;
    n->func_def.body = body;
    return n;
}

// ---- 动态数组 ----
void ast_block_add(ASTNode *block, ASTNode *stmt) {
    if (block->block.count >= block->block.cap) {
        block->block.cap *= 2;
        block->block.items = realloc(block->block.items,
                                      block->block.cap * sizeof(ASTNode *));
    }
    block->block.items[block->block.count++] = stmt;
}

void ast_program_add(ASTNode *prog, ASTNode *stmt) {
    if (prog->program.count >= prog->program.cap) {
        prog->program.cap *= 2;
        prog->program.items = realloc(prog->program.items,
                                       prog->program.cap * sizeof(ASTNode *));
    }
    prog->program.items[prog->program.count++] = stmt;
}

// ---- 递归释放 ----
void ast_free(ASTNode *node) {
    if (!node) return;
    switch (node->kind) {
    case AST_STRING:
        free(node->sval);
        break;
    case AST_IDENT:
        free(node->ident_name);
        break;
    case AST_UNARY:
        ast_free(node->unary.expr);
        break;
    case AST_BINARY:
        ast_free(node->binary.lhs);
        ast_free(node->binary.rhs);
        break;
    case AST_CALL:
        free(node->call.name);
        for (int i = 0; i < node->call.arg_count; i++)
            ast_free(node->call.args[i]);
        free(node->call.args);
        break;
    case AST_ASSIGN:
        free(node->assign.name);
        ast_free(node->assign.value);
        break;
    case AST_IF:
        ast_free(node->if_stmt.cond);
        ast_free(node->if_stmt.then_body);
        ast_free(node->if_stmt.else_body);
        break;
    case AST_WHILE:
        ast_free(node->while_stmt.cond);
        ast_free(node->while_stmt.body);
        break;
    case AST_RETURN:
        ast_free(node->ret.expr);
        break;
    case AST_BLOCK:
        for (int i = 0; i < node->block.count; i++)
            ast_free(node->block.items[i]);
        free(node->block.items);
        break;
    case AST_PROGRAM:
        for (int i = 0; i < node->program.count; i++)
            ast_free(node->program.items[i]);
        free(node->program.items);
        break;
    case AST_FUNC_DEF:
        free(node->func_def.name);
        for (int i = 0; i < node->func_def.param_count; i++)
            free(node->func_def.params[i]);
        free(node->func_def.params);
        ast_free(node->func_def.body);
        break;
    default: break;
    }
    free(node);
}

// ---- 调试 ----
const char *ast_kind_name(ASTKind kind) {
    static const char *names[] = {
        [AST_NONE]     = "none",
        [AST_INT]      = "int",
        [AST_FLOAT]    = "float",
        [AST_STRING]   = "string",
        [AST_BOOL]     = "bool",
        [AST_NIL]      = "nil",
        [AST_IDENT]    = "ident",
        [AST_BINARY]   = "binary",
        [AST_UNARY]    = "unary",
        [AST_CALL]     = "call",
        [AST_ASSIGN]   = "assign",
        [AST_IF]       = "if",
        [AST_WHILE]    = "while",
        [AST_RETURN]   = "return",
        [AST_BLOCK]    = "block",
        [AST_PROGRAM]  = "program",
        [AST_FUNC_DEF] = "func_def",
    };
    return names[kind];
}
