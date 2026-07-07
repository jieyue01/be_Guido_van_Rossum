#ifndef VALUE_H
#define VALUE_H

#include <stdbool.h>

// 值类型枚举——加新类型只需在这里加一行
typedef enum {
    VAL_NONE,
    VAL_INT,
    VAL_FLOAT,
    VAL_STRING,
    VAL_BOOL,
    VAL_FUNC,     // 用户自定义函数
    VAL_BUILTIN,  // C 函数指针（内置函数）
    // 后续扩展：
    // VAL_LIST,
    // VAL_TUPLE,
    // VAL_DICT,
} ValueKind;

struct ASTNode;
struct Env;

// 内置函数的 C 签名：接收参数数组和参数个数，返回 Value
typedef struct Value (*BuiltinFn)(struct Value *args, int argc, struct Env *env);

// 用户自定义函数
typedef struct {
    char **params;
    int param_count;
    struct ASTNode *body;    // 函数体的 AST
    struct Env *closure;     // 闭包捕获的外部环境
} FuncValue;

typedef struct Value {
    ValueKind kind;
    union {
        long          ival;
        double        fval;
        char         *sval;   // 字符串（owning pointer）
        bool          bval;
        FuncValue     func;
        BuiltinFn     builtin;
        // 后续：
        // struct { struct Value *items; int len; int cap; } list;
        // struct { struct Value *items; int len; int cap; } tuple;
        // struct Dict *dict;
    };
} Value;

// ---- 构造函数 ----
Value val_none(void);
Value val_int(long n);
Value val_float(double f);
Value val_string(const char *s);   // 内部 strdup
Value val_bool(bool b);
Value val_func(char **params, int n, struct ASTNode *body, struct Env *closure);
Value val_builtin(BuiltinFn fn);

// ---- 工具函数 ----
const char *val_kind_name(ValueKind kind);
void val_free(Value *v);           // 释放 owned 内存（string/func 等）
void val_print(Value v);           // 调试打印
bool val_is_truthy(Value v);       // None/False/0/"→ false，其余→ true
bool val_eq(Value a, Value b);     // == 比较

#endif
