/**
 * @file test_driver.c
 * @brief Driver模块单元测试
 * @version 1.0
 * @date 2026-02-09
 */

#include "test_framework.h"
#include "../../inc/driver/tk8710.h"
#include "../../inc/driver/tk8710_internal.h"
#include "../../inc/driver/tk8710_log.h"
#include <string.h>

/*==============================================================================
 * 测试回调函数
 *============================================================================*/

static int g_slotEndCount = 0;
static int g_rxDataCount = 0;
static int g_txSlotCount = 0;
static int g_errorCount = 0;

static void TestOnSlotEnd(uint8_t slotType, uint8_t slotIndex, uint32_t frameNo)
{
    (void)slotType;
    (void)slotIndex;
    (void)frameNo;
    g_slotEndCount++;
}

static void TestOnRxData(const TK8710IrqResult* rxResult)
{
    (void)rxResult;
    g_rxDataCount++;
}

static void TestOnTxSlot(uint8_t slotIndex, uint8_t maxUserCount)
{
    (void)slotIndex;
    (void)maxUserCount;
    g_txSlotCount++;
}

static void TestOnError(int errorCode)
{
    (void)errorCode;
    g_errorCount++;
}

static void ResetTestCounters(void)
{
    g_slotEndCount = 0;
    g_rxDataCount = 0;
    g_txSlotCount = 0;
    g_errorCount = 0;
}

/*==============================================================================
 * Driver初始化测�? *============================================================================*/

TEST_CASE(driver_init_null_config)
{
    int ret = TK8710Init(NULL, NULL);
    TEST_ASSERT_EQ(ret, TK8710_ERR_PARAM, "Init with NULL config should fail");
}

TEST_CASE(driver_init_valid_config)
{
    ChipConfig config = {0};
    config.interval = 32;
    config.ant_en = 0xFF;
    config.rf_sel = 0xFF;
    config.tx_bcn_en = 1;
    config.conti_mode = 1;
    
    int ret = TK8710Init(&config, NULL);
    TEST_ASSERT_EQ(ret, TK8710_OK, "Init with valid config should succeed");
    
    TK8710ResetChip(1);
}

TEST_CASE(driver_double_init)
{
    ChipConfig config = {0};
    config.interval = 32;
    config.ant_en = 0xFF;
    
    int ret1 = TK8710Init(&config, NULL);
    TEST_ASSERT_EQ(ret1, TK8710_OK, "First init should succeed");
    
    int ret2 = TK8710Init(&config, NULL);
    TEST_ASSERT_EQ(ret2, TK8710_ERR_STATE, "Double init should fail");
    
    TK8710ResetChip(1);
}

/*==============================================================================
 * Driver启动/停止测试
 *============================================================================*/

TEST_CASE(driver_start_stop)
{
    ChipConfig config = {0};
    config.interval = 32;
    config.ant_en = 0xFF;
    config.conti_mode = 1;
    
    TK8710Init(&config, NULL);
    
    int ret = TK8710Start(1, 1);
    TEST_ASSERT_EQ(ret, TK8710_OK, "Start should succeed");
    
    // 使用复位代替停止，因为没有停止控制类型
    TK8710ResetChip(1);
}

TEST_CASE(driver_start_without_init)
{
    int ret = TK8710Start(1, 1);
    TEST_ASSERT_EQ(ret, TK8710_ERR, "Start without init should fail");
}

/*==============================================================================
 * 调试功能测试
 *============================================================================*/

TEST_CASE(debug_irq_counters)
{
    TK8710ResetIrqCounters();
    
    uint32_t counters[10];
    TK8710GetAllIrqCounters(counters);
    
    int allZero = 1;
    for (int i = 0; i < 10; i++) {
        if (counters[i] != 0) {
            allZero = 0;
            break;
        }
    }
    TEST_ASSERT(allZero, "All counters should be zero after reset");
}

TEST_CASE(debug_irq_time_stats)
{
    TK8710ResetIrqTimeStats(0xFF);
    
    uint32_t totalTime, maxTime, minTime, count;
    int ret = TK8710GetIrqTimeStats(0, &totalTime, &maxTime, &minTime, &count);
    TEST_ASSERT_EQ(ret, 0, "GetIrqTimeStats should succeed");
    TEST_ASSERT_EQ(count, 0, "Count should be zero after reset");
    TEST_ASSERT_EQ(totalTime, 0, "TotalTime should be zero after reset");
}

