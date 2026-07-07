#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interpreter.h"

// ---- 前向声明 ----
static Value eval_node(Env *env, ASTNode *node, int *has_return);

// ---- 二元运算 ----

static Value eval_binary(int op, Value lhs, Value rhs) {
    // 字符串拼接
    if (lhs.kind == VAL_STRING && rhs.kind == VAL_STRING && op == TOK_PLUS) {
        char *buf = malloc(strlen(lhs.sval) + strlen(rhs.sval) + 1);
        strcpy(buf, lhs.sval);
        strcat(buf, rhs.sval);
        Value v = val_string(buf);
        free(buf);
        return v;
    }

    // 数值提升：有 float 就把两边都当 float
    if (lhs.kind == VAL_INT && rhs.kind == VAL_INT) {
        long a = lhs.ival, b = rhs.ival;
        switch (op) {
        case TOK_PLUS:  return val_int(a + b);
        case TOK_MINUS: return val_int(a - b);
        case TOK_STAR:  return val_int(a * b);
        case TOK_SLASH: return b != 0 ? val_int(a / b) : val_int(0);
        case TOK_MOD:   return b != 0 ? val_int(a % b) : val_int(0);
        case TOK_EQEQ:  return val_bool(a == b);
        case TOK_NEQ:   return val_bool(a != b);
        case TOK_LT:    return val_bool(a < b);
        case TOK_GT:    return val_bool(a > b);
        case TOK_LE:    return val_bool(a <= b);
        case TOK_GE:    return val_bool(a >= b);
        default: break;
        }
    }

    // float 路径
    double a = lhs.kind == VAL_FLOAT ? lhs.fval : (double)lhs.ival;
    double b = rhs.kind == VAL_FLOAT ? rhs.fval : (double)rhs.ival;
    switch (op) {
    case TOK_PLUS:  return val_float(a + b);
    case TOK_MINUS: return val_float(a - b);
    case TOK_STAR:  return val_float(a * b);
    case TOK_SLASH: return val_float(a / b);
    case TOK_EQEQ:  return val_bool(a == b);
    case TOK_NEQ:   return val_bool(a != b);
    case TOK_LT:    return val_bool(a < b);
    case TOK_GT:    return val_bool(a > b);
    case TOK_LE:    return val_bool(a <= b);
    case TOK_GE:    return val_bool(a >= b);
    default: break;
    }
    return val_none();
}

// ---- 函数调用 ----

static Value call_function(Env *caller_env, Value func, ASTNode **args, int arg_count) {
    // 求值实参
    Value *eval_args = malloc(arg_count * sizeof(Value));
    int dummy = 0;
    for (int i = 0; i < arg_count; i++)
        eval_args[i] = eval_node(caller_env, args[i], &dummy);

    Value result;

    if (func.kind == VAL_BUILTIN) {
        result = func.builtin(eval_args, arg_count, caller_env);
    }
    else if (func.kind == VAL_FUNC) {
        Env *func_env = env_new(func.func.closure);
        for (int i = 0; i < func.func.param_count && i < arg_count; i++)
            env_def(func_env, func.func.params[i], eval_args[i]);
        // 多余实参忽略，不足的填 None（临时方案）
        for (int i = arg_count; i < func.func.param_count; i++)
            env_def(func_env, func.func.params[i], val_none());

        int returned = 0;
        result = eval_node(func_env, func.func.body, &returned);
        env_free(func_env);
    }
    else {
        fprintf(stderr, "Error: value is not callable\n");
        result = val_none();
    }

    free(eval_args);
    return result;
}

// ---- 核心求值 ----

