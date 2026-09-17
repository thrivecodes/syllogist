#include "fact.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

Value value_int(int64_t val) {
    Value v;
    v.type = VAL_INT;
    v.as.i = val;
    return v;
}

Value value_float(double val) {
    Value v;
    v.type = VAL_FLOAT;
    v.as.f = val;
    return v;
}

Value value_string(const char *s) {
    Value v;
    v.type = VAL_STRING;
    v.as.s = syllogist_strdup(s);
    return v;
}

Value value_bool(bool val) {
    Value v;
    v.type = VAL_BOOL;
    v.as.b = val;
    return v;
}

Value value_symbol(const char *s) {
    Value v;
    v.type = VAL_SYMBOL;
    v.as.sym = syllogist_strdup(s);
    return v;
}

Value value_none(void) {
    Value v;
    v.type = VAL_NONE;
    v.as.i = 0;
    return v;
}

void value_free(Value *v) {
    if (!v) {
        return;
    }
    if (v->type == VAL_STRING) {
        free(v->as.s);
        v->as.s = NULL;
    } else if (v->type == VAL_SYMBOL) {
        free(v->as.sym);
        v->as.sym = NULL;
    }
    v->type = VAL_NONE;
    v->as.i = 0;
}

Value value_clone(const Value *v) {
    if (!v) {
        return value_none();
    }
    Value copy = *v;
    if (v->type == VAL_STRING) {
        copy.as.s = syllogist_strdup(v->as.s);
    } else if (v->type == VAL_SYMBOL) {
        copy.as.sym = syllogist_strdup(v->as.sym);
    }
    return copy;
}

bool value_equal(const Value *a, const Value *b) {
    if (!a && !b) {
        return true;
    }
    if (!a || !b) {
        return false;
    }

    if (a->type == b->type) {
        switch (a->type) {
            case VAL_NONE:
                return true;
            case VAL_INT:
                return a->as.i == b->as.i;
            case VAL_FLOAT:
                return a->as.f == b->as.f;
            case VAL_STRING:
                return strcmp(a->as.s ? a->as.s : "", b->as.s ? b->as.s : "") == 0;
            case VAL_BOOL:
                return a->as.b == b->as.b;
            case VAL_SYMBOL:
                return strcmp(a->as.sym ? a->as.sym : "", b->as.sym ? b->as.sym : "") == 0;
            default:
                return false;
        }
    }

    /* Cross-type int vs float numeric equality */
    if (a->type == VAL_INT && b->type == VAL_FLOAT) {
        return (double)a->as.i == b->as.f;
    }
    if (a->type == VAL_FLOAT && b->type == VAL_INT) {
        return a->as.f == (double)b->as.i;
    }

    return false;
}

int value_compare(const Value *a, const Value *b, bool *comparable) {
    if (!a || !b) {
        if (comparable) {
            *comparable = false;
        }
        return 0;
    }

    /* Int vs Int */
    if (a->type == VAL_INT && b->type == VAL_INT) {
        if (comparable) *comparable = true;
        if (a->as.i < b->as.i) return -1;
        if (a->as.i > b->as.i) return 1;
        return 0;
    }

    /* Float vs Float */
    if (a->type == VAL_FLOAT && b->type == VAL_FLOAT) {
        if (comparable) *comparable = true;
        if (a->as.f < b->as.f) return -1;
        if (a->as.f > b->as.f) return 1;
        return 0;
    }

    /* Int vs Float */
    if (a->type == VAL_INT && b->type == VAL_FLOAT) {
        if (comparable) *comparable = true;
        double da = (double)a->as.i;
        if (da < b->as.f) return -1;
        if (da > b->as.f) return 1;
        return 0;
    }

    /* Float vs Int */
    if (a->type == VAL_FLOAT && b->type == VAL_INT) {
        if (comparable) *comparable = true;
        double db = (double)b->as.i;
        if (a->as.f < db) return -1;
        if (a->as.f > db) return 1;
        return 0;
    }

    /* String vs String */
    if (a->type == VAL_STRING && b->type == VAL_STRING) {
        if (comparable) *comparable = true;
        int cmp = strcmp(a->as.s ? a->as.s : "", b->as.s ? b->as.s : "");
        if (cmp < 0) return -1;
        if (cmp > 0) return 1;
        return 0;
    }

    /* Symbol vs Symbol */
    if (a->type == VAL_SYMBOL && b->type == VAL_SYMBOL) {
        if (comparable) *comparable = true;
        int cmp = strcmp(a->as.sym ? a->as.sym : "", b->as.sym ? b->as.sym : "");
        if (cmp < 0) return -1;
        if (cmp > 0) return 1;
        return 0;
    }

    /* Bool vs Bool */
    if (a->type == VAL_BOOL && b->type == VAL_BOOL) {
        if (comparable) *comparable = true;
        if (a->as.b == b->as.b) return 0;
        return a->as.b ? 1 : -1;
    }

    /* None vs None */
    if (a->type == VAL_NONE && b->type == VAL_NONE) {
        if (comparable) *comparable = true;
        return 0;
    }

    /* Incomparable types */
    if (comparable) {
        *comparable = false;
    }
    return 0;
}

