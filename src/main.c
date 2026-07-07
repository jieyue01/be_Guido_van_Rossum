#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"

#define REPL_BUF_SIZE 4096

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Error: cannot open file '%s'\n", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(size + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    return buf;
}

static void run_source(const char *source) {
    Lexer *lex = lexer_new(source);
    Parser *p = parser_new(lex);
    ASTNode *program = parser_parse(p);

    if (parser_has_error(p)) {
        fprintf(stderr, "%s\n", parser_error(p));
    } else {
        Env *env = env_new(NULL);
        interp_register_builtins(env);
        interp_eval(env, program);
    }

    ast_free(program);
    parser_free(p);
    lexer_free(lex);
}

static char *repl_read_block(void) {
    char block[REPL_BUF_SIZE] = {0};
    int depth = 0;

    printf(">>> ");
    fflush(stdout);

    while (1) {
        char line[1024];
        if (!fgets(line, sizeof(line), stdin)) break;

        size_t len = strlen(block);
        size_t remain = REPL_BUF_SIZE - len - 1;
        if (strlen(line) >= remain) break;
        strcat(block, line);

        for (char *c = line; *c; c++) {
            if (*c == '{') depth++;
            else if (*c == '}') depth--;
        }

        if (depth <= 0) break;
        printf("... ");
        fflush(stdout);
    }

    return strdup(block);
}

int main(int argc, char **argv) {
    if (argc == 1) {
        printf("CPY v0.1 - Python-flavored interpreter\n");
        printf("Type 'exit()' to quit.\n\n");

        while (1) {
            char *input = repl_read_block();
            if (!input || strcmp(input, "exit()\n") == 0 || strcmp(input, "exit()") == 0) {
                free(input);
                break;
            }
            if (strlen(input) > 0) {
                run_source(input);
            }
            free(input);
        }
    } else if (argc == 2) {
        char *source = read_file(argv[1]);
        if (!source) return 1;
        run_source(source);
        free(source);
    } else {
        fprintf(stderr, "Usage: cpy [file.cpy]\n");
        return 1;
    }

    return 0;
}
