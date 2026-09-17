#include "test_runner.h"
#include "fact.h"
#include "rule.h"
#include "io.h"
#include "engine.h"

TEST(conflict_strategy_strings) {
    ASSERT_STR_EQ(conflict_strategy_to_string(STRATEGY_ORDER), "ORDER");
    ASSERT_STR_EQ(conflict_strategy_to_string(STRATEGY_SPECIFICITY), "SPECIFICITY");
    ASSERT_STR_EQ(conflict_strategy_to_string(STRATEGY_RECENCY), "RECENCY");
    ASSERT_STR_EQ(conflict_strategy_to_string(STRATEGY_SALIENCE), "SALIENCE");
    ASSERT_STR_EQ(conflict_strategy_to_string((ConflictStrategy)999), "UNKNOWN");
}

TEST(forward_chaining_worked_example) {
    Engine e;
    engine_init(&e, STRATEGY_ORDER);

    IOMock mock;
    io_mock_init(&mock);
    engine_set_action_sink(&e, io_mock_action_sink, &mock);

    /* Initial facts: temperature = 32, humidity = 45 */
    wm_assert(&e.wm, "temperature", value_int(32));
    wm_assert(&e.wm, "humidity", value_int(45));

    /* Rule 1: IF temperature > 30 THEN assert overheat_risk = true */
    Rule *r1 = rule_create("r1_overheat", 0);
    rule_add_condition(r1, "temperature", OP_GT, value_int(30));
    rule_add_action(r1, ACT_ASSERT, "overheat_risk", value_bool(true));
    rb_add_rule(&e.rb, r1);

    /* Rule 2: IF overheat_risk == true AND humidity < 50 THEN custom dry_heat_warning("alert") */
    Rule *r2 = rule_create("r2_dry_heat", 0);
    rule_add_condition(r2, "overheat_risk", OP_EQ, value_bool(true));
    rule_add_condition(r2, "humidity", OP_LT, value_int(50));
    rule_add_action(r2, ACT_CUSTOM, "dry_heat_warning", value_string("alert"));
    rb_add_rule(&e.rb, r2);

    /* Cycle 1: Rule 1 fires, asserts overheat_risk */
    EngineStepResult res1 = engine_step(&e);
    ASSERT_EQ_INT(res1, ENGINE_STEP_FIRED);
    ASSERT_EQ_INT(e.cycle_count, 1);
    ASSERT_EQ_INT(e.rules_fired_count, 1);

    const Fact *f_risk = wm_find(&e.wm, "overheat_risk");
    ASSERT(f_risk != NULL);
    ASSERT_EQ_INT(f_risk->value.type, VAL_BOOL);
    ASSERT(f_risk->value.as.b == true);
    ASSERT_EQ_INT(mock.count, 0);

    /* Cycle 2: Rule 2 fires, triggers dry_heat_warning */
    EngineStepResult res2 = engine_step(&e);
    ASSERT_EQ_INT(res2, ENGINE_STEP_FIRED);
    ASSERT_EQ_INT(e.cycle_count, 2);
    ASSERT_EQ_INT(e.rules_fired_count, 2);

    ASSERT_EQ_INT(mock.count, 1);
    const MockAction *ma = io_mock_get_action(&mock, 0);
    ASSERT(ma != NULL);
    ASSERT_STR_EQ(ma->name, "dry_heat_warning");
    ASSERT_EQ_INT(ma->arg.type, VAL_STRING);
    ASSERT_STR_EQ(ma->arg.as.s, "alert");

    /* Cycle 3: Quiescence */
    EngineStepResult res3 = engine_step(&e);
    ASSERT_EQ_INT(res3, ENGINE_STEP_QUIESCENT);
    ASSERT_EQ_INT(e.cycle_count, 3);
    ASSERT_EQ_INT(e.rules_fired_count, 2);

    io_mock_free(&mock);
    engine_free(&e);
}

