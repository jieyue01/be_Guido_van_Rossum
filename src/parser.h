#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"

typedef struct Parser Parser;

Parser   *parser_new(Lexer *lex);
ASTNode  *parser_parse(Parser *p);       // 返回 AST_PROGRAM
int       parser_has_error(Parser *p);
const char *parser_error(Parser *p);
void      parser_free(Parser *p);

#endif
