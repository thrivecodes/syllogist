#include "fact.h"
#include "rule.h"
#include "engine.h"
#include "io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define SYLLOGIST_VERSION "0.1.0"

/* Helper for case-insensitive string equality */
static bool str_equals_ci(const char *a, const char *b) {
    if (!a || !b) {
        return false;
    }
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return false;
        }
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

/* Parse conflict resolution strategy name */
static bool parse_strategy(const char *str, ConflictStrategy *out_strategy) {
    if (!str || !out_strategy) {
        return false;
    }
    if (str_equals_ci(str, "order")) {
        *out_strategy = STRATEGY_ORDER;
        return true;
    } else if (str_equals_ci(str, "specificity")) {
        *out_strategy = STRATEGY_SPECIFICITY;
        return true;
    } else if (str_equals_ci(str, "recency")) {
        *out_strategy = STRATEGY_RECENCY;
        return true;
    } else if (str_equals_ci(str, "salience")) {
        *out_strategy = STRATEGY_SALIENCE;
        return true;
    }
    return false;
}

/* CLI Help and Usage */
static void print_usage(const char *prog_name) {
    printf("Usage: %s [options] [rule-file]\n\n", prog_name ? prog_name : "syllogist");
    printf("A lightweight rule-based inference engine written in pure C99.\n\n");
    printf("Options:\n");
    printf("  -h, --help                 Display this help message and exit\n");
    printf("  -v, --version              Display version information and exit\n");
    printf("  --strategy <strategy>      Conflict resolution strategy:\n");
    printf("                               order       - First rule defined fires first (default)\n");
    printf("                               specificity - Rule with most conditions fires first\n");
    printf("                               recency     - Rule matching newest facts fires first\n");
    printf("                               salience    - Rule with highest priority score fires first\n");
    printf("  --demo                     Run the worked example trace (default behavior)\n\n");
    printf("Arguments:\n");
    printf("  rule-file                  Path to a rule file (full parser lands in Milestone 2)\n\n");
    printf("Overview:\n");
    printf("  Syllogist runs a classic match -> resolve -> act forward chaining inference loop\n");
    printf("  over working memory facts and rule bases. Features zero dependencies, manual\n");
    printf("  memory management, pluggable I/O action sinks, and refraction to prevent infinite loops.\n");
}

/* CLI Version */
static void print_version(void) {
    printf("Syllogist %s\n", SYLLOGIST_VERSION);
    printf("Lightweight Rule-Based Inference Engine (C99)\n");
}

/* Custom action sink for demo execution */
static bool cli_action_sink(const char *action_name, const Value *arg, void *user_data) {
    (void)user_data;
    printf("    [ACTION SINK] Triggered action: %s(", action_name ? action_name : "unknown");
    if (arg && arg->type != VAL_NONE) {
        value_print(arg, stdout);
    }
    printf(")\n");
    return true;
}

/* Print rule representation */
static void print_rule_summary(const Rule *r, size_t idx) {
    printf("  [Rule %zu] \"%s\" (salience: %d)\n", idx, r->name ? r->name : "unnamed", r->salience);
    printf("      IF   ");
    const Condition *c = r->conditions;
    bool first_c = true;
    while (c) {
        if (!first_c) {
            printf(" AND ");
        }
        printf("%s %s ", c->fact_name, operator_to_string(c->op));
        value_print(&c->target_val, stdout);
        first_c = false;
        c = c->next;
    }
    printf("\n      THEN ");
    const Action *a = r->actions;
    bool first_a = true;
    while (a) {
        if (!first_a) {
            printf(", ");
        }
        if (a->type == ACT_ASSERT) {
            printf("assert(%s, ", a->target_name);
            value_print(&a->arg_val, stdout);
            printf(")");
        } else if (a->type == ACT_RETRACT) {
            printf("retract(%s)", a->target_name);
        } else {
            printf("%s(", a->target_name);
            if (a->arg_val.type != VAL_NONE) {
                value_print(&a->arg_val, stdout);
            }
            printf(")");
        }
        first_a = false;
        a = a->next;
    }
    printf("\n");
}

