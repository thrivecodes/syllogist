#include "test_runner.h"
#include "fact.h"
#include "rule.h"

TEST(operator_strings) {
    ASSERT_STR_EQ(operator_to_string(OP_EQ), "==");
    ASSERT_STR_EQ(operator_to_string(OP_NEQ), "!=");
    ASSERT_STR_EQ(operator_to_string(OP_LT), "<");
    ASSERT_STR_EQ(operator_to_string(OP_LTE), "<=");
    ASSERT_STR_EQ(operator_to_string(OP_GT), ">");
    ASSERT_STR_EQ(operator_to_string(OP_GTE), ">=");
    ASSERT_STR_EQ(operator_to_string(OP_EXISTS), "exists");
    ASSERT_STR_EQ(operator_to_string((Operator)999), "unknown");
}

TEST(condition_creation_and_free) {
    Condition *c = condition_create("temperature", OP_GT, value_int(25));
    ASSERT(c != NULL);
    ASSERT_STR_EQ(c->fact_name, "temperature");
    ASSERT_EQ_INT(c->op, OP_GT);
    ASSERT_EQ_INT(c->target_val.type, VAL_INT);
    ASSERT_EQ_INT(c->target_val.as.i, 25);
    ASSERT(c->next == NULL);

    condition_free(c);
}

TEST(condition_numeric_evaluations) {
    WorkingMemory wm;
    wm_init(&wm);
    wm_assert(&wm, "temp", value_int(25));
    const Fact *f = wm_find(&wm, "temp");
    ASSERT(f != NULL);

    /* OP_GT */
    Condition *c_gt_true = condition_create("temp", OP_GT, value_int(20));
    Condition *c_gt_false_eq = condition_create("temp", OP_GT, value_int(25));
    Condition *c_gt_false = condition_create("temp", OP_GT, value_int(30));
    ASSERT(condition_eval(c_gt_true, f) == true);
    ASSERT(condition_eval(c_gt_false_eq, f) == false);
    ASSERT(condition_eval(c_gt_false, f) == false);

    /* OP_GTE */
    Condition *c_gte_true1 = condition_create("temp", OP_GTE, value_int(20));
    Condition *c_gte_true2 = condition_create("temp", OP_GTE, value_int(25));
    Condition *c_gte_false = condition_create("temp", OP_GTE, value_int(30));
    ASSERT(condition_eval(c_gte_true1, f) == true);
    ASSERT(condition_eval(c_gte_true2, f) == true);
    ASSERT(condition_eval(c_gte_false, f) == false);

    /* OP_LT */
    Condition *c_lt_true = condition_create("temp", OP_LT, value_int(30));
    Condition *c_lt_false_eq = condition_create("temp", OP_LT, value_int(25));
    Condition *c_lt_false = condition_create("temp", OP_LT, value_int(20));
    ASSERT(condition_eval(c_lt_true, f) == true);
    ASSERT(condition_eval(c_lt_false_eq, f) == false);
    ASSERT(condition_eval(c_lt_false, f) == false);

    /* OP_LTE */
    Condition *c_lte_true1 = condition_create("temp", OP_LTE, value_int(30));
    Condition *c_lte_true2 = condition_create("temp", OP_LTE, value_int(25));
    Condition *c_lte_false = condition_create("temp", OP_LTE, value_int(20));
    ASSERT(condition_eval(c_lte_true1, f) == true);
    ASSERT(condition_eval(c_lte_true2, f) == true);
    ASSERT(condition_eval(c_lte_false, f) == false);

    /* OP_EQ & OP_NEQ */
    Condition *c_eq_true = condition_create("temp", OP_EQ, value_int(25));
    Condition *c_eq_false = condition_create("temp", OP_EQ, value_int(26));
    Condition *c_neq_true = condition_create("temp", OP_NEQ, value_int(26));
    Condition *c_neq_false = condition_create("temp", OP_NEQ, value_int(25));
    ASSERT(condition_eval(c_eq_true, f) == true);
    ASSERT(condition_eval(c_eq_false, f) == false);
    ASSERT(condition_eval(c_neq_true, f) == true);
    ASSERT(condition_eval(c_neq_false, f) == false);

    /* Cross-type int vs float */
    Condition *c_eq_float = condition_create("temp", OP_EQ, value_float(25.0));
    Condition *c_lt_float = condition_create("temp", OP_LT, value_float(25.1));
    ASSERT(condition_eval(c_eq_float, f) == true);
    ASSERT(condition_eval(c_lt_float, f) == true);

    condition_free(c_gt_true);
    condition_free(c_gt_false_eq);
    condition_free(c_gt_false);
    condition_free(c_gte_true1);
    condition_free(c_gte_true2);
    condition_free(c_gte_false);
    condition_free(c_lt_true);
    condition_free(c_lt_false_eq);
    condition_free(c_lt_false);
    condition_free(c_lte_true1);
    condition_free(c_lte_true2);
    condition_free(c_lte_false);
    condition_free(c_eq_true);
    condition_free(c_eq_false);
    condition_free(c_neq_true);
    condition_free(c_neq_false);
    condition_free(c_eq_float);
    condition_free(c_lt_float);
    wm_free(&wm);
}

