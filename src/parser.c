#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "parser.h"

struct Parser {
    Lexer *lex;
    Token  cur;         // 当前 token
    int    has_error;
    char   error_msg[256];
};

// ---- 前向声明 ----
static ASTNode *parse_statement(Parser *p);
static ASTNode *parse_block(Parser *p);
static ASTNode *parse_expr(Parser *p);
static ASTNode *parse_if(Parser *p);
static ASTNode *parse_while(Parser *p);
static ASTNode *parse_func_def(Parser *p);
static ASTNode *parse_return(Parser *p);

// ---- 内部工具 ----

static void advance(Parser *p) {
    p->cur = lexer_next(p->lex);
}

static int check(Parser *p, TokenKind kind) {
    return p->cur.kind == kind;
}

static int match(Parser *p, TokenKind kind) {
    if (check(p, kind)) { advance(p); return 1; }
    return 0;
}

static void expect(Parser *p, TokenKind kind, const char *msg) {
    if (match(p, kind)) return;
    p->has_error = 1;
    snprintf(p->error_msg, sizeof(p->error_msg),
             "line %d: expected %s, got '%s'", p->cur.line, msg, p->cur.lexeme ? p->cur.lexeme : "?");
}

static void error(Parser *p, const char *msg) {
    if (p->has_error) return;  // 只保留第一个错误
    p->has_error = 1;
    snprintf(p->error_msg, sizeof(p->error_msg), "line %d: %s", p->cur.line, msg);
}

// print 等关键字可以当函数名调用
static int is_callable(TokenKind k) {
    return k == TOK_IDENT || k == TOK_PRINT;
}

// 跳过语句间的换行
static void skip_newlines(Parser *p) {
    while (match(p, TOK_NEWLINE));
}

// ---- 入口 ----

static ASTNode *parse_program(Parser *p) {
    ASTNode *prog = ast_program();
    skip_newlines(p);
    while (!check(p, TOK_EOF) && !p->has_error) {
        ASTNode *stmt = parse_statement(p);
        if (stmt) ast_program_add(prog, stmt);
        skip_newlines(p);
    }
    return prog;
}

// ---- 语句分发 ----

static ASTNode *parse_statement(Parser *p) {
    switch (p->cur.kind) {
    case TOK_IF:     return parse_if(p);
    case TOK_WHILE:  return parse_while(p);
    case TOK_DEF:    return parse_func_def(p);
    case TOK_RETURN: return parse_return(p);
    case TOK_LBRACE: return parse_block(p);
    default: {
        // 表达式语句: expr / IDENT = expr / print(...)
        ASTNode *expr = parse_expr(p);
        if (!expr) return NULL;
        // 如果是 assign，已经被 parse_expr 的 assign 分支处理了
        // 否则就是裸表达式语句
        if (expr->kind == AST_ASSIGN) return expr;
        return expr;  // 如 print("hello") 作为 AST_CALL
    }}
}

// ---- 语句块 ----

static ASTNode *parse_block(Parser *p) {
    expect(p, TOK_LBRACE, "{");
    ASTNode *block = ast_block();
    skip_newlines(p);
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF) && !p->has_error) {
        ASTNode *stmt = parse_statement(p);
        if (stmt) ast_block_add(block, stmt);
        skip_newlines(p);
    }
    expect(p, TOK_RBRACE, "}");
    return block;
}

// ---- if / elif / else ----

static ASTNode *parse_if(Parser *p) {
    advance(p);  // 吃掉 if
    ASTNode *cond = parse_expr(p);
    if (!cond) return NULL;
    ASTNode *then_body = parse_block(p);
    ASTNode *else_body = NULL;

    skip_newlines(p);
    if (match(p, TOK_ELIF)) {
        // elif 递归下沉为嵌套 AST_IF
        ASTNode *elif_cond = parse_expr(p);
        if (elif_cond) {
            ASTNode *elif_then = parse_block(p);
            else_body = ast_if(elif_cond, elif_then, NULL);
            // 继续检查后续 elif/else，挂在 else_body 上
            ASTNode **tail = &else_body->if_stmt.else_body;
            skip_newlines(p);
            while (check(p, TOK_ELIF) && !p->has_error) {
                advance(p);
                ASTNode *ec = parse_expr(p);
                ASTNode *eb = parse_block(p);
                *tail = ast_if(ec, eb, NULL);
                tail = &((*tail)->if_stmt.else_body);
                skip_newlines(p);
            }
            if (match(p, TOK_ELSE)) {
                *tail = parse_block(p);
            }
        }
    }
    else if (match(p, TOK_ELSE)) {
        else_body = parse_block(p);
    }

    return ast_if(cond, then_body, else_body);
}

