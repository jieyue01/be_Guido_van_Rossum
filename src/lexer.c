#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

// Windows/MinGW 可能没有 strndup
static char *my_strndup(const char *s, size_t n) {
    size_t len = strlen(s);
    if (len > n) len = n;
    char *p = malloc(len + 1);
    if (p) { memcpy(p, s, len); p[len] = '\0'; }
    return p;
}

// ---- 内部工具 ----

static char cur(Lexer *lex) { return *lex->pos; }
static int is_eof(Lexer *lex) { return *lex->pos == '\0'; }

static void advance(Lexer *lex) {
    if (is_eof(lex)) return;
    if (*lex->pos == '\n') { lex->line++; lex->col = 1; }
    else lex->col++;
    lex->pos++;
}

static Token make_tok(Lexer *lex, TokenKind kind) {
    return (Token){.kind = kind, .line = lex->line, .col = lex->col};
}

// ---- 空白和注释 ----

static void skip_line_ws(Lexer *lex) {
    while (cur(lex) == ' ' || cur(lex) == '\t' || cur(lex) == '\r')
        advance(lex);
}

// 跳过行内空白和注释，停在 \n、EOF 或有效字符
static void skip_ws_and_comments(Lexer *lex) {
    while (1) {
        skip_line_ws(lex);
        if (cur(lex) == '#') {
            while (!is_eof(lex) && cur(lex) != '\n') advance(lex);
        } else {
            break;
        }
    }
}

// ---- 字符串 ----

static Token read_string(Lexer *lex) {
    int line = lex->line, col = lex->col;
    advance(lex);  // 跳过开头 "
    char buf[4096];
    int i = 0;
    while (!is_eof(lex) && cur(lex) != '"') {
        if (cur(lex) == '\n' || i >= (int)sizeof(buf) - 1) {
            return (Token){.kind = TOK_ERROR, .line = line, .col = col,
                           .lexeme = "unterminated string"};
        }
        if (cur(lex) == '\\') {
            advance(lex);
            switch (cur(lex)) {
            case 'n':  buf[i++] = '\n'; break;
            case 't':  buf[i++] = '\t'; break;
            case '\\': buf[i++] = '\\'; break;
            case '"':  buf[i++] = '"';  break;
            default:   buf[i++] = cur(lex); break;
            }
        } else {
            buf[i++] = cur(lex);
        }
        advance(lex);
    }
    if (is_eof(lex)) {
        return (Token){.kind = TOK_ERROR, .line = line, .col = col,
                       .lexeme = "unterminated string"};
    }
    advance(lex);  // 跳过结尾 "
    buf[i] = '\0';
    Token t = make_tok(lex, TOK_STRING);
    t.line = line; t.col = col;
    t.sval = strdup(buf);
    t.lexeme = t.sval;
    return t;
}

// ---- 数字 ----

static Token read_number(Lexer *lex) {
    int line = lex->line, col = lex->col;
    const char *start = lex->pos;
    int is_float = 0;

    while (isdigit(cur(lex))) advance(lex);
    if (cur(lex) == '.' && isdigit(*(lex->pos + 1))) {
        is_float = 1;
        advance(lex);
        while (isdigit(cur(lex))) advance(lex);
    }

    Token t;
    t.line = line; t.col = col;
    if (is_float) {
        t.kind = TOK_FLOAT;
        t.fval = strtod(start, NULL);
    } else {
        t.kind = TOK_INT;
        t.ival = strtol(start, NULL, 10);
    }
    // lexeme: 拷贝一份供 parser 调试
    int len = (int)(lex->pos - start);
    char *s = malloc(len + 1);
    memcpy(s, start, len); s[len] = '\0';
    t.lexeme = s;
    return t;
}

// ---- 标识符和关键字 ----

typedef struct { const char *name; TokenKind kind; } Keyword;

static const Keyword keywords[] = {
    {"print", TOK_PRINT}, {"if", TOK_IF},     {"elif", TOK_ELIF},
    {"else", TOK_ELSE},   {"while", TOK_WHILE}, {"def", TOK_DEF},
    {"return", TOK_RETURN}, {"and", TOK_AND}, {"or", TOK_OR},
    {"not", TOK_NOT},     {"True", TOK_TRUE}, {"False", TOK_FALSE},
    {"None", TOK_NONE},
    {NULL, 0},
};

static Token read_ident(Lexer *lex) {
    int line = lex->line, col = lex->col;
    const char *start = lex->pos;
    while (isalnum(cur(lex)) || cur(lex) == '_') advance(lex);
    int len = (int)(lex->pos - start);

    // 查关键字表
    for (const Keyword *kw = keywords; kw->name; kw++) {
        if (strlen(kw->name) == (size_t)len &&
            strncmp(start, kw->name, len) == 0) {
            Token t = make_tok(lex, kw->kind);
            t.line = line; t.col = col;
            t.lexeme = my_strndup(start, len);  // 调试用
            return t;
        }
    }

    // 普通标识符
    Token t = make_tok(lex, TOK_IDENT);
    t.line = line; t.col = col;
    t.lexeme = my_strndup(start, len);
    t.sval = t.lexeme;  // ident name 复用 lexeme
    return t;
}

// ---- 主入口 ----