TEST(conflict_resolution_order) {
    Engine e;
    engine_init(&e, STRATEGY_ORDER);

    IOMock mock;
    io_mock_init(&mock);
    engine_set_action_sink(&e, io_mock_action_sink, &mock);

    wm_assert(&e.wm, "sensor", value_int(10));

    Rule *r1 = rule_create("rule_first", 0);
    rule_add_condition(r1, "sensor", OP_EQ, value_int(10));
    rule_add_action(r1, ACT_CUSTOM, "action_first", value_none());
    rb_add_rule(&e.rb, r1);

    Rule *r2 = rule_create("rule_second", 0);
    rule_add_condition(r2, "sensor", OP_EQ, value_int(10));
    rule_add_action(r2, ACT_CUSTOM, "action_second", value_none());
    rb_add_rule(&e.rb, r2);

    /* Run engine to quiescence */
    EngineStepResult res = engine_run(&e);
    ASSERT_EQ_INT(res, ENGINE_STEP_QUIESCENT);
    ASSERT_EQ_INT(e.rules_fired_count, 2);
    ASSERT_EQ_INT(mock.count, 2);

    /* Rule 1 must have fired before Rule 2 */
    const MockAction *a0 = io_mock_get_action(&mock, 0);
    const MockAction *a1 = io_mock_get_action(&mock, 1);
    ASSERT(a0 != NULL);
    ASSERT(a1 != NULL);
    ASSERT_STR_EQ(a0->name, "action_first");
    ASSERT_STR_EQ(a1->name, "action_second");

    io_mock_free(&mock);
    engine_free(&e);
}

TEST(conflict_resolution_specificity) {
    Engine e;
    engine_init(&e, STRATEGY_SPECIFICITY);

    IOMock mock;
    io_mock_init(&mock);
    engine_set_action_sink(&e, io_mock_action_sink, &mock);

    wm_assert(&e.wm, "a", value_int(1));
    wm_assert(&e.wm, "b", value_int(2));

    /* Rule 1 has 1 condition, added first */
    Rule *r1 = rule_create("generic_rule", 0);
    rule_add_condition(r1, "a", OP_EQ, value_int(1));
    rule_add_action(r1, ACT_CUSTOM, "fired_generic", value_int(1));
    rb_add_rule(&e.rb, r1);

    /* Rule 2 has 2 conditions, added second */
    Rule *r2 = rule_create("specific_rule", 0);
    rule_add_condition(r2, "a", OP_EQ, value_int(1));
    rule_add_condition(r2, "b", OP_EQ, value_int(2));
    rule_add_action(r2, ACT_CUSTOM, "fired_specific", value_int(2));
    rb_add_rule(&e.rb, r2);

    /* Cycle 1: Specific rule (2 conditions) must win over generic rule (1 condition) */
    EngineStepResult res1 = engine_step(&e);
    ASSERT_EQ_INT(res1, ENGINE_STEP_FIRED);
    ASSERT_EQ_INT(mock.count, 1);
    const MockAction *a0 = io_mock_get_action(&mock, 0);
    ASSERT(a0 != NULL);
    ASSERT_STR_EQ(a0->name, "fired_specific");

    /* Cycle 2: Generic rule now fires */
    EngineStepResult res2 = engine_step(&e);
    ASSERT_EQ_INT(res2, ENGINE_STEP_FIRED);
    ASSERT_EQ_INT(mock.count, 2);
    const MockAction *a1 = io_mock_get_action(&mock, 1);
    ASSERT(a1 != NULL);
    ASSERT_STR_EQ(a1->name, "fired_generic");

    /* Cycle 3: Quiescent */
    EngineStepResult res3 = engine_step(&e);
    ASSERT_EQ_INT(res3, ENGINE_STEP_QUIESCENT);

    io_mock_free(&mock);
    engine_free(&e);
}