/* Execute worked example from README with full step-by-step trace */
static int run_worked_example(ConflictStrategy strategy, const char *rule_file_path) {
    printf("================================================================================\n");
    printf("  SYLLOGIST - Lightweight Rule-Based Inference Engine (v%s)\n", SYLLOGIST_VERSION);
    printf("================================================================================\n");
    printf("Configuration:\n");
    printf("  Conflict Strategy : %s\n", conflict_strategy_to_string(strategy));
    printf("  Execution Mode    : Step-by-Step Forward Chaining Trace\n");
    if (rule_file_path) {
        printf("  Rule File Target  : %s\n", rule_file_path);
        printf("  Notice            : File parser integration scheduled for Milestone 2.\n");
        printf("                      Running worked example rulebase to verify inference core.\n");
    }
    printf("================================================================================\n\n");

    Engine engine;
    engine_init(&engine, strategy);
    engine_set_action_sink(&engine, cli_action_sink, NULL);

    /* Setup Initial Working Memory facts */
    wm_assert(&engine.wm, "temperature", value_int(32));
    wm_assert(&engine.wm, "humidity", value_int(45));

    /* Setup Rule 1: detect_overheat */
    Rule *r1 = rule_create("detect_overheat", 0);
    rule_add_condition(r1, "temperature", OP_GT, value_int(30));
    rule_add_action(r1, ACT_ASSERT, "overheat_risk", value_bool(true));
    rb_add_rule(&engine.rb, r1);

    /* Setup Rule 2: warn_dry_heat */
    Rule *r2 = rule_create("warn_dry_heat", 0);
    rule_add_condition(r2, "overheat_risk", OP_EQ, value_bool(true));
    rule_add_condition(r2, "humidity", OP_LT, value_int(50));
    rule_add_action(r2, ACT_CUSTOM, "alert", value_string("dry_heat_warning"));
    rb_add_rule(&engine.rb, r2);

    /* Display initial Working Memory */
    printf("[WORKING MEMORY] Initial Facts (%zu):\n", wm_count(&engine.wm));
    const Fact *f = engine.wm.head;
    while (f) {
        printf("  * %s = ", f->name);
        value_print(&f->value, stdout);
        printf(" (tick: %llu)\n", (unsigned long long)f->timestamp);
        f = f->next;
    }
    printf("\n");

    /* Display loaded Rule Base */
    printf("[RULE BASE] Loaded Rules (%zu):\n", rb_count(&engine.rb));
    const Rule *r = engine.rb.head;
    size_t r_idx = 1;
    while (r) {
        print_rule_summary(r, r_idx++);
        r = r->next;
    }
    printf("\n================================================================================\n");
    printf("Starting inference match-resolve-act cycle...\n");
    printf("================================================================================\n");

    size_t cycle_num = 1;
    while (true) {
        printf("\n--- Cycle %zu ---\n", cycle_num);

        /* Phase 1: Match & Agenda building */
        printf("  [MATCH] Evaluating rules against current Working Memory:\n");
        const Rule *agenda_best = NULL;
        size_t best_order = 0;
        uint64_t best_ts = 0;
        size_t eligible_count = 0;

        size_t order = 0;
        const Rule *curr = engine.rb.head;
        while (curr) {
            uint64_t match_ts = 0;
            bool matches = rule_matches(curr, &engine.wm, &match_ts);
            if (!matches) {
                printf("    - Rule \"%s\": NO MATCH (conditions not met)\n", curr->name);
            } else if (engine_is_rule_refracted(&engine, curr->name, match_ts)) {
                printf("    - Rule \"%s\": REFRACTED (already fired for fact tick %llu)\n",
                       curr->name, (unsigned long long)match_ts);
            } else {
                printf("    - Rule \"%s\": MATCH (eligible, max fact tick: %llu)\n",
                       curr->name, (unsigned long long)match_ts);
                if (!agenda_best || engine_rule_beats(curr, order, match_ts,
                                                      agenda_best, best_order, best_ts,
                                                      strategy)) {
                    agenda_best = curr;
                    best_order = order;
                    best_ts = match_ts;
                }
                eligible_count++;
            }
            curr = curr->next;
            order++;
        }

        /* Phase 2: Conflict Resolution */
        if (!agenda_best) {
            printf("  [RESOLVE] Agenda is empty (0 eligible rules).\n");
            printf("  [ACT]     No rules to fire. Engine reached quiescence.\n");
            EngineStepResult res = engine_step(&engine);
            (void)res; /* Will be ENGINE_STEP_QUIESCENT */
            break;
        }

        printf("  [RESOLVE] Agenda contains %zu rule(s). Selected \"%s\" using %s strategy.\n",
               eligible_count, agenda_best->name, conflict_strategy_to_string(strategy));

        /* Phase 3: Act */
        printf("  [ACT]     Firing rule \"%s\":\n", agenda_best->name);
        size_t wm_count_before = wm_count(&engine.wm);

        EngineStepResult res = engine_step(&engine);
        if (res == ENGINE_STEP_FIRED) {
            size_t wm_count_after = wm_count(&engine.wm);
            if (wm_count_after > wm_count_before) {
                /* Newly asserted fact is at head of working memory */
                const Fact *new_fact = engine.wm.head;
                printf("    [ASSERT] Working Memory updated: %s = ", new_fact->name);
                value_print(&new_fact->value, stdout);
                printf(" (tick: %llu)\n", (unsigned long long)new_fact->timestamp);
            }
            printf("  [STATUS]  Cycle %zu finished successfully.\n", cycle_num);
        } else {
            printf("  [STATUS]  Cycle %zu ended with result code: %d\n", cycle_num, (int)res);
            break;
        }

        cycle_num++;
    }

    printf("\n================================================================================\n");
    printf("Inference Complete (Engine Quiescent)\n");
    printf("================================================================================\n");
    printf("Final Working Memory State (%zu facts):\n", wm_count(&engine.wm));
    const Fact *final_f = engine.wm.head;
    while (final_f) {
        printf("  * %-16s = ", final_f->name);
        value_print(&final_f->value, stdout);
        printf("  (timestamp: %llu)\n", (unsigned long long)final_f->timestamp);
        final_f = final_f->next;
    }

    printf("\nExecution Statistics:\n");
    printf("  Total Cycles Evaluated : %zu\n", engine.cycle_count);
    printf("  Rules Fired            : %zu\n", engine.rules_fired_count);
    printf("  Conflict Strategy      : %s\n", conflict_strategy_to_string(engine.strategy));
    printf("  Termination Status     : QUIESCENT (forward chaining stable)\n");
    printf("================================================================================\n");

    /* Clean memory teardown: frees working memory, rules, and firing history */
    engine_free(&engine);
    return 0;
}

