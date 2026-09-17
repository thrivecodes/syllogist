#include "test_runner.h"
#include "fact.h"

TEST(value_creation_and_free) {
    /* Int */
    Value vi = value_int(42);
    ASSERT_EQ_INT(vi.type, VAL_INT);
    ASSERT_EQ_INT(vi.as.i, 42);
    value_free(&vi);
    ASSERT_EQ_INT(vi.type, VAL_NONE);

    /* Float */
    Value vf = value_float(3.14);
    ASSERT_EQ_INT(vf.type, VAL_FLOAT);
    ASSERT(vf.as.f > 3.13 && vf.as.f < 3.15);
    value_free(&vf);
    ASSERT_EQ_INT(vf.type, VAL_NONE);

    /* String */
    Value vs = value_string("syllogist");
    ASSERT_EQ_INT(vs.type, VAL_STRING);
    ASSERT_STR_EQ(vs.as.s, "syllogist");
    value_free(&vs);
    ASSERT_EQ_INT(vs.type, VAL_NONE);

    /* Bool */
    Value vb_true = value_bool(true);
    ASSERT_EQ_INT(vb_true.type, VAL_BOOL);
    ASSERT(vb_true.as.b == true);
    value_free(&vb_true);

    Value vb_false = value_bool(false);
    ASSERT_EQ_INT(vb_false.type, VAL_BOOL);
    ASSERT(vb_false.as.b == false);
    value_free(&vb_false);

    /* Symbol */
    Value vsym = value_symbol("turn_on");
    ASSERT_EQ_INT(vsym.type, VAL_SYMBOL);
    ASSERT_STR_EQ(vsym.as.sym, "turn_on");
    value_free(&vsym);
    ASSERT_EQ_INT(vsym.type, VAL_NONE);

    /* None */
    Value vn = value_none();
    ASSERT_EQ_INT(vn.type, VAL_NONE);
    value_free(&vn);
}

TEST(value_cloning) {
    /* String clone: check deep copy */
    Value orig_str = value_string("deep_copy_me");
    Value clone_str = value_clone(&orig_str);
    ASSERT_EQ_INT(clone_str.type, VAL_STRING);
    ASSERT_STR_EQ(clone_str.as.s, "deep_copy_me");
    ASSERT(clone_str.as.s != orig_str.as.s);

    /* Modify clone buffer to prove isolation */
    clone_str.as.s[0] = 'D';
    ASSERT_STR_EQ(clone_str.as.s, "Deep_copy_me");
    ASSERT_STR_EQ(orig_str.as.s, "deep_copy_me");

    value_free(&orig_str);
    value_free(&clone_str);

    /* Symbol clone: check deep copy */
    Value orig_sym = value_symbol("action_alert");
    Value clone_sym = value_clone(&orig_sym);
    ASSERT_EQ_INT(clone_sym.type, VAL_SYMBOL);
    ASSERT_STR_EQ(clone_sym.as.sym, "action_alert");
    ASSERT(clone_sym.as.sym != orig_sym.as.sym);
    value_free(&orig_sym);
    value_free(&clone_sym);

    /* Primitive clone */
    Value vi = value_int(999);
    Value vi_clone = value_clone(&vi);
    ASSERT_EQ_INT(vi_clone.type, VAL_INT);
    ASSERT_EQ_INT(vi_clone.as.i, 999);
    value_free(&vi);
    value_free(&vi_clone);

    /* None clone */
    Value vn = value_none();
    Value vn_clone = value_clone(&vn);
    ASSERT_EQ_INT(vn_clone.type, VAL_NONE);
    value_free(&vn);
    value_free(&vn_clone);
}