// ---- while ----

static ASTNode *parse_while(Parser *p) {
    advance(p);  // 吃掉 while
    ASTNode *cond = parse_expr(p);
    ASTNode *body = parse_block(p);
    return ast_while(cond, body);
}

// ---- def ----

static ASTNode *parse_func_def(Parser *p) {
    advance(p);  // 吃掉 def
    if (!check(p, TOK_IDENT)) {
        error(p, "expected function name after 'def'");
        return NULL;
    }
    char *name = strdup(p->cur.lexeme);
    advance(p);

    expect(p, TOK_LPAREN, "(");
    // 参数列表
    char **params = NULL;
    int param_count = 0;
    if (!check(p, TOK_RPAREN)) {
        int cap = 4;
        params = malloc(cap * sizeof(char *));
        do {
            if (!check(p, TOK_IDENT)) {
                error(p, "expected parameter name");
                break;
            }
            if (param_count >= cap) { cap *= 2; params = realloc(params, cap * sizeof(char *)); }
            params[param_count++] = strdup(p->cur.lexeme);
            advance(p);
        } while (match(p, TOK_COMMA));
    }
    expect(p, TOK_RPAREN, ")");
    ASTNode *body = parse_block(p);
    return ast_func_def(name, params, param_count, body);
}

// ---- return ----

static ASTNode *parse_return(Parser *p) {
    advance(p);  // 吃掉 return
    ASTNode *expr = NULL;
    if (!check(p, TOK_NEWLINE) && !check(p, TOK_RBRACE) && !check(p, TOK_EOF))
        expr = parse_expr(p);
    return ast_return(expr);
}

// ---- 表达式层级（从低到高优先级） ----

static ASTNode *parse_or(Parser *p);
static ASTNode *parse_and(Parser *p);
static ASTNode *parse_not(Parser *p);
static ASTNode *parse_comparison(Parser *p);
static ASTNode *parse_add(Parser *p);
static ASTNode *parse_mul(Parser *p);
static ASTNode *parse_unary(Parser *p);
static ASTNode *parse_primary(Parser *p);

static ASTNode *parse_expr(Parser *p) {
    return parse_or(p);
}

// or: and { "or" and }
static ASTNode *parse_or(Parser *p) {
    ASTNode *lhs = parse_and(p);
    while (match(p, TOK_OR)) {
        int op = TOK_OR;
        ASTNode *rhs = parse_and(p);
        lhs = ast_binary(lhs, op, rhs);
    }
    return lhs;
}

// and: not { "and" not }
static ASTNode *parse_and(Parser *p) {
    ASTNode *lhs = parse_not(p);
    while (match(p, TOK_AND)) {
        int op = TOK_AND;
        ASTNode *rhs = parse_not(p);
        lhs = ast_binary(lhs, op, rhs);
    }
    return lhs;
}

// not: [ "not" ] comparison
static ASTNode *parse_not(Parser *p) {
    if (match(p, TOK_NOT))
        return ast_unary(TOK_NOT, parse_not(p));
    return parse_comparison(p);
}

// comparison: add { ("==" | "!=" | "<" | ">" | "<=" | ">=") add }
static ASTNode *parse_comparison(Parser *p) {
    ASTNode *lhs = parse_add(p);
    while (check(p, TOK_EQEQ) || check(p, TOK_NEQ) || check(p, TOK_LT) ||
           check(p, TOK_GT)  || check(p, TOK_LE)  || check(p, TOK_GE)) {
        int op = p->cur.kind;
        advance(p);
        ASTNode *rhs = parse_add(p);
        lhs = ast_binary(lhs, op, rhs);
    }
    return lhs;
}