TEST(condition_string_and_bool_evaluations) {
    WorkingMemory wm;
    wm_init(&wm);
    wm_assert(&wm, "status", value_string("operational"));
    wm_assert(&wm, "active", value_bool(true));

    const Fact *f_status = wm_find(&wm, "status");
    const Fact *f_active = wm_find(&wm, "active");
    ASSERT(f_status != NULL);
    ASSERT(f_active != NULL);

    /* String equality & inequality */
    Condition *c_str_eq = condition_create("status", OP_EQ, value_string("operational"));
    Condition *c_str_diff = condition_create("status", OP_EQ, value_string("failed"));
    Condition *c_str_neq = condition_create("status", OP_NEQ, value_string("failed"));
    ASSERT(condition_eval(c_str_eq, f_status) == true);
    ASSERT(condition_eval(c_str_diff, f_status) == false);
    ASSERT(condition_eval(c_str_neq, f_status) == true);

    /* Bool equality */
    Condition *c_bool_true = condition_create("active", OP_EQ, value_bool(true));
    Condition *c_bool_false = condition_create("active", OP_EQ, value_bool(false));
    ASSERT(condition_eval(c_bool_true, f_active) == true);
    ASSERT(condition_eval(c_bool_false, f_active) == false);

    condition_free(c_str_eq);
    condition_free(c_str_diff);
    condition_free(c_str_neq);
    condition_free(c_bool_true);
    condition_free(c_bool_false);
    wm_free(&wm);
}

TEST(condition_op_exists_and_edge_cases) {
    WorkingMemory wm;
    wm_init(&wm);
    wm_assert(&wm, "sensor_a", value_none());

    const Fact *f = wm_find(&wm, "sensor_a");
    ASSERT(f != NULL);

    Condition *c_exists = condition_create("sensor_a", OP_EXISTS, value_none());
    ASSERT(condition_eval(c_exists, f) == true);

    /* Evaluation against NULL */
    ASSERT(condition_eval(c_exists, NULL) == false);
    ASSERT(condition_eval(NULL, f) == false);

    /* Name mismatch */
    Condition *c_wrong_name = condition_create("sensor_b", OP_EXISTS, value_none());
    ASSERT(condition_eval(c_wrong_name, f) == false);

    /* Incomparable type comparison */
    Condition *c_incomp = condition_create("sensor_a", OP_GT, value_int(10));
    ASSERT(condition_eval(c_incomp, f) == false);

    condition_free(c_exists);
    condition_free(c_wrong_name);
    condition_free(c_incomp);
    wm_free(&wm);
}

