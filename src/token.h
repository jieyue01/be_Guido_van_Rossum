#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
    // 字面量
    TOK_IDENT,       // 变量名/函数名
    TOK_STRING,      // "hello"
    TOK_INT,         // 42
    TOK_FLOAT,       // 3.14

    // 关键字
    TOK_PRINT,       // print
    TOK_IF,          // if
    TOK_ELIF,        // elif
    TOK_ELSE,        // else
    TOK_WHILE,       // while
    TOK_DEF,         // def
    TOK_RETURN,      // return
    TOK_AND,         // and
    TOK_OR,          // or
    TOK_NOT,         // not
    TOK_TRUE,        // True
    TOK_FALSE,       // False
    TOK_NONE,        // None

    // 括号/分隔符
    TOK_LPAREN,      // (
    TOK_RPAREN,      // )
    TOK_LBRACE,      // {
    TOK_RBRACE,      // }
    TOK_LBRACKET,    // [    用于 list/dict 字面量
    TOK_RBRACKET,    // ]
    TOK_COMMA,       // ,
    TOK_COLON,       // :    用于 dict key:value 和切片
    TOK_DOT,         // .

    // 运算符
    TOK_PLUS,        // +
    TOK_MINUS,       // -
    TOK_STAR,        // *
    TOK_SLASH,       // /
    TOK_MOD,         // %
    TOK_ASSIGN,      // =
    TOK_EQEQ,        // ==
    TOK_NEQ,         // !=
    TOK_LT,          // <
    TOK_GT,          // >
    TOK_LE,          // <=
    TOK_GE,          // >=

    // 特殊
    TOK_NEWLINE,     // 换行（语句分隔符）
    TOK_EOF,         // 文件结束
    TOK_ERROR,       // 非法字符
} TokenKind;

typedef struct {
    TokenKind kind;
    char *lexeme;    // 原始字符串
    int line;        // 行号
    int col;         // 列号
    union {
        long ival;
        double fval;
        char *sval;  // 指向 lexeme 内部，不单独分配
    };
} Token;

const char *token_kind_name(TokenKind kind);

#endif
