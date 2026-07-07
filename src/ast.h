#ifndef AST_H
#define AST_H

#include "token.h"

// AST 节点类型
typedef enum {
    AST_NONE = 0,
    AST_INT,        // 42
    AST_FLOAT,      // 3.14
    AST_STRING,     // "hello"
    AST_BOOL,       // True / False
    AST_NIL,        // None
    AST_IDENT,      // 变量引用
    AST_BINARY,     // a + b, 3 * 4 等二元运算
    AST_UNARY,      // -a, not x 等一元运算
    AST_CALL,       // print(x), add(1, 2)
    AST_ASSIGN,     // x = expr
    AST_IF,         // if / elif / else
    AST_WHILE,      // while
    AST_RETURN,     // return expr
    AST_BLOCK,      // { stmt; stmt; ... }
    AST_PROGRAM,    // 根节点
    AST_FUNC_DEF,   // def name(params) { body }
} ASTKind;

typedef struct ASTNode {
    ASTKind kind;
    int line;           // 出错时定位

    union {
        // 字面量
        long    ival;
        double  fval;
        char   *sval;
        int     bval;

        // 变量引用
        char *ident_name;

        // 一元运算: -expr, not expr
        struct {
            int op;                 // TOK_MINUS or TOK_NOT
            struct ASTNode *expr;
        } unary;

        // 二元运算: lhs op rhs
        struct {
            struct ASTNode *lhs;
            int op;                 // TOK_PLUS / TOK_EQEQ / ...
            struct ASTNode *rhs;
        } binary;

        // 函数调用: name(args...)
        struct {
            char *name;
            struct ASTNode **args;
            int arg_count;
        } call;

        // 赋值: name = value
        struct {
            char *name;
            struct ASTNode *value;
        } assign;

        // if: if cond { then_body } [elif ...] [else else_body]
        struct {
            struct ASTNode *cond;
            struct ASTNode *then_body;   // AST_BLOCK
            struct ASTNode *else_body;   // AST_BLOCK 或另一个 AST_IF（elif 链）
        } if_stmt;

        // while: while cond { body }
        struct {
            struct ASTNode *cond;
            struct ASTNode *body;        // AST_BLOCK
        } while_stmt;

        // return: return expr
        struct {
            struct ASTNode *expr;        // NULL = 空 return
        } ret;

        // 语句块: { stmt... }
        struct {
            struct ASTNode **items;
            int count;
            int cap;
        } block;

        // 根节点
        struct {
            struct ASTNode **items;
            int count;
            int cap;
        } program;

        // 函数定义: def name(params) { body }
        struct {
            char *name;
            char **params;
            int param_count;
            struct ASTNode *body;        // AST_BLOCK
        } func_def;
    };
} ASTNode;

// ---- 构造函数 ----
ASTNode *ast_int(long val);
ASTNode *ast_float(double val);
ASTNode *ast_string(char *s);
ASTNode *ast_bool(int val);
ASTNode *ast_none(void);
ASTNode *ast_ident(char *name);
ASTNode *ast_unary(int op, ASTNode *expr);
ASTNode *ast_binary(ASTNode *lhs, int op, ASTNode *rhs);
ASTNode *ast_call(char *name, ASTNode **args, int arg_count);
ASTNode *ast_assign(char *name, ASTNode *value);
ASTNode *ast_if(ASTNode *cond, ASTNode *then_body, ASTNode *else_body);
ASTNode *ast_while(ASTNode *cond, ASTNode *body);
ASTNode *ast_return(ASTNode *expr);
ASTNode *ast_block(void);
ASTNode *ast_program(void);
ASTNode *ast_func_def(char *name, char **params, int param_count, ASTNode *body);

// ---- 工具 ----
void     ast_block_add(ASTNode *block, ASTNode *stmt);
void     ast_program_add(ASTNode *prog, ASTNode *stmt);
void     ast_free(ASTNode *node);
const char *ast_kind_name(ASTKind kind);

#endif
