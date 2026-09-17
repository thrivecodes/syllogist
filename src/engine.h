#ifndef SYLLOGIST_ENGINE_H
#define SYLLOGIST_ENGINE_H

#include "fact.h"
#include "rule.h"
#include "io.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ConflictStrategy {
    STRATEGY_ORDER,
    STRATEGY_SPECIFICITY,
    STRATEGY_RECENCY,
    STRATEGY_SALIENCE
} ConflictStrategy;

const char* conflict_strategy_to_string(ConflictStrategy s);

typedef enum EngineStepResult {
    ENGINE_STEP_FIRED,
    ENGINE_STEP_QUIESCENT,
    ENGINE_STEP_MAX_CYCLES,
    ENGINE_STEP_ERROR
} EngineStepResult;

typedef struct RuleFiringRecord {
    char *rule_name;
    bool has_fired;
    uint64_t last_fired_timestamp;
    struct RuleFiringRecord *next;
} RuleFiringRecord;

typedef struct Engine {
    WorkingMemory wm;
    RuleBase rb;
    ConflictStrategy strategy;
    size_t max_cycles; /* default 1000 */
    size_t cycle_count;
    size_t rules_fired_count;
    ActionSinkFn action_sink;
    void *action_user_data;
    RuleFiringRecord *firing_history;
} Engine;

/* Lifecycle and configuration */
void engine_init(Engine *e, ConflictStrategy strategy);
void engine_free(Engine *e);
void engine_set_strategy(Engine *e, ConflictStrategy strategy);
void engine_set_max_cycles(Engine *e, size_t max_cycles);
void engine_set_action_sink(Engine *e, ActionSinkFn sink, void *user_data);

/* Execution */
EngineStepResult engine_step(Engine *e);
EngineStepResult engine_run(Engine *e);

/* Diagnostics */
void engine_dump(const Engine *e, FILE *out);

#ifdef __cplusplus
}
#endif

#endif /* SYLLOGIST_ENGINE_H */
