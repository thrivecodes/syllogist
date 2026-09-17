#ifndef SYLLOGIST_FACT_H
#define SYLLOGIST_FACT_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ValueType {
    VAL_NONE,
    VAL_INT,
    VAL_FLOAT,
    VAL_STRING,
    VAL_BOOL,
    VAL_SYMBOL
} ValueType;

typedef struct Value {
    ValueType type;
    union {
        int64_t i;
        double f;
        char *s;
        bool b;
        char *sym;
    } as;
} Value;

/* Value constructors and operations */
Value value_int(int64_t val);
Value value_float(double val);
Value value_string(const char *s);
Value value_bool(bool val);
Value value_symbol(const char *s);
Value value_none(void);
void value_free(Value *v);
Value value_clone(const Value *v);
bool value_equal(const Value *a, const Value *b);
int value_compare(const Value *a, const Value *b, bool *comparable);
void value_print(const Value *v, FILE *out);

typedef struct Fact {
    char *name;
    Value value;
    uint64_t timestamp; /* logical tick */
    struct Fact *next;
} Fact;

typedef struct WorkingMemory {
    Fact *head;
    size_t count;
    uint64_t clock; /* monotonic tick incremented on each assert/update */
} WorkingMemory;

/* WorkingMemory lifecycle and operations */
void wm_init(WorkingMemory *wm);
void wm_free(WorkingMemory *wm);
Fact* wm_assert(WorkingMemory *wm, const char *name, Value val);
bool wm_retract(WorkingMemory *wm, const char *name);
const Fact* wm_find(const WorkingMemory *wm, const char *name);
size_t wm_count(const WorkingMemory *wm);
void wm_dump(const WorkingMemory *wm, FILE *out);

#ifdef __cplusplus
}
#endif

#endif /* SYLLOGIST_FACT_H */
