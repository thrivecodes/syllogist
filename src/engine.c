#include "engine.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* conflict_strategy_to_string(ConflictStrategy s) {
    switch (s) {
        case STRATEGY_ORDER:       return "ORDER";
        case STRATEGY_SPECIFICITY: return "SPECIFICITY";
        case STRATEGY_RECENCY:     return "RECENCY";
        case STRATEGY_SALIENCE:    return "SALIENCE";
        default:                   return "UNKNOWN";
    }
}

static RuleFiringRecord* find_firing_record(const Engine *e, const char *rule_name) {
    if (!e || !rule_name) {
        return NULL;
    }
    RuleFiringRecord *curr = e->firing_history;
    while (curr) {
        if (curr->rule_name && strcmp(curr->rule_name, rule_name) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

static void update_firing_record(Engine *e, const char *rule_name, uint64_t fired_ts) {
    if (!e || !rule_name) {
        return;
    }
    RuleFiringRecord *rec = find_firing_record(e, rule_name);
    if (rec) {
        rec->has_fired = true;
        rec->last_fired_timestamp = fired_ts;
        return;
    }
    RuleFiringRecord *new_rec = (RuleFiringRecord *)malloc(sizeof(RuleFiringRecord));
    if (!new_rec) {
        return;
    }
    new_rec->rule_name = syllogist_strdup(rule_name);
    if (!new_rec->rule_name) {
        free(new_rec);
        return;
    }
    new_rec->has_fired = true;
    new_rec->last_fired_timestamp = fired_ts;
    new_rec->next = e->firing_history;
    e->firing_history = new_rec;
}

static void free_firing_history(Engine *e) {
    if (!e) {
        return;
    }
    RuleFiringRecord *curr = e->firing_history;
    while (curr) {
        RuleFiringRecord *next = curr->next;
        free(curr->rule_name);
        free(curr);
        curr = next;
    }
    e->firing_history = NULL;
}

static bool candidate_beats_best(const Rule *cand, size_t cand_order, uint64_t cand_ts,
                                 const Rule *best, size_t best_order, uint64_t best_ts,
                                 ConflictStrategy strategy) {
    switch (strategy) {
        case STRATEGY_ORDER:
            return cand_order < best_order;

        case STRATEGY_SPECIFICITY:
            if (cand->condition_count > best->condition_count) {
                return true;
            }
            if (cand->condition_count < best->condition_count) {
                return false;
            }
            return cand_order < best_order;

        case STRATEGY_RECENCY:
            if (cand_ts > best_ts) {
                return true;
            }
            if (cand_ts < best_ts) {
                return false;
            }
            if (cand->condition_count > best->condition_count) {
                return true;
            }
            if (cand->condition_count < best->condition_count) {
                return false;
            }
            return cand_order < best_order;

        case STRATEGY_SALIENCE:
            if (cand->salience > best->salience) {
                return true;
            }
            if (cand->salience < best->salience) {
                return false;
            }
            return cand_order < best_order;

        default:
            return cand_order < best_order;
    }
}

void engine_init(Engine *e, ConflictStrategy strategy) {
    if (!e) {
        return;
    }
    wm_init(&e->wm);
    rb_init(&e->rb);
    e->strategy = strategy;
    e->max_cycles = 1000;
    e->cycle_count = 0;
    e->rules_fired_count = 0;
    e->action_sink = NULL;
    e->action_user_data = NULL;
    e->firing_history = NULL;
}

void engine_free(Engine *e) {
    if (!e) {
        return;
    }
    wm_free(&e->wm);
    rb_free(&e->rb);
    free_firing_history(e);
    e->cycle_count = 0;
    e->rules_fired_count = 0;
    e->max_cycles = 0;
    e->action_sink = NULL;
    e->action_user_data = NULL;
}

void engine_set_strategy(Engine *e, ConflictStrategy strategy) {
    if (e) {
        e->strategy = strategy;
    }
}

void engine_set_max_cycles(Engine *e, size_t max_cycles) {
    if (e) {
        e->max_cycles = max_cycles;
    }
}

void engine_set_action_sink(Engine *e, ActionSinkFn sink, void *user_data) {
    if (e) {
        e->action_sink = sink;
        e->action_user_data = user_data;
    }
}

EngineStepResult engine_step(Engine *e) {
    if (!e) {
        return ENGINE_STEP_ERROR;
    }

    if (e->cycle_count >= e->max_cycles) {
        return ENGINE_STEP_MAX_CYCLES;
    }

    e->cycle_count++;

    const Rule *best_rule = NULL;
    size_t best_order = 0;
    uint64_t best_ts = 0;

    size_t order = 0;
    const Rule *curr = e->rb.head;
    while (curr) {
        uint64_t max_ts = 0;
        if (rule_matches(curr, &e->wm, &max_ts)) {
            RuleFiringRecord *rec = find_firing_record(e, curr->name);
            bool eligible = false;
            if (!rec || !rec->has_fired) {
                eligible = true;
            } else if (max_ts > rec->last_fired_timestamp) {
                eligible = true;
            }

            if (eligible) {
                if (!best_rule || candidate_beats_best(curr, order, max_ts,
                                                       best_rule, best_order, best_ts,
                                                       e->strategy)) {
                    best_rule = curr;
                    best_order = order;
                    best_ts = max_ts;
                }
            }
        }
        curr = curr->next;
        order++;
    }

    if (!best_rule) {
        return ENGINE_STEP_QUIESCENT;
    }

    /* Execute actions of best_rule */
    const Action *act = best_rule->actions;
    while (act) {
        switch (act->type) {
            case ACT_ASSERT:
                wm_assert(&e->wm, act->target_name, value_clone(&act->arg_val));
                break;
            case ACT_RETRACT:
                wm_retract(&e->wm, act->target_name);
                break;
            case ACT_CUSTOM:
                if (e->action_sink) {
                    e->action_sink(act->target_name, &act->arg_val, e->action_user_data);
                }
                break;
        }
        act = act->next;
    }

    /* Update firing history for refraction */
    update_firing_record(e, best_rule->name, best_ts);
    e->rules_fired_count++;

    return ENGINE_STEP_FIRED;
}

EngineStepResult engine_run(Engine *e) {
    if (!e) {
        return ENGINE_STEP_ERROR;
    }

    while (e->cycle_count < e->max_cycles) {
        EngineStepResult res = engine_step(e);
        if (res != ENGINE_STEP_FIRED) {
            return res;
        }
    }

    return ENGINE_STEP_MAX_CYCLES;
}

void engine_dump(const Engine *e, FILE *out) {
    if (!out) {
        out = stdout;
    }
    if (!e) {
        fputs("=== Engine: (null) ===\n", out);
        return;
    }

    fprintf(out, "=== Engine (strategy: %s, cycles: %zu/%zu, fired: %zu) ===\n",
            conflict_strategy_to_string(e->strategy),
            e->cycle_count, e->max_cycles, e->rules_fired_count);
    wm_dump(&e->wm, out);
    fprintf(out, "=== Rule Base (%zu rules) ===\n", e->rb.count);
    const Rule *r = e->rb.head;
    while (r) {
        fprintf(out, "  Rule '%s' (salience: %d, conds: %zu, acts: %zu)\n",
                r->name ? r->name : "(unnamed)",
                r->salience,
                r->condition_count,
                r->action_count);
        r = r->next;
    }
    fputs("===================================================\n", out);
}

bool engine_is_rule_refracted(const Engine *e, const char *rule_name, uint64_t match_ts) {
    if (!e || !rule_name) {
        return false;
    }
    const RuleFiringRecord *rec = find_firing_record(e, rule_name);
    if (!rec || !rec->has_fired) {
        return false;
    }
    return match_ts <= rec->last_fired_timestamp;
}

bool engine_rule_beats(const Rule *cand, size_t cand_order, uint64_t cand_ts,
                       const Rule *best, size_t best_order, uint64_t best_ts,
                       ConflictStrategy strategy) {
    return candidate_beats_best(cand, cand_order, cand_ts, best, best_order, best_ts, strategy);
}