TEST(value_equality) {
    /* None */
    Value n1 = value_none();
    Value n2 = value_none();
    ASSERT(value_equal(&n1, &n2));

    /* Int */
    Value i1 = value_int(100);
    Value i2 = value_int(100);
    Value i3 = value_int(200);
    ASSERT(value_equal(&i1, &i2));
    ASSERT(!value_equal(&i1, &i3));

    /* Float */
    Value f1 = value_float(50.5);
    Value f2 = value_float(50.5);
    Value f3 = value_float(50.6);
    ASSERT(value_equal(&f1, &f2));
    ASSERT(!value_equal(&f1, &f3));

    /* Int vs Float numeric equality */
    Value num_i = value_int(42);
    Value num_f = value_float(42.0);
    Value num_f_diff = value_float(42.5);
    ASSERT(value_equal(&num_i, &num_f));
    ASSERT(value_equal(&num_f, &num_i));
    ASSERT(!value_equal(&num_i, &num_f_diff));

    /* String */
    Value s1 = value_string("alpha");
    Value s2 = value_string("alpha");
    Value s3 = value_string("beta");
    ASSERT(value_equal(&s1, &s2));
    ASSERT(!value_equal(&s1, &s3));

    /* Symbol */
    Value sym1 = value_symbol("trigger");
    Value sym2 = value_symbol("trigger");
    Value sym3 = value_symbol("halt");
    ASSERT(value_equal(&sym1, &sym2));
    ASSERT(!value_equal(&sym1, &sym3));

    /* String vs Symbol should not be equal (different ValueTypes) */
    Value s_trig = value_string("trigger");
    ASSERT(!value_equal(&sym1, &s_trig));

    /* Bool */
    Value b1 = value_bool(true);
    Value b2 = value_bool(true);
    Value b3 = value_bool(false);
    ASSERT(value_equal(&b1, &b2));
    ASSERT(!value_equal(&b1, &b3));

    /* Mixed types */
    ASSERT(!value_equal(&i1, &s1));
    ASSERT(!value_equal(&b1, &n1));

    /* Cleanup */
    value_free(&n1); value_free(&n2);
    value_free(&i1); value_free(&i2); value_free(&i3);
    value_free(&f1); value_free(&f2); value_free(&f3);
    value_free(&num_i); value_free(&num_f); value_free(&num_f_diff);
    value_free(&s1); value_free(&s2); value_free(&s3);
    value_free(&sym1); value_free(&sym2); value_free(&sym3);
    value_free(&s_trig);
    value_free(&b1); value_free(&b2); value_free(&b3);
}

TEST(value_comparison) {
    bool comp = false;

    /* Int vs Int */
    Value i1 = value_int(10);
    Value i2 = value_int(20);
    Value i3 = value_int(10);
    ASSERT_EQ_INT(value_compare(&i1, &i2, &comp), -1);
    ASSERT(comp);
    ASSERT_EQ_INT(value_compare(&i2, &i1, &comp), 1);
    ASSERT(comp);
    ASSERT_EQ_INT(value_compare(&i1, &i3, &comp), 0);
    ASSERT(comp);

    /* Float vs Float */
    Value f1 = value_float(1.5);
    Value f2 = value_float(2.5);
    ASSERT_EQ_INT(value_compare(&f1, &f2, &comp), -1);
    ASSERT(comp);
    ASSERT_EQ_INT(value_compare(&f2, &f1, &comp), 1);
    ASSERT(comp);

    /* Int vs Float */
    Value i_ten = value_int(10);
    Value f_nine_nine = value_float(9.9);
    Value f_ten = value_float(10.0);
    Value f_ten_one = value_float(10.1);
    ASSERT_EQ_INT(value_compare(&i_ten, &f_nine_nine, &comp), 1);
    ASSERT(comp);
    ASSERT_EQ_INT(value_compare(&i_ten, &f_ten, &comp), 0);
    ASSERT(comp);
    ASSERT_EQ_INT(value_compare(&i_ten, &f_ten_one, &comp), -1);
    ASSERT(comp);
    ASSERT_EQ_INT(value_compare(&f_nine_nine, &i_ten, &comp), -1);
    ASSERT(comp);

    /* String vs String */
    Value s_apple = value_string("apple");
    Value s_banana = value_string("banana");
    Value s_apple2 = value_string("apple");
    ASSERT_EQ_INT(value_compare(&s_apple, &s_banana, &comp), -1);
    ASSERT(comp);
    ASSERT_EQ_INT(value_compare(&s_banana, &s_apple, &comp), 1);
    ASSERT(comp);
    ASSERT_EQ_INT(value_compare(&s_apple, &s_apple2, &comp), 0);
    ASSERT(comp);

    /* Symbol vs Symbol */
    Value sym_a = value_symbol("abc");
    Value sym_b = value_symbol("xyz");
    ASSERT_EQ_INT(value_compare(&sym_a, &sym_b, &comp), -1);
    ASSERT(comp);

    /* Bool vs Bool */
    Value b_f = value_bool(false);
    Value b_t = value_bool(true);
    ASSERT_EQ_INT(value_compare(&b_f, &b_t, &comp), -1);
    ASSERT(comp);
    ASSERT_EQ_INT(value_compare(&b_t, &b_f, &comp), 1);
    ASSERT(comp);

    /* Incomparable: Int vs String */
    comp = true;
    value_compare(&i1, &s_apple, &comp);
    ASSERT(!comp);

    /* Cleanup */
    value_free(&i1); value_free(&i2); value_free(&i3);
    value_free(&f1); value_free(&f2);
    value_free(&i_ten); value_free(&f_nine_nine); value_free(&f_ten); value_free(&f_ten_one);
    value_free(&s_apple); value_free(&s_banana); value_free(&s_apple2);
    value_free(&sym_a); value_free(&sym_b);
    value_free(&b_f); value_free(&b_t);
}