int main(int argc, char **argv) {
    ConflictStrategy strategy = STRATEGY_ORDER;
    const char *rule_file = NULL;

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(arg, "-v") == 0 || strcmp(arg, "--version") == 0) {
            print_version();
            return 0;
        } else if (strcmp(arg, "--demo") == 0) {
            /* Explicit demo flag, already default behavior */
        } else if (strcmp(arg, "--strategy") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --strategy requires an argument (order, specificity, recency, salience).\n\n");
                print_usage(argv[0]);
                return 1;
            }
            i++;
            if (!parse_strategy(argv[i], &strategy)) {
                fprintf(stderr, "Error: Unknown strategy '%s'. Expected order, specificity, recency, or salience.\n\n", argv[i]);
                print_usage(argv[0]);
                return 1;
            }
        } else if (strncmp(arg, "--strategy=", 11) == 0) {
            const char *val = arg + 11;
            if (!parse_strategy(val, &strategy)) {
                fprintf(stderr, "Error: Unknown strategy '%s'. Expected order, specificity, recency, or salience.\n\n", val);
                print_usage(argv[0]);
                return 1;
            }
        } else if (arg[0] == '-') {
            fprintf(stderr, "Error: Unknown option '%s'.\n\n", arg);
            print_usage(argv[0]);
            return 1;
        } else {
            if (!rule_file) {
                rule_file = arg;
            } else {
                fprintf(stderr, "Error: Multiple rule files provided ('%s' and '%s').\n\n", rule_file, arg);
                print_usage(argv[0]);
                return 1;
            }
        }
    }

    return run_worked_example(strategy, rule_file);
}
