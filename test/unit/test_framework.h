/**
 * @file test_framework.h
 * @brief 简单的测试框架定义
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>

/* 测试统计变量 */
static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

/* 测试宏定义 */
#define TEST_CASE(name) void test_##name(void)
#define TEST_ASSERT_EQ(actual, expected, message) \
    do { \
        if ((actual) != (expected)) { \
            printf("FAIL: %s - %s (expected: %d, actual: %d)\n", __FUNCTION__, message, (int)(expected), (int)(actual)); \
            g_tests_failed++; \
        } else { \
            printf("PASS: %s - %s\n", __FUNCTION__, message); \
            g_tests_passed++; \
        } \
        g_tests_run++; \
    } while(0)

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s - %s\n", __FUNCTION__, message); \
            g_tests_failed++; \
        } else { \
            printf("PASS: %s - %s\n", __FUNCTION__, message); \
            g_tests_passed++; \
        } \
        g_tests_run++; \
    } while(0)

#define TEST_ASSERT_NE(actual, expected, message) \
    do { \
        if ((actual) == (expected)) { \
            printf("FAIL: %s - %s (should not equal: %d)\n", __FUNCTION__, message, (int)(expected)); \
            g_tests_failed++; \
        } else { \
            printf("PASS: %s - %s\n", __FUNCTION__, message); \
            g_tests_passed++; \
        } \
        g_tests_run++; \
    } while(0)

#define RUN_TEST(name) \
    do { \
        printf("  Running %s...\n", #name); \
        test_##name(); \
    } while(0)

#define TEST_SUITE_BEGIN(name) \
    printf("\n%s\n", name); \
    printf("----------------------------\n")

#define TEST_SUITE_END() \
    printf("Suite completed: %d/%d tests passed\n\n", g_tests_passed, g_tests_run)

#define TEST_RESULT() \
    ((g_tests_failed == 0) ? 0 : 1)

#endif /* TEST_FRAMEWORK_H */
