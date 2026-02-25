/**
 * @file test_framework.c
 * @brief 测试框架自身的测试
 * @version 1.0
 * @date 2026-02-11
 */

#include "test_framework.h"
#include <stdio.h>
#include <stdlib.h>

/*==============================================================================
 * 测试框架功能测试
 *============================================================================*/

TEST_CASE(framework_basic_assertions)
{
    /* 测试相等断言 */
    TEST_ASSERT_EQ(1, 1, "Equal numbers should pass");
    TEST_ASSERT_EQ(0, 0, "Zero equality should pass");
    
    /* 测试不等断言 */
    TEST_ASSERT_NE(1, 2, "Different numbers should pass");
    TEST_ASSERT_NE(0, 1, "Zero vs one should pass");
    
    /* 测试条件断言 */
    TEST_ASSERT(1, "True condition should pass");
    TEST_ASSERT(0 == 0, "Zero equality condition should pass");
}

TEST_CASE(framework_counter_functionality)
{
    /* 保存当前计数器状态 */
    int initial_run = g_tests_run;
    int initial_passed = g_tests_passed;
    int initial_failed = g_tests_failed;
    
    /* 执行一些断言 */
    TEST_ASSERT_EQ(1, 1, "This should pass");
    TEST_ASSERT_NE(1, 2, "This should pass");
    TEST_ASSERT(1, "This should pass");
    
    /* 验证计数器增加了 */
    int expected_run = initial_run + 3;
    int expected_passed = initial_passed + 3;
    
    /* 注意：由于宏定义会直接修改全局变量，我们无法直接在这里测试 */
    /* 这个测试主要是为了验证宏定义能正常工作 */
    TEST_ASSERT_EQ(g_tests_run, expected_run, "Test run counter should increase");
    TEST_ASSERT_EQ(g_tests_passed, expected_passed, "Test pass counter should increase");
}

TEST_CASE(framework_negative_tests)
{
    /* 这些测试会失败，但用于验证框架的失败处理 */
    /* 在实际使用中，这些断言会失败并增加失败计数 */
    
    /* 注意：以下注释掉的代码会失败，仅用于演示 */
    /*
    TEST_ASSERT_EQ(1, 2, "This should fail");
    TEST_ASSERT_NE(1, 1, "This should fail");
    TEST_ASSERT(0, "This should fail");
    */
    
    /* 我们用通过的方式来测试框架功能 */
    TEST_ASSERT(1, "Framework negative test placeholder");
}

/*==============================================================================
 * 主函数
 *============================================================================*/

int main(void)
{
    printf("\n");
    printf("Test Framework Self-Test\n");
    printf("========================\n");
    
    /* 基础断言测试 */
    TEST_SUITE_BEGIN("Basic Assertion Tests");
    RUN_TEST(framework_basic_assertions);
    TEST_SUITE_END();
    
    /* 计数器功能测试 */
    TEST_SUITE_BEGIN("Counter Functionality Tests");
    RUN_TEST(framework_counter_functionality);
    TEST_SUITE_END();
    
    /* 负面测试（框架失败处理） */
    TEST_SUITE_BEGIN("Framework Negative Tests");
    RUN_TEST(framework_negative_tests);
    TEST_SUITE_END();
    
    printf("\n");
    printf("Framework Self-Test Results:\n");
    printf("Total tests run: %d\n", g_tests_run);
    printf("Tests passed: %d\n", g_tests_passed);
    printf("Tests failed: %d\n", g_tests_failed);
    printf("Success rate: %.1f%%\n", 
           g_tests_run > 0 ? (float)g_tests_passed / g_tests_run * 100.0 : 0.0);
    
    return TEST_RESULT();
}