TEST(action_creation_and_free) {
    ASSERT_STR_EQ(action_type_to_string(ACT_ASSERT), "ASSERT");
    ASSERT_STR_EQ(action_type_to_string(ACT_RETRACT), "RETRACT");
    ASSERT_STR_EQ(action_type_to_string(ACT_CUSTOM), "CUSTOM");
    ASSERT_STR_EQ(action_type_to_string((ActionType)99), "UNKNOWN");

    Action *a1 = action_create(ACT_ASSERT, "alarm", value_bool(true));
    ASSERT(a1 != NULL);
    ASSERT_EQ_INT(a1->type, ACT_ASSERT);
    ASSERT_STR_EQ(a1->target_name, "alarm");
    ASSERT_EQ_INT(a1->arg_val.type, VAL_BOOL);
    ASSERT(a1->arg_val.as.b == true);
    ASSERT(a1->next == NULL);

    Action *a2 = action_create(ACT_RETRACT, "temp_reading", value_none());
    ASSERT(a2 != NULL);
    ASSERT_EQ_INT(a2->type, ACT_RETRACT);
    ASSERT_STR_EQ(a2->target_name, "temp_reading");
    ASSERT_EQ_INT(a2->arg_val.type, VAL_NONE);

    Action *a3 = action_create(ACT_CUSTOM, "send_email", value_string("admin@example.com"));
    ASSERT(a3 != NULL);
    ASSERT_EQ_INT(a3->type, ACT_CUSTOM);
    ASSERT_STR_EQ(a3->target_name, "send_email");
    ASSERT_STR_EQ(a3->arg_val.as.s, "admin@example.com");

    /* Chain actions and free chain */
    a1->next = a2;
    a2->next = a3;
    action_free(a1);
}

TEST(rule_creation_and_appending) {
    Rule *r = rule_create("high_temp_alert", 100);
    ASSERT(r != NULL);
    ASSERT_STR_EQ(r->name, "high_temp_alert");
    ASSERT_EQ_INT(r->salience, 100);
    ASSERT_EQ_INT(r->condition_count, 0);
    ASSERT(r->conditions == NULL);
    ASSERT_EQ_INT(r->action_count, 0);
    ASSERT(r->actions == NULL);
    ASSERT(r->next == NULL);

    /* Append multiple conditions */
    rule_add_condition(r, "temp", OP_GT, value_int(30));
    rule_add_condition(r, "status", OP_EQ, value_string("active"));
    ASSERT_EQ_INT(r->condition_count, 2);
    ASSERT(r->conditions != NULL);
    ASSERT_STR_EQ(r->conditions->fact_name, "temp");
    ASSERT(r->conditions->next != NULL);
    ASSERT_STR_EQ(r->conditions->next->fact_name, "status");
    ASSERT(r->conditions->next->next == NULL);

    /* Append multiple actions */
    rule_add_action(r, ACT_ASSERT, "alert", value_string("OVERHEAT"));
    rule_add_action(r, ACT_CUSTOM, "log_event", value_int(1));
    ASSERT_EQ_INT(r->action_count, 2);
    ASSERT(r->actions != NULL);
    ASSERT_EQ_INT(r->actions->type, ACT_ASSERT);
    ASSERT_STR_EQ(r->actions->target_name, "alert");
    ASSERT(r->actions->next != NULL);
    ASSERT_EQ_INT(r->actions->next->type, ACT_CUSTOM);
    ASSERT_STR_EQ(r->actions->next->target_name, "log_event");
    ASSERT(r->actions->next->next == NULL);

    rule_free(r);
}

TEST(rule_matches_evaluations) {
    WorkingMemory wm;
    wm_init(&wm);

    wm_assert(&wm, "temp", value_int(35));      /* tick 1 */
    wm_assert(&wm, "humidity", value_int(80));  /* tick 2 */
    wm_assert(&wm, "power", value_bool(true));  /* tick 3 */

    /* Rule matching all conditions */
    Rule *r_match = rule_create("alert_condition", 10);
    rule_add_condition(r_match, "temp", OP_GT, value_int(30));
    rule_add_condition(r_match, "humidity", OP_GTE, value_int(75));
    rule_add_condition(r_match, "power", OP_EQ, value_bool(true));

    uint64_t max_ts = 0;
    ASSERT(rule_matches(r_match, &wm, &max_ts) == true);
    ASSERT_EQ_INT(max_ts, 3);

    /* Rule with failing condition */
    Rule *r_fail_cond = rule_create("cold_alert", 10);
    rule_add_condition(r_fail_cond, "temp", OP_LT, value_int(0));
    ASSERT(rule_matches(r_fail_cond, &wm, &max_ts) == false);

    /* Rule with missing fact */
    Rule *r_missing = rule_create("smoke_alert", 10);
    rule_add_condition(r_missing, "smoke_level", OP_EXISTS, value_none());
    ASSERT(rule_matches(r_missing, &wm, &max_ts) == false);

    /* Rule with 0 conditions vacuously matches with max_ts = 0 */
    Rule *r_empty = rule_create("empty_rule", 0);
    max_ts = 999;
    ASSERT(rule_matches(r_empty, &wm, &max_ts) == true);
    ASSERT_EQ_INT(max_ts, 0);

    /* Test NULL out_max_timestamp safety */
    ASSERT(rule_matches(r_match, &wm, NULL) == true);

    /* NULL rule or wm safety */
    ASSERT(rule_matches(NULL, &wm, &max_ts) == false);
    ASSERT(rule_matches(r_match, NULL, &max_ts) == false);

    rule_free(r_match);
    rule_free(r_fail_cond);
    rule_free(r_missing);
    rule_free(r_empty);
    wm_free(&wm);
}

