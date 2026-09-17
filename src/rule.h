#ifndef SYLLOGIST_RULE_H
#define SYLLOGIST_RULE_H

#include "fact.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Operator {
    OP_EQ,
    OP_NEQ,
    OP_LT,
    OP_LTE,
    OP_GT,
    OP_GTE,
    OP_EXISTS
} Operator;

const char* operator_to_string(Operator op);

typedef struct Condition {
    char *fact_name;
    Operator op;
    Value target_val;
    struct Condition *next;
} Condition;

Condition* condition_create(const char *fact_name, Operator op, Value target_val);
void condition_free(Condition *c);
bool condition_eval(const Condition *c, const Fact *f);

typedef enum ActionType {
    ACT_ASSERT,
    ACT_RETRACT,
    ACT_CUSTOM
} ActionType;

const char* action_type_to_string(ActionType type);

typedef struct Action {
    ActionType type;
    char *target_name;
    Value arg_val;
    struct Action *next;
} Action;

Action* action_create(ActionType type, const char *target_name, Value arg_val);
void action_free(Action *a);

typedef struct Rule {
    char *name;
    int salience;
    Condition *conditions;
    size_t condition_count;
    Action *actions;
    size_t action_count;
    struct Rule *next;
} Rule;

Rule* rule_create(const char *name, int salience);
void rule_free(Rule *r);
void rule_add_condition(Rule *r, const char *fact_name, Operator op, Value target_val);
void rule_add_action(Rule *r, ActionType type, const char *target_name, Value arg_val);
bool rule_matches(const Rule *r, const WorkingMemory *wm, uint64_t *out_max_timestamp);

typedef struct RuleBase {
    Rule *head;
    size_t count;
} RuleBase;

void rb_init(RuleBase *rb);
void rb_free(RuleBase *rb);
void rb_add_rule(RuleBase *rb, Rule *rule);
Rule* rb_find_rule(const RuleBase *rb, const char *name);
size_t rb_count(const RuleBase *rb);

#ifdef __cplusplus
}
#endif

#endif /* SYLLOGIST_RULE_H */