TEST(value_print) {
    FILE *tmp = tmpfile();
    ASSERT(tmp != NULL);

    Value vi = value_int(12345);
    value_print(&vi, tmp);
    fputc('\n', tmp);

    Value vf = value_float(7.5);
    value_print(&vf, tmp);
    fputc('\n', tmp);

    Value vs = value_string("testing");
    value_print(&vs, tmp);
    fputc('\n', tmp);

    Value vb = value_bool(true);
    value_print(&vb, tmp);
    fputc('\n', tmp);

    Value sym = value_symbol("motor_on");
    value_print(&sym, tmp);
    fputc('\n', tmp);

    Value vn = value_none();
    value_print(&vn, tmp);
    fputc('\n', tmp);

    /* Rewind and verify contents */
    rewind(tmp);
    char buf[128];
    ASSERT(fgets(buf, sizeof(buf), tmp) != NULL);
    ASSERT_STR_EQ(buf, "12345\n");
    ASSERT(fgets(buf, sizeof(buf), tmp) != NULL);
    ASSERT_STR_EQ(buf, "7.5\n");
    ASSERT(fgets(buf, sizeof(buf), tmp) != NULL);
    ASSERT_STR_EQ(buf, "\"testing\"\n");
    ASSERT(fgets(buf, sizeof(buf), tmp) != NULL);
    ASSERT_STR_EQ(buf, "true\n");
    ASSERT(fgets(buf, sizeof(buf), tmp) != NULL);
    ASSERT_STR_EQ(buf, "motor_on\n");
    ASSERT(fgets(buf, sizeof(buf), tmp) != NULL);
    ASSERT_STR_EQ(buf, "none\n");

    fclose(tmp);
    value_free(&vi);
    value_free(&vf);
    value_free(&vs);
    value_free(&vb);
    value_free(&sym);
    value_free(&vn);
}

TEST(wm_init_and_count) {
    WorkingMemory wm;
    wm_init(&wm);

    ASSERT_EQ_INT(wm_count(&wm), 0);
    ASSERT_EQ_INT(wm.clock, 0);
    ASSERT(wm.head == NULL);

    wm_free(&wm);
    ASSERT_EQ_INT(wm_count(&wm), 0);
    ASSERT(wm.head == NULL);
}