TEST_CASE(debug_force_process)
{
    TK8710SetForceProcessAllUsers(1);
    TEST_ASSERT_EQ(TK8710GetForceProcessAllUsers(), 1, "Force process should be enabled");
    
    TK8710SetForceProcessAllUsers(0);
    TEST_ASSERT_EQ(TK8710GetForceProcessAllUsers(), 0, "Force process should be disabled");
}

TEST_CASE(debug_force_max_tx)
{
    TK8710SetForceMaxUsersTx(1);
    TEST_ASSERT_EQ(TK8710GetForceMaxUsersTx(), 1, "Force max TX should be enabled");
    
    TK8710SetForceMaxUsersTx(0);
    TEST_ASSERT_EQ(TK8710GetForceMaxUsersTx(), 0, "Force max TX should be disabled");
}

/*==============================================================================
 * 日志模块测试
 *============================================================================*/

TEST_CASE(log_init)
{
    int ret = TK8710LogSimpleInit(TK8710_LOG_DEBUG, TK8710_LOG_MODULE_ALL);
    TEST_ASSERT_EQ(ret, 0, "Log init should succeed");
    TEST_ASSERT_EQ(TK8710LogGetLevel(), TK8710_LOG_DEBUG, "Log level should be DEBUG");
    TEST_ASSERT_EQ(TK8710LogGetModuleMask(), TK8710_LOG_MODULE_ALL, "Module mask should be ALL");
}

TEST_CASE(log_set_level)
{
    TK8710LogSetLevel(TK8710_LOG_ERROR);
    TEST_ASSERT_EQ(TK8710LogGetLevel(), TK8710_LOG_ERROR, "Log level should be ERROR");
    
    TK8710LogSetLevel(TK8710_LOG_INFO);
    TEST_ASSERT_EQ(TK8710LogGetLevel(), TK8710_LOG_INFO, "Log level should be INFO");
}

TEST_CASE(log_set_module_mask)
{
    TK8710LogSetModuleMask(TK8710_LOG_MODULE_CORE | TK8710_LOG_MODULE_IRQ);
    uint32_t mask = TK8710LogGetModuleMask();
    TEST_ASSERT(mask & TK8710_LOG_MODULE_CORE, "CORE module should be enabled");
    TEST_ASSERT(mask & TK8710_LOG_MODULE_IRQ, "IRQ module should be enabled");
    TEST_ASSERT(!(mask & 0x10), "TRM module should be disabled");
}

/*==============================================================================
 * 主函�? *============================================================================*/

int main(void)
{
    printf("\n");
    printf("TK8710 HAL Driver Unit Tests\n");
    printf("============================\n");
    
    /* 初始化日�?*/
    TK8710LogSimpleInit(TK8710_LOG_WARN, TK8710_LOG_MODULE_ALL);
    
    /* Driver初始化测�?*/
    TEST_SUITE_BEGIN("Driver Init Tests");
    RUN_TEST(driver_init_null_config);
    RUN_TEST(driver_init_valid_config);
    RUN_TEST(driver_double_init);
    TEST_SUITE_END();
    
    /* Driver启动/停止测试 */
    TEST_SUITE_BEGIN("Driver Start/Stop Tests");
    RUN_TEST(driver_start_stop);
    RUN_TEST(driver_start_without_init);
    TEST_SUITE_END();
    
    /* 调试功能测试 */
    TEST_SUITE_BEGIN("Debug Function Tests");
    RUN_TEST(debug_irq_counters);
    RUN_TEST(debug_irq_time_stats);
    RUN_TEST(debug_force_process);
    RUN_TEST(debug_force_max_tx);
    TEST_SUITE_END();
    
    /* 日志模块测试 */
    TEST_SUITE_BEGIN("Log Module Tests");
    RUN_TEST(log_init);
    RUN_TEST(log_set_level);
    RUN_TEST(log_set_module_mask);
    TEST_SUITE_END();
    
    printf("\n");
    return TEST_RESULT();
}