// add: mul { ("+" | "-") mul }
static ASTNode *parse_add(Parser *p) {
    ASTNode *lhs = parse_mul(p);
    while (check(p, TOK_PLUS) || check(p, TOK_MINUS)) {
        int op = p->cur.kind;
        advance(p);
        ASTNode *rhs = parse_mul(p);
        lhs = ast_binary(lhs, op, rhs);
    }
    return lhs;
}

// mul: unary { ("*" | "/" | "%") unary }
static ASTNode *parse_mul(Parser *p) {
    ASTNode *lhs = parse_unary(p);
    while (check(p, TOK_STAR) || check(p, TOK_SLASH) || check(p, TOK_MOD)) {
        int op = p->cur.kind;
        advance(p);
        ASTNode *rhs = parse_unary(p);
        lhs = ast_binary(lhs, op, rhs);
    }
    return lhs;
}

// unary: ["-" | "+"] primary
static ASTNode *parse_unary(Parser *p) {
    if (match(p, TOK_MINUS))
        return ast_unary(TOK_MINUS, parse_unary(p));
    if (match(p, TOK_PLUS))
        return parse_unary(p);  // +expr → 直接忽略
    return parse_primary(p);
}

// primary: INT | FLOAT | STRING | True | False | None
//         | IDENT [ "=" expr ]           ← 赋值
//         | callable "(" args ")"        ← 函数调用
//         | IDENT                        ← 变量引用
//         | "(" expr ")"
static ASTNode *parse_primary(Parser *p) {
    Token t = p->cur;

    // 字面量
    if (match(p, TOK_INT))    { ASTNode *n = ast_int(t.ival); n->line = t.line; free(t.lexeme); return n; }
    if (match(p, TOK_FLOAT))  { ASTNode *n = ast_float(t.fval); n->line = t.line; free(t.lexeme); return n; }
    if (match(p, TOK_STRING)) { ASTNode *n = ast_string(t.sval); n->line = t.line; return n; }
    if (match(p, TOK_TRUE))   return ast_bool(1);
    if (match(p, TOK_FALSE))  return ast_bool(0);
    if (match(p, TOK_NONE))   return ast_none();

    // 括号分组
    if (match(p, TOK_LPAREN)) {
        ASTNode *expr = parse_expr(p);
        expect(p, TOK_RPAREN, ")");
        return expr;
    }

    // 标识符 / 可调用关键字
    if (is_callable(t.kind)) {
        advance(p);

        // 赋值: x = expr
        if (check(p, TOK_ASSIGN)) {
            advance(p);
            ASTNode *val = parse_expr(p);
            ASTNode *n = ast_assign(t.lexeme, val);
            n->line = t.line;
            return n;
        }

        // 函数调用: name(args)
        if (match(p, TOK_LPAREN)) {
            ASTNode **args = NULL;
            int arg_count = 0;
            if (!check(p, TOK_RPAREN)) {
                int cap = 4;
                args = malloc(cap * sizeof(ASTNode *));
                do {
                    if (arg_count >= cap) { cap *= 2; args = realloc(args, cap * sizeof(ASTNode *)); }
                    args[arg_count++] = parse_expr(p);
                } while (match(p, TOK_COMMA));
            }
            expect(p, TOK_RPAREN, ")");
            ASTNode *n = ast_call(t.lexeme, args, arg_count);
            n->line = t.line;
            return n;
        }

        // 普通变量引用
        ASTNode *n = ast_ident(t.lexeme);
        n->line = t.line;
        return n;
    }

    error(p, "unexpected token");
    return NULL;
}

// ---- 生命周期 ----

Parser *parser_new(Lexer *lex) {
    Parser *p = calloc(1, sizeof(Parser));
    p->lex = lex;
    advance(p);  // 预读第一个 token
    return p;
}

ASTNode *parser_parse(Parser *p) {
    return parse_program(p);
}

int parser_has_error(Parser *p) {
    return p->has_error;
}

const char *parser_error(Parser *p) {
    return p->error_msg;
}

void parser_free(Parser *p) {
    if (!p) return;
    free(p);
}