TEST(wm_assert_new_facts) {
    WorkingMemory wm;
    wm_init(&wm);

    /* Assert first fact */
    Fact *f1 = wm_assert(&wm, "temperature", value_int(22));
    ASSERT(f1 != NULL);
    ASSERT_STR_EQ(f1->name, "temperature");
    ASSERT_EQ_INT(f1->value.as.i, 22);
    ASSERT_EQ_INT(f1->timestamp, 1);
    ASSERT_EQ_INT(wm.clock, 1);
    ASSERT_EQ_INT(wm_count(&wm), 1);
    ASSERT(wm.head == f1);

    /* Assert second fact: prepends to head */
    Fact *f2 = wm_assert(&wm, "humidity", value_float(65.5));
    ASSERT(f2 != NULL);
    ASSERT_STR_EQ(f2->name, "humidity");
    ASSERT_EQ_INT(f2->timestamp, 2);
    ASSERT_EQ_INT(wm.clock, 2);
    ASSERT_EQ_INT(wm_count(&wm), 2);
    ASSERT(wm.head == f2);
    ASSERT(f2->next == f1);
    ASSERT(f1->next == NULL);

    /* Assert third fact */
    Fact *f3 = wm_assert(&wm, "location", value_string("server_room"));
    ASSERT(f3 != NULL);
    ASSERT_STR_EQ(f3->name, "location");
    ASSERT_EQ_INT(f3->timestamp, 3);
    ASSERT_EQ_INT(wm.clock, 3);
    ASSERT_EQ_INT(wm_count(&wm), 3);
    ASSERT(wm.head == f3);
    ASSERT(f3->next == f2);

    wm_free(&wm);
}

TEST(wm_assert_update_existing) {
    WorkingMemory wm;
    wm_init(&wm);

    wm_assert(&wm, "fan_speed", value_int(100)); /* clock = 1 */
    wm_assert(&wm, "status", value_symbol("standby")); /* clock = 2 */
    ASSERT_EQ_INT(wm_count(&wm), 2);
    ASSERT_EQ_INT(wm.clock, 2);

    /* Update existing fact "fan_speed" */
    Fact *updated = wm_assert(&wm, "fan_speed", value_int(250)); /* clock = 3 */
    ASSERT(updated != NULL);
    ASSERT_STR_EQ(updated->name, "fan_speed");
    ASSERT_EQ_INT(updated->value.as.i, 250);
    ASSERT_EQ_INT(updated->timestamp, 3);
    ASSERT_EQ_INT(wm.clock, 3);
    ASSERT_EQ_INT(wm_count(&wm), 2); /* Count unchanged */

    /* Verify find retrieves the updated value */
    const Fact *found = wm_find(&wm, "fan_speed");
    ASSERT(found != NULL);
    ASSERT_EQ_INT(found->value.as.i, 250);
    ASSERT_EQ_INT(found->timestamp, 3);

    wm_free(&wm);
}

TEST(wm_find_facts) {
    WorkingMemory wm;
    wm_init(&wm);

    wm_assert(&wm, "door_locked", value_bool(true));
    wm_assert(&wm, "occupants", value_int(4));

    const Fact *f_door = wm_find(&wm, "door_locked");
    ASSERT(f_door != NULL);
    ASSERT_EQ_INT(f_door->value.type, VAL_BOOL);
    ASSERT(f_door->value.as.b == true);

    const Fact *f_occ = wm_find(&wm, "occupants");
    ASSERT(f_occ != NULL);
    ASSERT_EQ_INT(f_occ->value.type, VAL_INT);
    ASSERT_EQ_INT(f_occ->value.as.i, 4);

    const Fact *f_none = wm_find(&wm, "non_existent_key");
    ASSERT(f_none == NULL);

    const Fact *f_null = wm_find(&wm, NULL);
    ASSERT(f_null == NULL);

    wm_free(&wm);
}

