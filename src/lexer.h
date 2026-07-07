#ifndef LEXER_H
#define LEXER_H

#include "token.h"

typedef struct {
    const char *src;     // 源码字符串
    const char *pos;     // 当前字符指针
    int line;            // 当前行号
    int col;             // 当前列号
    Token peek;          // 预读一个 token
    int has_peek;        // peek 是否有效
} Lexer;

Lexer *lexer_new(const char *source);
Token  lexer_next(Lexer *lex);    // 取下一个 token
Token  lexer_peek(Lexer *lex);    // 预读不消耗
void   lexer_free(Lexer *lex);

#endif
