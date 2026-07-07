#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "value.h"

// ---- 构造函数 ----
Value val_none(void) {
    return (Value){.kind = VAL_NONE};
}

Value val_int(long n) {
    Value v = {.kind = VAL_INT};
    v.ival = n;
    return v;
}

Value val_float(double f) {
    Value v = {.kind = VAL_FLOAT};
    v.fval = f;
    return v;
}

Value val_string(const char *s) {
    Value v = {.kind = VAL_STRING};
    v.sval = strdup(s);
    return v;
}

Value val_bool(bool b) {
    Value v = {.kind = VAL_BOOL};
    v.bval = b;
    return v;
}

Value val_func(char **params, int n, struct ASTNode *body, struct Env *closure) {
    Value v = {.kind = VAL_FUNC};
    v.func.params = params;
    v.func.param_count = n;
    v.func.body = body;
    v.func.closure = closure;
    return v;
}

Value val_builtin(BuiltinFn fn) {
    Value v = {.kind = VAL_BUILTIN};
    v.builtin = fn;
    return v;
}

// ---- 工具 ----
void val_free(Value *v) {
    if (!v) return;
    switch (v->kind) {
    case VAL_STRING:
        free(v->sval);
        break;
    case VAL_FUNC:
        for (int i = 0; i < v->func.param_count; i++)
            free(v->func.params[i]);
        free(v->func.params);
        ast_free(v->func.body);
        break;
    default: break;
    }
    *v = (Value){.kind = VAL_NONE};
}

void val_print(Value v) {
    switch (v.kind) {
    case VAL_NONE:    printf("None"); break;
    case VAL_INT:     printf("%ld", v.ival); break;
    case VAL_FLOAT:   printf("%g", v.fval); break;
    case VAL_STRING:  printf("%s", v.sval); break;
    case VAL_BOOL:    printf("%s", v.bval ? "True" : "False"); break;
    case VAL_FUNC:    printf("<function %p>", (void *)&v); break;
    case VAL_BUILTIN: printf("<builtin>"); break;
    }
}

bool val_is_truthy(Value v) {
    switch (v.kind) {
    case VAL_NONE:    return false;
    case VAL_INT:     return v.ival != 0;
    case VAL_FLOAT:   return v.fval != 0.0;
    case VAL_STRING:  return strlen(v.sval) > 0;
    case VAL_BOOL:    return v.bval;
    case VAL_FUNC:
    case VAL_BUILTIN: return true;
    }
    return false;
}

bool val_eq(Value a, Value b) {
    if (a.kind != b.kind) return false;
    switch (a.kind) {
    case VAL_NONE:    return true;
    case VAL_INT:     return a.ival == b.ival;
    case VAL_FLOAT:   return a.fval == b.fval;
    case VAL_STRING:  return strcmp(a.sval, b.sval) == 0;
    case VAL_BOOL:    return a.bval == b.bval;
    case VAL_FUNC:    return &a == &b;
    case VAL_BUILTIN: return a.builtin == b.builtin;
    }
    return false;
}

const char *val_kind_name(ValueKind kind) {
    static const char *names[] = {
        [VAL_NONE]    = "NoneType",
        [VAL_INT]     = "int",
        [VAL_FLOAT]   = "float",
        [VAL_STRING]  = "str",
        [VAL_BOOL]    = "bool",
        [VAL_FUNC]    = "function",
        [VAL_BUILTIN] = "builtin",
    };
    return names[kind];
}
