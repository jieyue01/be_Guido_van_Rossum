#include <stdlib.h>
#include <string.h>
#include "env.h"
#include "ast.h"

// FNV-1a hash
static unsigned long hash_str(const char *s) {
    unsigned long long h = 14695981039346656037ULL;
    while (*s) {
        h ^= (unsigned char)*s;
        h *= 1099511628211ULL;
        s++;
    }
    return (unsigned long)h;
}

Env *env_new(Env *parent) {
    Env *env = calloc(1, sizeof(Env));
    env->cap = ENV_INIT_CAP;
    env->slots = calloc(env->cap, sizeof(Binding));
    env->parent = parent;
    return env;
}

void env_free(Env *env) {
    if (!env) return;
    for (int i = 0; i < env->cap; i++) {
        if (env->slots[i].name) {
            free(env->slots[i].name);
            val_free(&env->slots[i].value);
        }
    }
    free(env->slots);
    free(env);
}

static Binding *env_find_slot(Env *env, const char *name) {
    unsigned long h = hash_str(name);
    for (int i = 0; i < env->cap; i++) {
        int idx = (h + i) % env->cap;
        Binding *b = &env->slots[idx];
        if (!b->name) return b;  // 空槽，未找到
        if (strcmp(b->name, name) == 0) return b;
    }
    return NULL;  // 表满了
}

static void env_grow(Env *env) {
    Binding *old = env->slots;
    int old_cap = env->cap;
    env->cap *= 2;
    env->slots = calloc(env->cap, sizeof(Binding));

    for (int i = 0; i < old_cap; i++) {
        if (old[i].name) {
            Binding *dst = env_find_slot(env, old[i].name);
            *dst = old[i];
        }
    }
    free(old);
}

void env_set(Env *env, const char *name, Value val) {
    // 先沿链查找，存在就原地更新
    for (Env *e = env; e; e = e->parent) {
        unsigned long h = hash_str(name);
        for (int i = 0; i < e->cap; i++) {
            int idx = (h + i) % e->cap;
            Binding *b = &e->slots[idx];
            if (!b->name) break;
            if (strcmp(b->name, name) == 0) {
                val_free(&b->value);
                b->value = val;
                return;
            }
        }
    }
    // 不存在，在当前作用域创建
    if (env->count >= env->cap / 2) env_grow(env);
    Binding *slot = env_find_slot(env, name);
    if (!slot) return;
    slot->name = strdup(name);
    slot->value = val;
    env->count++;
}

void env_def(Env *env, const char *name, Value val) {
    // 强制在当前作用域定义
    if (env->count >= env->cap / 2) env_grow(env);
    Binding *slot = env_find_slot(env, name);
    if (!slot) return;
    if (slot->name) {
        val_free(&slot->value);
        slot->value = val;
        return;
    }
    slot->name = strdup(name);
    slot->value = val;
    env->count++;
}

Value env_get(Env *env, const char *name) {
    for (Env *e = env; e; e = e->parent) {
        unsigned long h = hash_str(name);
        for (int i = 0; i < e->cap; i++) {
            int idx = (h + i) % e->cap;
            Binding *b = &e->slots[idx];
            if (!b->name) break;
            if (strcmp(b->name, name) == 0) return b->value;
        }
    }
    return (Value){.kind = VAL_NONE};
}
