#include "rule.h"
#include <stdlib.h>
#include <string.h>

static char *safe_strdup(const char *s) {
    if (!s) {
        char *empty = (char *)malloc(1);
        if (empty) {
            empty[0] = '\0';
        }
        return empty;
    }
    size_t len = strlen(s);
    char *copy = (char *)malloc(len + 1);
    if (copy) {
        memcpy(copy, s, len + 1);
    }
    return copy;
}

const char* operator_to_string(Operator op) {
    switch (op) {
        case OP_EQ:     return "==";
        case OP_NEQ:    return "!=";
        case OP_LT:     return "<";
        case OP_LTE:    return "<=";
        case OP_GT:     return ">";
        case OP_GTE:    return ">=";
        case OP_EXISTS: return "exists";
        default:        return "unknown";
    }
}

Condition* condition_create(const char *fact_name, Operator op, Value target_val) {
    Condition *c = (Condition *)malloc(sizeof(Condition));
    if (!c) {
        value_free(&target_val);
        return NULL;
    }
    c->fact_name = safe_strdup(fact_name);
    c->op = op;
    c->target_val = target_val;
    c->next = NULL;
    return c;
}

void condition_free(Condition *c) {
    while (c) {
        Condition *next = c->next;
        free(c->fact_name);
        value_free(&c->target_val);
        free(c);
        c = next;
    }
}

bool condition_eval(const Condition *c, const Fact *f) {
    if (!c || !f) {
        return false;
    }
    if (c->fact_name && f->name && strcmp(c->fact_name, f->name) != 0) {
        return false;
    }
    switch (c->op) {
        case OP_EQ:
            return value_equal(&f->value, &c->target_val);
        case OP_NEQ:
            return !value_equal(&f->value, &c->target_val);
        case OP_LT: {
            bool comp = false;
            int cmp = value_compare(&f->value, &c->target_val, &comp);
            return comp && (cmp < 0);
        }
        case OP_LTE: {
            bool comp = false;
            int cmp = value_compare(&f->value, &c->target_val, &comp);
            return comp && (cmp <= 0);
        }
        case OP_GT: {
            bool comp = false;
            int cmp = value_compare(&f->value, &c->target_val, &comp);
            return comp && (cmp > 0);
        }
        case OP_GTE: {
            bool comp = false;
            int cmp = value_compare(&f->value, &c->target_val, &comp);
            return comp && (cmp >= 0);
        }
        case OP_EXISTS:
            return true;
        default:
            return false;
    }
}

const char* action_type_to_string(ActionType type) {
    switch (type) {
        case ACT_ASSERT:  return "ASSERT";
        case ACT_RETRACT: return "RETRACT";
        case ACT_CUSTOM:  return "CUSTOM";
        default:          return "UNKNOWN";
    }
}

Action* action_create(ActionType type, const char *target_name, Value arg_val) {
    Action *a = (Action *)malloc(sizeof(Action));
    if (!a) {
        value_free(&arg_val);
        return NULL;
    }
    a->type = type;
    a->target_name = safe_strdup(target_name);
    a->arg_val = arg_val;
    a->next = NULL;
    return a;
}

void action_free(Action *a) {
    while (a) {
        Action *next = a->next;
        free(a->target_name);
        value_free(&a->arg_val);
        free(a);
        a = next;
    }
}

Rule* rule_create(const char *name, int salience) {
    Rule *r = (Rule *)malloc(sizeof(Rule));
    if (!r) {
        return NULL;
    }
    r->name = safe_strdup(name);
    r->salience = salience;
    r->conditions = NULL;
    r->condition_count = 0;
    r->actions = NULL;
    r->action_count = 0;
    r->next = NULL;
    return r;
}

void rule_free(Rule *r) {
    if (!r) {
        return;
    }
    free(r->name);
    condition_free(r->conditions);
    action_free(r->actions);
    free(r);
}

void rule_add_condition(Rule *r, const char *fact_name, Operator op, Value target_val) {
    if (!r) {
        value_free(&target_val);
        return;
    }
    Condition *cond = condition_create(fact_name, op, target_val);
    if (!cond) {
        return;
    }

    if (!r->conditions) {
        r->conditions = cond;
    } else {
        Condition *curr = r->conditions;
        while (curr->next) {
            curr = curr->next;
        }
        curr->next = cond;
    }
    r->condition_count++;
}

void rule_add_action(Rule *r, ActionType type, const char *target_name, Value arg_val) {
    if (!r) {
        value_free(&arg_val);
        return;
    }
    Action *act = action_create(type, target_name, arg_val);
    if (!act) {
        return;
    }

    if (!r->actions) {
        r->actions = act;
    } else {
        Action *curr = r->actions;
        while (curr->next) {
            curr = curr->next;
        }
        curr->next = act;
    }
    r->action_count++;
}

bool rule_matches(const Rule *r, const WorkingMemory *wm, uint64_t *out_max_timestamp) {
    if (!r || !wm) {
        return false;
    }

    uint64_t max_ts = 0;
    const Condition *c = r->conditions;
    while (c) {
        const Fact *f = wm_find(wm, c->fact_name);
        if (!f) {
            return false;
        }
        if (!condition_eval(c, f)) {
            return false;
        }
        if (f->timestamp > max_ts) {
            max_ts = f->timestamp;
        }
        c = c->next;
    }

    if (out_max_timestamp) {
        *out_max_timestamp = max_ts;
    }
    return true;
}

void rb_init(RuleBase *rb) {
    if (!rb) {
        return;
    }
    rb->head = NULL;
    rb->count = 0;
}

void rb_free(RuleBase *rb) {
    if (!rb) {
        return;
    }
    Rule *curr = rb->head;
    while (curr) {
        Rule *next = curr->next;
        rule_free(curr);
        curr = next;
    }
    rb->head = NULL;
    rb->count = 0;
}

void rb_add_rule(RuleBase *rb, Rule *rule) {
    if (!rb || !rule) {
        return;
    }
    rule->next = NULL;
    if (!rb->head) {
        rb->head = rule;
    } else {
        Rule *curr = rb->head;
        while (curr->next) {
            curr = curr->next;
        }
        curr->next = rule;
    }
    rb->count++;
}

Rule* rb_find_rule(const RuleBase *rb, const char *name) {
    if (!rb || !name) {
        return NULL;
    }
    Rule *curr = rb->head;
    while (curr) {
        if (curr->name && strcmp(curr->name, name) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

size_t rb_count(const RuleBase *rb) {
    return rb ? rb->count : 0;
}
