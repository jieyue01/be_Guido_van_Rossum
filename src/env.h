#ifndef ENV_H
#define ENV_H

#include "value.h"

#define ENV_INIT_CAP 16

// 单个变量绑定
typedef struct {
    char  *name;
    Value value;
} Binding;

// 作用域（符号表），链式嵌套
typedef struct Env {
    Binding *slots;       // 哈希表数组
    int      count;        // 已用槽位
    int      cap;          // 总容量
    struct Env *parent;   // 上级作用域（NULL = 全局）
} Env;

Env   *env_new(Env *parent);
void   env_free(Env *env);

// 在当前作用域插入/更新变量
void   env_set(Env *env, const char *name, Value val);
// 沿作用域链向上查找（找不到返回 VAL_NONE）
Value  env_get(Env *env, const char *name);
// 在当前作用域定义（用于 def，不查上级，不覆盖上级）
void   env_def(Env *env, const char *name, Value val);

#endif