static Value eval_node(Env *env, ASTNode *node, int *has_return) {
    if (!node) return val_none();

    switch (node->kind) {

    // ---- 字面量 ----
    case AST_INT:    return val_int(node->ival);
    case AST_FLOAT:  return val_float(node->fval);
    case AST_STRING: return val_string(node->sval);
    case AST_BOOL:   return val_bool(node->bval);
    case AST_NIL:    return val_none();

    // ---- 变量引用 ----
    case AST_IDENT:
        return env_get(env, node->ident_name);

    // ---- 一元运算 ----
    case AST_UNARY: {
        Value operand = eval_node(env, node->unary.expr, has_return);
        if (has_return && *has_return) return operand;
        if (node->unary.op == TOK_MINUS) {
            if (operand.kind == VAL_INT)   return val_int(-operand.ival);
            if (operand.kind == VAL_FLOAT) return val_float(-operand.fval);
        }
        if (node->unary.op == TOK_NOT)
            return val_bool(!val_is_truthy(operand));
        return val_none();
    }

    // ---- 二元运算 ----
    case AST_BINARY: {
        Value lhs = eval_node(env, node->binary.lhs, has_return);
        if (has_return && *has_return) return lhs;
        // 短路求值：and / or
        if (node->binary.op == TOK_AND)
            return val_is_truthy(lhs) ? eval_node(env, node->binary.rhs, has_return) : lhs;
        if (node->binary.op == TOK_OR)
            return val_is_truthy(lhs) ? lhs : eval_node(env, node->binary.rhs, has_return);
        Value rhs = eval_node(env, node->binary.rhs, has_return);
        if (has_return && *has_return) return rhs;
        return eval_binary(node->binary.op, lhs, rhs);
    }

    // ---- 函数调用 ----
    case AST_CALL: {
        Value func = env_get(env, node->call.name);
        return call_function(env, func, node->call.args, node->call.arg_count);
    }

    // ---- 赋值 ----
    case AST_ASSIGN: {
        Value val = eval_node(env, node->assign.value, has_return);
        if (has_return && *has_return) return val;
        env_set(env, node->assign.name, val);
        return val;
    }

    // ---- if / elif / else ----
    case AST_IF: {
        Value cond = eval_node(env, node->if_stmt.cond, has_return);
        if (has_return && *has_return) return cond;
        if (val_is_truthy(cond))
            return eval_node(env, node->if_stmt.then_body, has_return);
        else if (node->if_stmt.else_body)
            return eval_node(env, node->if_stmt.else_body, has_return);
        return val_none();
    }

    // ---- while ----
    case AST_WHILE: {
        Value last = val_none();
        while (1) {
            Value cond = eval_node(env, node->while_stmt.cond, has_return);
            if (has_return && *has_return) return cond;
            if (!val_is_truthy(cond)) break;
            last = eval_node(env, node->while_stmt.body, has_return);
            if (has_return && *has_return) return last;
        }
        return last;
    }

    // ---- return ----
    case AST_RETURN: {
        Value v = node->ret.expr ? eval_node(env, node->ret.expr, has_return) : val_none();
        if (has_return) *has_return = 1;
        return v;
    }

    // ---- 语句块 ----
    case AST_BLOCK: {
        Value last = val_none();
        for (int i = 0; i < node->block.count; i++) {
            last = eval_node(env, node->block.items[i], has_return);
            if (has_return && *has_return) return last;
        }
        return last;
    }

    // ---- 根节点 ----
    case AST_PROGRAM: {
        Value last = val_none();
        for (int i = 0; i < node->program.count; i++) {
            last = eval_node(env, node->program.items[i], has_return);
            if (has_return && *has_return) return last;
        }
        return last;
    }

    // ---- 函数定义 ----
    case AST_FUNC_DEF: {
        Value f = val_func(node->func_def.params, node->func_def.param_count,
                           node->func_def.body, env);
        env_def(env, node->func_def.name, f);
        return f;
    }

    default: return val_none();
    }
}

// ---- 公共入口 ----

void interp_eval(Env *env, ASTNode *node) {
    int returned = 0;
    eval_node(env, node, &returned);
}

// ---- 内置函数实现 ----

static Value builtin_print(Value *args, int argc, Env *env) {
    (void)env;
    for (int i = 0; i < argc; i++) {
        if (i > 0) printf(" ");
        val_print(args[i]);
    }
    printf("\n");
    return val_none();
}

static Value builtin_input(Value *args, int argc, Env *env) {
    (void)env;
    if (argc > 0) val_print(args[0]);
    char buf[4096];
    if (fgets(buf, sizeof(buf), stdin)) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
        return val_string(buf);
    }
    return val_string("");
}

static Value builtin_len(Value *args, int argc, Env *env) {
    (void)env;
    if (argc < 1) return val_int(0);
    if (args[0].kind == VAL_STRING) return val_int((long)strlen(args[0].sval));
    return val_int(0);
}

static Value builtin_int(Value *args, int argc, Env *env) {
    (void)env;
    if (argc < 1) return val_int(0);
    switch (args[0].kind) {
    case VAL_INT:   return args[0];
    case VAL_FLOAT: return val_int((long)args[0].fval);
    case VAL_STRING:return val_int(strtol(args[0].sval, NULL, 10));
    case VAL_BOOL:  return val_int(args[0].bval ? 1 : 0);
    default:        return val_int(0);
    }
}

static Value builtin_str(Value *args, int argc, Env *env) {
    (void)env;
    if (argc < 1) return val_string("");
    switch (args[0].kind) {
    case VAL_INT: {
        char buf[32];
        snprintf(buf, sizeof(buf), "%ld", args[0].ival);
        return val_string(buf);
    }
    case VAL_FLOAT: {
        char buf[32];
        snprintf(buf, sizeof(buf), "%g", args[0].fval);
        return val_string(buf);
    }
    case VAL_STRING: return args[0];
    case VAL_BOOL:   return val_string(args[0].bval ? "True" : "False");
    case VAL_NONE:   return val_string("None");
    default:         return val_string("");
    }
}

static Value builtin_type(Value *args, int argc, Env *env) {
    (void)env;
    if (argc < 1) return val_string("NoneType");
    printf("<%s>\n", val_kind_name(args[0].kind));
    return val_none();
}

void interp_register_builtins(Env *env) {
    env_set(env, "print", val_builtin(builtin_print));
    env_set(env, "input", val_builtin(builtin_input));
    env_set(env, "len",   val_builtin(builtin_len));
    env_set(env, "int",   val_builtin(builtin_int));
    env_set(env, "str",   val_builtin(builtin_str));
    env_set(env, "type",  val_builtin(builtin_type));
}