TEST(conflict_resolution_recency) {
    Engine e;
    engine_init(&e, STRATEGY_RECENCY);

    IOMock mock;
    io_mock_init(&mock);
    engine_set_action_sink(&e, io_mock_action_sink, &mock);

    /* Assert fact 1 at clock 1 */
    wm_assert(&e.wm, "fact_old", value_int(100));
    /* Assert intermediate fact to bump clock to 2 */
    wm_assert(&e.wm, "fact_middle", value_int(200));
    /* Assert fact 3 at clock 3 */
    wm_assert(&e.wm, "fact_new", value_int(300));

    /* Rule 1 matches fact_old (timestamp 1), added first */
    Rule *r1 = rule_create("old_rule", 0);
    rule_add_condition(r1, "fact_old", OP_EQ, value_int(100));
    rule_add_action(r1, ACT_CUSTOM, "fired_old", value_int(100));
    rb_add_rule(&e.rb, r1);

    /* Rule 2 matches fact_new (timestamp 3), added second */
    Rule *r2 = rule_create("new_rule", 0);
    rule_add_condition(r2, "fact_new", OP_EQ, value_int(300));
    rule_add_action(r2, ACT_CUSTOM, "fired_new", value_int(300));
    rb_add_rule(&e.rb, r2);

    /* Cycle 1: Rule matching fact updated at clock 3 must fire before rule at clock 1 */
    EngineStepResult res1 = engine_step(&e);
    ASSERT_EQ_INT(res1, ENGINE_STEP_FIRED);
    ASSERT_EQ_INT(mock.count, 1);
    const MockAction *a0 = io_mock_get_action(&mock, 0);
    ASSERT(a0 != NULL);
    ASSERT_STR_EQ(a0->name, "fired_new");

    /* Cycle 2: Old rule fires */
    EngineStepResult res2 = engine_step(&e);
    ASSERT_EQ_INT(res2, ENGINE_STEP_FIRED);
    ASSERT_EQ_INT(mock.count, 2);
    const MockAction *a1 = io_mock_get_action(&mock, 1);
    ASSERT(a1 != NULL);
    ASSERT_STR_EQ(a1->name, "fired_old");

    /* Cycle 3: Quiescent */
    EngineStepResult res3 = engine_step(&e);
    ASSERT_EQ_INT(res3, ENGINE_STEP_QUIESCENT);

    io_mock_free(&mock);
    engine_free(&e);
}

TEST(conflict_resolution_salience) {
    Engine e;
    engine_init(&e, STRATEGY_SALIENCE);

    IOMock mock;
    io_mock_init(&mock);
    engine_set_action_sink(&e, io_mock_action_sink, &mock);

    wm_assert(&e.wm, "event", value_string("trigger"));

    /* Low salience rule (salience 0), added first */
    Rule *r_low = rule_create("low_salience_rule", 0);
    rule_add_condition(r_low, "event", OP_EXISTS, value_none());
    rule_add_action(r_low, ACT_CUSTOM, "low_action", value_int(0));
    rb_add_rule(&e.rb, r_low);

    /* High salience rule (salience 10), added second */
    Rule *r_high = rule_create("high_salience_rule", 10);
    rule_add_condition(r_high, "event", OP_EXISTS, value_none());
    rule_add_action(r_high, ACT_CUSTOM, "high_action", value_int(10));
    rb_add_rule(&e.rb, r_high);

    /* Cycle 1: Rule with salience 10 must fire before rule with salience 0 */
    EngineStepResult res1 = engine_step(&e);
    ASSERT_EQ_INT(res1, ENGINE_STEP_FIRED);
    ASSERT_EQ_INT(mock.count, 1);
    const MockAction *a0 = io_mock_get_action(&mock, 0);
    ASSERT(a0 != NULL);
    ASSERT_STR_EQ(a0->name, "high_action");

    /* Cycle 2: Low salience rule fires */
    EngineStepResult res2 = engine_step(&e);
    ASSERT_EQ_INT(res2, ENGINE_STEP_FIRED);
    ASSERT_EQ_INT(mock.count, 2);
    const MockAction *a1 = io_mock_get_action(&mock, 1);
    ASSERT(a1 != NULL);
    ASSERT_STR_EQ(a1->name, "low_action");

    /* Cycle 3: Quiescent */
    EngineStepResult res3 = engine_step(&e);
    ASSERT_EQ_INT(res3, ENGINE_STEP_QUIESCENT);

    io_mock_free(&mock);
    engine_free(&e);
}