TEST(wm_retract_facts) {
    WorkingMemory wm;
    wm_init(&wm);

    wm_assert(&wm, "a", value_int(1));
    wm_assert(&wm, "b", value_int(2));
    wm_assert(&wm, "c", value_int(3));
    ASSERT_EQ_INT(wm_count(&wm), 3);

    /* Retract middle element ("b") */
    ASSERT(wm_retract(&wm, "b") == true);
    ASSERT_EQ_INT(wm_count(&wm), 2);
    ASSERT(wm_find(&wm, "b") == NULL);
    ASSERT(wm_find(&wm, "a") != NULL);
    ASSERT(wm_find(&wm, "c") != NULL);

    /* Retract head element ("c") */
    ASSERT(wm_retract(&wm, "c") == true);
    ASSERT_EQ_INT(wm_count(&wm), 1);
    ASSERT(wm_find(&wm, "c") == NULL);
    ASSERT(wm_find(&wm, "a") != NULL);
    ASSERT(wm.head != NULL);
    ASSERT_STR_EQ(wm.head->name, "a");

    /* Retract last element ("a") */
    ASSERT(wm_retract(&wm, "a") == true);
    ASSERT_EQ_INT(wm_count(&wm), 0);
    ASSERT(wm.head == NULL);
    ASSERT(wm_find(&wm, "a") == NULL);

    /* Retract on empty memory or missing fact returns false */
    ASSERT(wm_retract(&wm, "a") == false);
    ASSERT(wm_retract(&wm, "unknown") == false);

    wm_free(&wm);
}

TEST(wm_clock_ticking) {
    WorkingMemory wm;
    wm_init(&wm);

    ASSERT_EQ_INT(wm.clock, 0);

    /* Each assert increments clock */
    Fact *f1 = wm_assert(&wm, "f1", value_int(10));
    ASSERT_EQ_INT(wm.clock, 1);
    ASSERT_EQ_INT(f1->timestamp, 1);

    Fact *f2 = wm_assert(&wm, "f2", value_int(20));
    ASSERT_EQ_INT(wm.clock, 2);
    ASSERT_EQ_INT(f2->timestamp, 2);

    /* Updating existing fact advances clock */
    Fact *f1_up = wm_assert(&wm, "f1", value_int(15));
    ASSERT(f1_up == f1);
    ASSERT_EQ_INT(wm.clock, 3);
    ASSERT_EQ_INT(f1->timestamp, 3);

    /* Retract does not advance clock */
    wm_retract(&wm, "f2");
    ASSERT_EQ_INT(wm.clock, 3);

    /* Next assert continues monotonicity */
    Fact *f3 = wm_assert(&wm, "f3", value_int(30));
    ASSERT_EQ_INT(wm.clock, 4);
    ASSERT_EQ_INT(f3->timestamp, 4);

    wm_free(&wm);
}

TEST(wm_dump_output) {
    WorkingMemory wm;
    wm_init(&wm);

    wm_assert(&wm, "temp", value_int(24));
    wm_assert(&wm, "mode", value_symbol("cooling"));

    FILE *tmp = tmpfile();
    ASSERT(tmp != NULL);
    wm_dump(&wm, tmp);

    rewind(tmp);
    char line[256];
    bool saw_header = false;
    bool saw_temp = false;
    bool saw_mode = false;

    while (fgets(line, sizeof(line), tmp)) {
        if (strstr(line, "=== Working Memory")) saw_header = true;
        if (strstr(line, "temp = 24")) saw_temp = true;
        if (strstr(line, "mode = cooling")) saw_mode = true;
    }

    ASSERT(saw_header);
    ASSERT(saw_temp);
    ASSERT(saw_mode);

    fclose(tmp);
    wm_free(&wm);
}

int main(void) {
    printf("Starting Syllogist Fact and WorkingMemory Tests...\n\n");

    RUN_TEST(value_creation_and_free);
    RUN_TEST(value_cloning);
    RUN_TEST(value_equality);
    RUN_TEST(value_comparison);
    RUN_TEST(value_print);
    RUN_TEST(wm_init_and_count);
    RUN_TEST(wm_assert_new_facts);
    RUN_TEST(wm_assert_update_existing);
    RUN_TEST(wm_find_facts);
    RUN_TEST(wm_retract_facts);
    RUN_TEST(wm_clock_ticking);
    RUN_TEST(wm_dump_output);

    return TEST_RUNNER_SUMMARY();
}
