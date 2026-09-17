#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

static int g_test_runner_total = 0;
static int g_test_runner_passed = 0;
static int g_test_runner_failed = 0;
static int g_test_runner_current_failed = 0;
static const char *g_test_runner_current_name = NULL;

#define TEST(name) static void test_##name(void)

#define ASSERT(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "  FAIL: %s:%d: ASSERT(%s) in %s\n", \
                __FILE__, __LINE__, #condition, \
                g_test_runner_current_name ? g_test_runner_current_name : "unknown_test"); \
        g_test_runner_current_failed = 1; \
        return; \
    } \
} while (0)

#define ASSERT_EQ_INT(a, b) do { \
    long long _actual = (long long)(a); \
    long long _expected = (long long)(b); \
    if (_actual != _expected) { \
        fprintf(stderr, "  FAIL: %s:%d: ASSERT_EQ_INT(%s, %s) in %s (expected %lld, got %lld)\n", \
                __FILE__, __LINE__, #a, #b, \
                g_test_runner_current_name ? g_test_runner_current_name : "unknown_test", \
                _expected, _actual); \
        g_test_runner_current_failed = 1; \
        return; \
    } \
} while (0)

#define ASSERT_STR_EQ(a, b) do { \
    const char *_str_a = (a); \
    const char *_str_b = (b); \
    if (_str_a == NULL || _str_b == NULL) { \
        if (_str_a != _str_b) { \
            fprintf(stderr, "  FAIL: %s:%d: ASSERT_STR_EQ(%s, %s) in %s (one is NULL: %p vs %p)\n", \
                    __FILE__, __LINE__, #a, #b, \
                    g_test_runner_current_name ? g_test_runner_current_name : "unknown_test", \
                    (const void *)_str_a, (const void *)_str_b); \
            g_test_runner_current_failed = 1; \
            return; \
        } \
    } else if (strcmp(_str_a, _str_b) != 0) { \
        fprintf(stderr, "  FAIL: %s:%d: ASSERT_STR_EQ(%s, %s) in %s (expected \"%s\", got \"%s\")\n", \
                __FILE__, __LINE__, #a, #b, \
                g_test_runner_current_name ? g_test_runner_current_name : "unknown_test", \
                _str_b, _str_a); \
        g_test_runner_current_failed = 1; \
        return; \
    } \
} while (0)

#define RUN_TEST(name) do { \
    g_test_runner_total++; \
    g_test_runner_current_failed = 0; \
    g_test_runner_current_name = #name; \
    printf("[ RUN      ] %s\n", #name); \
    test_##name(); \
    if (g_test_runner_current_failed) { \
        g_test_runner_failed++; \
        printf("[  FAILED  ] %s\n", #name); \
    } else { \
        g_test_runner_passed++; \
        printf("[       OK ] %s\n", #name); \
    } \
} while (0)

static inline int test_runner_summary(void) {
    printf("\n========================================\n");
    printf("Test Summary: %d total, %d passed, %d failed\n",
           g_test_runner_total, g_test_runner_passed, g_test_runner_failed);
    printf("========================================\n");
    return (g_test_runner_failed == 0) ? 0 : 1;
}

#define TEST_RUNNER_SUMMARY() test_runner_summary()

#endif /* TEST_RUNNER_H */