TEST(refraction_prevents_refiring_on_unchanged_facts) {
    Engine e;
    engine_init(&e, STRATEGY_ORDER);

    IOMock mock;
    io_mock_init(&mock);
    engine_set_action_sink(&e, io_mock_action_sink, &mock);

    wm_assert(&e.wm, "status", value_string("running"));

    Rule *r = rule_create("run_logger", 5);
    rule_add_condition(r, "status", OP_EQ, value_string("running"));
    rule_add_action(r, ACT_CUSTOM, "log_running", value_none());
    rb_add_rule(&e.rb, r);

    /* Step 1: Fires */
    EngineStepResult res1 = engine_step(&e);
    ASSERT_EQ_INT(res1, ENGINE_STEP_FIRED);
    ASSERT_EQ_INT(mock.count, 1);

    /* Step 2: Quiescent because fact hasn't changed (refracted) */
    EngineStepResult res2 = engine_step(&e);
    ASSERT_EQ_INT(res2, ENGINE_STEP_QUIESCENT);
    ASSERT_EQ_INT(mock.count, 1);

    /* Step 3: Still quiescent */
    EngineStepResult res3 = engine_step(&e);
    ASSERT_EQ_INT(res3, ENGINE_STEP_QUIESCENT);
    ASSERT_EQ_INT(mock.count, 1);

    io_mock_free(&mock);
    engine_free(&e);
}

TEST(refraction_refires_when_condition_fact_modified) {
    Engine e;
    engine_init(&e, STRATEGY_ORDER);

    IOMock mock;
    io_mock_init(&mock);
    engine_set_action_sink(&e, io_mock_action_sink, &mock);

    wm_assert(&e.wm, "pressure", value_int(100));

    Rule *r = rule_create("pressure_monitor", 0);
    rule_add_condition(r, "pressure", OP_GTE, value_int(100));
    rule_add_action(r, ACT_CUSTOM, "warn_pressure", value_none());
    rb_add_rule(&e.rb, r);

    /* Step 1: Fires */
    EngineStepResult res1 = engine_step(&e);
    ASSERT_EQ_INT(res1, ENGINE_STEP_FIRED);
    ASSERT_EQ_INT(mock.count, 1);

    /* Step 2: Quiescent */
    EngineStepResult res2 = engine_step(&e);
    ASSERT_EQ_INT(res2, ENGINE_STEP_QUIESCENT);
    ASSERT_EQ_INT(mock.count, 1);

    /* Modify fact (bumps timestamp) */
    wm_assert(&e.wm, "pressure", value_int(105));

    /* Step 3: Re-fires because timestamp was bumped */
    EngineStepResult res3 = engine_step(&e);
    ASSERT_EQ_INT(res3, ENGINE_STEP_FIRED);
    ASSERT_EQ_INT(mock.count, 2);

    /* Step 4: Quiescent again */
    EngineStepResult res4 = engine_step(&e);
    ASSERT_EQ_INT(res4, ENGINE_STEP_QUIESCENT);
    ASSERT_EQ_INT(mock.count, 2);

    /* Re-assert identical value: still bumps timestamp in working memory */
    wm_assert(&e.wm, "pressure", value_int(105));

    /* Step 5: Re-fires again */
    EngineStepResult res5 = engine_step(&e);
    ASSERT_EQ_INT(res5, ENGINE_STEP_FIRED);
    ASSERT_EQ_INT(mock.count, 3);

    io_mock_free(&mock);
    engine_free(&e);
}