Token lexer_next(Lexer *lex) {
    if (lex->has_peek) {
        lex->has_peek = 0;
        return lex->peek;
    }

    skip_ws_and_comments(lex);

    if (is_eof(lex)) return make_tok(lex, TOK_EOF);

    int line = lex->line, col = lex->col;
    char c = cur(lex);

    // 换行：发一个 TOK_NEWLINE，跳过紧随的多余换行
    if (c == '\n') {
        advance(lex);
        while (cur(lex) == '\n') advance(lex);
        return (Token){.kind=TOK_NEWLINE,.line=line,.col=col};
    }

    // 单字符 token
    switch (c) {
    case '(': advance(lex); return (Token){.kind=TOK_LPAREN,.line=line,.col=col};
    case ')': advance(lex); return (Token){.kind=TOK_RPAREN,.line=line,.col=col};
    case '{': advance(lex); return (Token){.kind=TOK_LBRACE,.line=line,.col=col};
    case '}': advance(lex); return (Token){.kind=TOK_RBRACE,.line=line,.col=col};
    case '[': advance(lex); return (Token){.kind=TOK_LBRACKET,.line=line,.col=col};
    case ']': advance(lex); return (Token){.kind=TOK_RBRACKET,.line=line,.col=col};
    case ',': advance(lex); return (Token){.kind=TOK_COMMA,.line=line,.col=col};
    case ':': advance(lex); return (Token){.kind=TOK_COLON,.line=line,.col=col};
    case '.': advance(lex); return (Token){.kind=TOK_DOT,.line=line,.col=col};
    case '+': advance(lex); return (Token){.kind=TOK_PLUS,.line=line,.col=col};
    case '-': advance(lex); return (Token){.kind=TOK_MINUS,.line=line,.col=col};
    case '*': advance(lex); return (Token){.kind=TOK_STAR,.line=line,.col=col};
    case '/': advance(lex); return (Token){.kind=TOK_SLASH,.line=line,.col=col};
    case '%': advance(lex); return (Token){.kind=TOK_MOD,.line=line,.col=col};

    // 双字符 token
    case '=':
        advance(lex);
        if (cur(lex) == '=') { advance(lex); return (Token){.kind=TOK_EQEQ,.line=line,.col=col}; }
        return (Token){.kind=TOK_ASSIGN,.line=line,.col=col};
    case '!':
        advance(lex);
        if (cur(lex) == '=') { advance(lex); return (Token){.kind=TOK_NEQ,.line=line,.col=col}; }
        return (Token){.kind=TOK_ERROR,.line=line,.col=col,.lexeme="expected '=' after '!'"};
    case '<':
        advance(lex);
        if (cur(lex) == '=') { advance(lex); return (Token){.kind=TOK_LE,.line=line,.col=col}; }
        return (Token){.kind=TOK_LT,.line=line,.col=col};
    case '>':
        advance(lex);
        if (cur(lex) == '=') { advance(lex); return (Token){.kind=TOK_GE,.line=line,.col=col}; }
        return (Token){.kind=TOK_GT,.line=line,.col=col};

    case '"': return read_string(lex);
    }

    // 数字
    if (isdigit(c)) return read_number(lex);

    // 标识符
    if (isalpha(c) || c == '_') return read_ident(lex);

    // 非法字符
    advance(lex);
    return (Token){.kind=TOK_ERROR,.line=line,.col=col,.lexeme="illegal character"};
}

Token lexer_peek(Lexer *lex) {
    if (!lex->has_peek) {
        lex->peek = lexer_next(lex);
        lex->has_peek = 1;
    }
    return lex->peek;
}

// ---- 生命周期 ----

Lexer *lexer_new(const char *source) {
    Lexer *lex = calloc(1, sizeof(Lexer));
    lex->src = source;
    lex->pos = source;
    lex->line = 1;
    lex->col = 1;
    return lex;
}

void lexer_free(Lexer *lex) {
    if (!lex) return;
    // 清理 peek 缓存中的动态内存
    if (lex->has_peek) {
        if (lex->peek.kind == TOK_STRING) free(lex->peek.sval);
        if (lex->peek.kind == TOK_IDENT || lex->peek.kind == TOK_INT ||
            lex->peek.kind == TOK_FLOAT) free(lex->peek.lexeme);
    }
    free(lex);
}

const char *token_kind_name(TokenKind kind) {
    static const char *names[] = {
        [TOK_IDENT]="IDENT",[TOK_STRING]="STRING",[TOK_INT]="INT",[TOK_FLOAT]="FLOAT",
        [TOK_PRINT]="print",[TOK_IF]="if",[TOK_ELIF]="elif",[TOK_ELSE]="else",
        [TOK_WHILE]="while",[TOK_DEF]="def",[TOK_RETURN]="return",
        [TOK_AND]="and",[TOK_OR]="or",[TOK_NOT]="not",
        [TOK_TRUE]="True",[TOK_FALSE]="False",[TOK_NONE]="None",
        [TOK_LPAREN]="(",[TOK_RPAREN]=")",[TOK_LBRACE]="{",[TOK_RBRACE]="}",
        [TOK_LBRACKET]="[",[TOK_RBRACKET]="]",
        [TOK_COMMA]=",",[TOK_COLON]=":",[TOK_DOT]=".",
        [TOK_PLUS]="+",[TOK_MINUS]="-",[TOK_STAR]="*",[TOK_SLASH]="/",[TOK_MOD]="%",
        [TOK_ASSIGN]="=",[TOK_EQEQ]="==",[TOK_NEQ]="!=",
        [TOK_LT]="<",[TOK_GT]=">",[TOK_LE]="<=",[TOK_GE]=">=",
        [TOK_NEWLINE]="NEWLINE",[TOK_EOF]="EOF",[TOK_ERROR]="ERROR",
    };
    return names[kind];
}