TEST(rule_matches_timestamp_calculation) {
    WorkingMemory wm;
    wm_init(&wm);

    wm_assert(&wm, "f1", value_int(10)); /* tick 1 */
    wm_assert(&wm, "f2", value_int(20)); /* tick 2 */
    wm_assert(&wm, "f3", value_int(30)); /* tick 3 */

    /* Update f1 to advance its timestamp to 4 */
    wm_assert(&wm, "f1", value_int(15)); /* tick 4 */

    Rule *r = rule_create("ts_test", 5);
    rule_add_condition(r, "f1", OP_EQ, value_int(15));
    rule_add_condition(r, "f2", OP_EQ, value_int(20));

    uint64_t max_ts = 0;
    ASSERT(rule_matches(r, &wm, &max_ts) == true);
    ASSERT_EQ_INT(max_ts, 4); /* max of f1(tick 4) and f2(tick 2) */

    rule_free(r);
    wm_free(&wm);
}

TEST(rulebase_operations) {
    RuleBase rb;
    rb_init(&rb);
    ASSERT_EQ_INT(rb_count(&rb), 0);
    ASSERT(rb.head == NULL);

    Rule *r1 = rule_create("rule_one", 10);
    Rule *r2 = rule_create("rule_two", 20);
    Rule *r3 = rule_create("rule_three", 30);

    rb_add_rule(&rb, r1);
    ASSERT_EQ_INT(rb_count(&rb), 1);
    ASSERT(rb.head == r1);

    rb_add_rule(&rb, r2);
    ASSERT_EQ_INT(rb_count(&rb), 2);
    ASSERT(rb.head->next == r2);

    rb_add_rule(&rb, r3);
    ASSERT_EQ_INT(rb_count(&rb), 3);
    ASSERT(rb.head->next->next == r3);
    ASSERT(r3->next == NULL);

    /* Find rules */
    Rule *found1 = rb_find_rule(&rb, "rule_one");
    ASSERT(found1 == r1);
    Rule *found2 = rb_find_rule(&rb, "rule_two");
    ASSERT(found2 == r2);
    Rule *found3 = rb_find_rule(&rb, "rule_three");
    ASSERT(found3 == r3);

    /* Find non-existent */
    ASSERT(rb_find_rule(&rb, "non_existent") == NULL);
    ASSERT(rb_find_rule(&rb, NULL) == NULL);
    ASSERT(rb_find_rule(NULL, "rule_one") == NULL);

    /* Free RuleBase */
    rb_free(&rb);
    ASSERT_EQ_INT(rb_count(&rb), 0);
    ASSERT(rb.head == NULL);
}

int main(void) {
    printf("Starting Syllogist Rule, Condition, and Action Tests...\n\n");

    RUN_TEST(operator_strings);
    RUN_TEST(condition_creation_and_free);
    RUN_TEST(condition_numeric_evaluations);
    RUN_TEST(condition_string_and_bool_evaluations);
    RUN_TEST(condition_op_exists_and_edge_cases);
    RUN_TEST(action_creation_and_free);
    RUN_TEST(rule_creation_and_appending);
    RUN_TEST(rule_matches_evaluations);
    RUN_TEST(rule_matches_timestamp_calculation);
    RUN_TEST(rulebase_operations);

    return TEST_RUNNER_SUMMARY();
}