TEST(max_cycles_loop_prevention) {
    Engine e;
    engine_init(&e, STRATEGY_ORDER);
    engine_set_max_cycles(&e, 20);

    /* Rule ping: IF state == "ping" THEN assert state = "pong" */
    Rule *r_ping = rule_create("r_ping", 0);
    rule_add_condition(r_ping, "state", OP_EQ, value_string("ping"));
    rule_add_action(r_ping, ACT_ASSERT, "state", value_string("pong"));
    rb_add_rule(&e.rb, r_ping);

    /* Rule pong: IF state == "pong" THEN assert state = "ping" */
    Rule *r_pong = rule_create("r_pong", 0);
    rule_add_condition(r_pong, "state", OP_EQ, value_string("pong"));
    rule_add_action(r_pong, ACT_ASSERT, "state", value_string("ping"));
    rb_add_rule(&e.rb, r_pong);

    wm_assert(&e.wm, "state", value_string("ping"));

    /* engine_run should halt when max_cycles (20) is reached */
    EngineStepResult res = engine_run(&e);
    ASSERT_EQ_INT(res, ENGINE_STEP_MAX_CYCLES);
    ASSERT_EQ_INT(e.cycle_count, 20);
    ASSERT_EQ_INT(e.rules_fired_count, 20);

    /* Subsequent engine_step should also return MAX_CYCLES */
    EngineStepResult step_res = engine_step(&e);
    ASSERT_EQ_INT(step_res, ENGINE_STEP_MAX_CYCLES);

    engine_free(&e);
}

TEST(retract_action_execution) {
    Engine e;
    engine_init(&e, STRATEGY_ORDER);

    wm_assert(&e.wm, "transient_flag", value_bool(true));
    ASSERT_EQ_INT(wm_count(&e.wm), 1);

    Rule *r = rule_create("cleaner", 0);
    rule_add_condition(r, "transient_flag", OP_EQ, value_bool(true));
    rule_add_action(r, ACT_RETRACT, "transient_flag", value_none());
    rb_add_rule(&e.rb, r);

    EngineStepResult res1 = engine_step(&e);
    ASSERT_EQ_INT(res1, ENGINE_STEP_FIRED);
    ASSERT_EQ_INT(wm_count(&e.wm), 0);
    ASSERT(wm_find(&e.wm, "transient_flag") == NULL);

    EngineStepResult res2 = engine_step(&e);
    ASSERT_EQ_INT(res2, ENGINE_STEP_QUIESCENT);

    engine_free(&e);
}

TEST(io_mock_standalone_operations) {
    IOMock mock;
    io_mock_init(&mock);
    ASSERT_EQ_INT(mock.count, 0);
    ASSERT(mock.head == NULL);

    Value val1 = value_string("first_arg");
    Value val2 = value_int(42);

    ASSERT(io_mock_action_sink("act1", &val1, &mock) == true);
    ASSERT(io_mock_action_sink("act2", &val2, &mock) == true);
    ASSERT_EQ_INT(mock.count, 2);

    const MockAction *a0 = io_mock_get_action(&mock, 0);
    ASSERT(a0 != NULL);
    ASSERT_STR_EQ(a0->name, "act1");
    ASSERT_STR_EQ(a0->arg.as.s, "first_arg");

    const MockAction *a1 = io_mock_get_action(&mock, 1);
    ASSERT(a1 != NULL);
    ASSERT_STR_EQ(a1->name, "act2");
    ASSERT_EQ_INT(a1->arg.as.i, 42);

    /* Out of bounds access */
    ASSERT(io_mock_get_action(&mock, 2) == NULL);
    ASSERT(io_mock_get_action(&mock, 99) == NULL);
    ASSERT(io_mock_get_action(NULL, 0) == NULL);

    /* Test invalid call */
    ASSERT(io_mock_action_sink(NULL, &val1, &mock) == false);
    ASSERT(io_mock_action_sink("act3", &val1, NULL) == false);

    value_free(&val1);
    value_free(&val2);
    io_mock_free(&mock);
    ASSERT_EQ_INT(mock.count, 0);
    ASSERT(mock.head == NULL);
}