void value_print(const Value *v, FILE *out) {
    if (!out) {
        out = stdout;
    }
    if (!v) {
        fputs("none", out);
        return;
    }
    switch (v->type) {
        case VAL_NONE:
            fputs("none", out);
            break;
        case VAL_INT:
            fprintf(out, "%" PRId64, v->as.i);
            break;
        case VAL_FLOAT:
            fprintf(out, "%g", v->as.f);
            break;
        case VAL_STRING:
            fprintf(out, "\"%s\"", v->as.s ? v->as.s : "");
            break;
        case VAL_BOOL:
            fputs(v->as.b ? "true" : "false", out);
            break;
        case VAL_SYMBOL:
            fprintf(out, "%s", v->as.sym ? v->as.sym : "");
            break;
    }
}

void wm_init(WorkingMemory *wm) {
    if (!wm) {
        return;
    }
    wm->head = NULL;
    wm->count = 0;
    wm->clock = 0;
}

void wm_free(WorkingMemory *wm) {
    if (!wm) {
        return;
    }
    Fact *curr = wm->head;
    while (curr) {
        Fact *next = curr->next;
        free(curr->name);
        value_free(&curr->value);
        free(curr);
        curr = next;
    }
    wm->head = NULL;
    wm->count = 0;
    wm->clock = 0;
}

Fact* wm_assert(WorkingMemory *wm, const char *name, Value val) {
    if (!wm || !name) {
        value_free(&val);
        return NULL;
    }

    wm->clock++;

    /* Check if a fact with this name already exists */
    Fact *curr = wm->head;
    while (curr) {
        if (curr->name && strcmp(curr->name, name) == 0) {
            value_free(&curr->value);
            curr->value = val;
            curr->timestamp = wm->clock;
            return curr;
        }
        curr = curr->next;
    }

    /* Create and prepend new Fact */
    Fact *new_fact = (Fact *)malloc(sizeof(Fact));
    if (!new_fact) {
        value_free(&val);
        return NULL;
    }
    new_fact->name = syllogist_strdup(name);
    if (!new_fact->name) {
        value_free(&val);
        free(new_fact);
        return NULL;
    }
    new_fact->value = val;
    new_fact->timestamp = wm->clock;
    new_fact->next = wm->head;

    wm->head = new_fact;
    wm->count++;

    return new_fact;
}

bool wm_retract(WorkingMemory *wm, const char *name) {
    if (!wm || !name || !wm->head) {
        return false;
    }

    Fact *curr = wm->head;
    Fact *prev = NULL;
    while (curr) {
        if (curr->name && strcmp(curr->name, name) == 0) {
            if (prev) {
                prev->next = curr->next;
            } else {
                wm->head = curr->next;
            }
            wm->count--;
            free(curr->name);
            value_free(&curr->value);
            free(curr);
            return true;
        }
        prev = curr;
        curr = curr->next;
    }

    return false;
}

const Fact* wm_find(const WorkingMemory *wm, const char *name) {
    if (!wm || !name) {
        return NULL;
    }

    const Fact *curr = wm->head;
    while (curr) {
        if (strcmp(curr->name, name) == 0) {
            return curr;
        }
        curr = curr->next;
    }

    return NULL;
}

size_t wm_count(const WorkingMemory *wm) {
    return wm ? wm->count : 0;
}

void wm_dump(const WorkingMemory *wm, FILE *out) {
    if (!out) {
        out = stdout;
    }
    if (!wm) {
        fputs("=== Working Memory: (null) ===\n", out);
        return;
    }

    fprintf(out, "=== Working Memory (clock: %" PRIu64 ", count: %zu) ===\n",
            wm->clock, wm->count);
    const Fact *curr = wm->head;
    while (curr) {
        fprintf(out, "  [t=%" PRIu64 "] %s = ",
                curr->timestamp,
                curr->name ? curr->name : "(unnamed)");
        value_print(&curr->value, out);
        fputc('\n', out);
        curr = curr->next;
    }
    fputs("===============================================\n", out);
}