TEST(io_console_action_sink_smoke) {
    Value val = value_string("hello");
    io_console_action_sink("greet", &val, NULL);
    io_console_action_sink("test_null_val", NULL, NULL);
    io_console_action_sink(NULL, NULL, NULL);
    value_free(&val);
}

TEST(engine_dump_smoke) {
    Engine e;
    engine_init(&e, STRATEGY_SPECIFICITY);
    wm_assert(&e.wm, "k", value_int(1));

    Rule *r = rule_create("dummy", 10);
    rule_add_condition(r, "k", OP_EQ, value_int(1));
    rule_add_action(r, ACT_ASSERT, "out", value_int(2));
    rb_add_rule(&e.rb, r);

    /* Dump to temporary file */
    FILE *tmp = tmpfile();
    if (tmp) {
        engine_dump(&e, tmp);
        fclose(tmp);
    }
    engine_dump(NULL, NULL);

    engine_free(&e);
}

TEST(engine_inspection_api) {
    Engine e;
    engine_init(&e, STRATEGY_ORDER);

    /* Initially not refracted */
    ASSERT(!engine_is_rule_refracted(&e, "rule1", 1));

    wm_assert(&e.wm, "a", value_int(10));
    Rule *r1 = rule_create("rule1", 0);
    rule_add_condition(r1, "a", OP_EQ, value_int(10));
    rule_add_action(r1, ACT_ASSERT, "b", value_int(20));
    rb_add_rule(&e.rb, r1);

    Rule *r2 = rule_create("rule2", 5);
    rule_add_condition(r2, "a", OP_EQ, value_int(10));
    rule_add_action(r2, ACT_ASSERT, "c", value_int(30));
    rb_add_rule(&e.rb, r2);

    /* Test engine_rule_beats */
    ASSERT(engine_rule_beats(r1, 0, 1, r2, 1, 1, STRATEGY_ORDER));
    ASSERT(engine_rule_beats(r2, 1, 1, r1, 0, 1, STRATEGY_SALIENCE));

    /* Fire one step */
    EngineStepResult res = engine_step(&e);
    ASSERT_EQ_INT(res, ENGINE_STEP_FIRED);

    /* rule1 has fired for tick 1 */
    ASSERT(engine_is_rule_refracted(&e, "rule1", 1));
    /* But if tick increases to 2, it is not refracted */
    ASSERT(!engine_is_rule_refracted(&e, "rule1", 2));

    engine_free(&e);
}

int main(void) {
    printf("Starting Syllogist Inference Engine & Pluggable I/O Tests...\n\n");

    RUN_TEST(conflict_strategy_strings);
    RUN_TEST(forward_chaining_worked_example);
    RUN_TEST(conflict_resolution_order);
    RUN_TEST(conflict_resolution_specificity);
    RUN_TEST(conflict_resolution_recency);
    RUN_TEST(conflict_resolution_salience);
    RUN_TEST(refraction_prevents_refiring_on_unchanged_facts);
    RUN_TEST(refraction_refires_when_condition_fact_modified);
    RUN_TEST(max_cycles_loop_prevention);
    RUN_TEST(retract_action_execution);
    RUN_TEST(io_mock_standalone_operations);
    RUN_TEST(io_console_action_sink_smoke);
    RUN_TEST(engine_dump_smoke);
    RUN_TEST(engine_inspection_api);

    return TEST_RUNNER_SUMMARY();
}

