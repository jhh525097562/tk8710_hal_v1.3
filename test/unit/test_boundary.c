/**
 * @file test_boundary.c
 * @brief 边界条件和压力测�? * @version 1.0
 * @date 2026-02-09
 */

#include "test_framework.h"
#include "trm/trm.h"
#include "driver/tk8710.h"
#include "tk8710_internal.h"
#include "driver/tk8710_log.h"
#include <string.h>
#include <stdlib.h>

/*==============================================================================
 * 辅助函数
 *============================================================================*/

static TRM_InitConfig g_defaultConfig;

static void SetupDefaultConfig(void)
{
    memset(&g_defaultConfig, 0, sizeof(g_defaultConfig));
    g_defaultConfig.beamMode = TRM_BEAM_MODE_FULL_STORE;
    g_defaultConfig.beamMaxUsers = 100;
    g_defaultConfig.beamTimeoutMs = 5000;
    
    /* 创建默认的芯片配置、时隙配置和射频配置 */
    static ChipConfig defaultChipConfig = {
        .bcn_agc = 32,
        .interval = 32,
        .tx_dly = 1,
        .tx_fix_info = 0,
        .offset_adj = 0,
        .tx_pre = 0,
        .conti_mode = 1,
        .bcn_scan = 0,
        .ant_en = 0xFF,
        .rf_sel = 0xFF,
        .tx_bcn_en = 1,
        .ts_sync = 0,
        .rf_model = 1,
        .bcnbits = 0x1F,
        .anoiseThe1 = 0,
        .power2rssi = 0,
        .irq_ctrl0 = 0,
        .irq_ctrl1 = 0
    };
    
    static slotCfg_t defaultSlotConfig = {
        .msMode = TK8710_MODE_MASTER,
        .plCrcEn = 1,
        .rateCount = 1,
        .rateModes = {TK8710_RATE_MODE_8},
        .brdUserNum = 1,
        .antEn = 0xFF,
        .rfSel = 0xFF,
        .txAutoMode = 0,
        .txBcnEn = 1,
        .bcnRotation = {0, 1, 2, 3, 4, 5, 6, 7},
        .rx_delay = 0,
        .md_agc = 1024,
        .local_sync = 0
    };
    
    static ChiprfConfig defaultRfConfig = {
        .rftype = TK8710_RF_TYPE_1255_1M,
        .Freq = 2400000000,
        .rxgain = 1,
        .txgain = 1,
        .txadc = {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}
    };
    
    g_defaultConfig.chipConfig = &defaultChipConfig;
    g_defaultConfig.slotConfig = &defaultSlotConfig;
    g_defaultConfig.rfConfig = &defaultRfConfig;
}

/*==============================================================================
 * 边界条件测试 - 波束管理
 *============================================================================*/

TEST_CASE(beam_max_users)
{
    SetupDefaultConfig();
    g_defaultConfig.beamMaxUsers = 10;  /* 限制最大用户数�?0 */
    TRM_Init(&g_defaultConfig);
    
    /* 添加10个用户应该成�?*/
    int successCount = 0;
    for (int i = 0; i < 10; i++) {
        TRM_BeamInfo beam;
        memset(&beam, 0, sizeof(beam));
        beam.userId = 0x1000 + i;
        if (TRM_SetBeamInfo(beam.userId, &beam) == TRM_OK) {
            successCount++;
        }
    }
    TEST_ASSERT_EQ(successCount, 10, "Should add 10 beams successfully");
    
    /* �?1个应该失�?*/
    TRM_BeamInfo beam;
    memset(&beam, 0, sizeof(beam));
    beam.userId = 0x2000;
    int ret = TRM_SetBeamInfo(beam.userId, &beam);
    TEST_ASSERT_EQ(ret, TRM_ERR_NO_MEM, "11th beam should fail with NO_MEM");
    
    TRM_Deinit();
}

TEST_CASE(beam_update_existing)
{
    SetupDefaultConfig();
    TRM_Init(&g_defaultConfig);
    
    /* 添加波束 */
    TRM_BeamInfo beam1;
    memset(&beam1, 0, sizeof(beam1));
    beam1.userId = 0x12345678;
    beam1.freq = 2400000000;
    beam1.pilotPower = 100;
    TRM_SetBeamInfo(0x12345678, &beam1);
    
    /* 更新同一用户的波�?*/
    TRM_BeamInfo beam2;
    memset(&beam2, 0, sizeof(beam2));
    beam2.userId = 0x12345678;
    beam2.freq = 2450000000;
    beam2.pilotPower = 200;
    int ret = TRM_SetBeamInfo(0x12345678, &beam2);
    TEST_ASSERT_EQ(ret, TRM_OK, "Update existing beam should succeed");
    
    /* 验证更新后的�?*/
    TRM_BeamInfo beamOut;
    TRM_GetBeamInfo(0x12345678, &beamOut);
    TEST_ASSERT_EQ(beamOut.freq, 2450000000, "Freq should be updated");
    TEST_ASSERT_EQ(beamOut.pilotPower, 200, "PilotPower should be updated");
    
    TRM_Deinit();
}

TEST_CASE(beam_null_param)
{
    SetupDefaultConfig();
    TRM_Init(&g_defaultConfig);
    
    int ret = TRM_SetBeamInfo(0x12345678, NULL);
    TEST_ASSERT_EQ(ret, TRM_ERR_PARAM, "SetBeamInfo with NULL should fail");
    
    ret = TRM_GetBeamInfo(0x12345678, NULL);
    TEST_ASSERT_EQ(ret, TRM_ERR_PARAM, "GetBeamInfo with NULL should fail");
    
    TRM_Deinit();
}

/*==============================================================================
 * 边界条件测试 - 数据发�? *============================================================================*/

TEST_CASE(send_data_max_length)
{
    SetupDefaultConfig();
    TRM_Init(&g_defaultConfig);
    TRM_Start();
    
    /* 发送最大长度数�?(512字节) */
    uint8_t maxData[512];
    memset(maxData, 0xAA, sizeof(maxData));
    int ret = TRM_SendData(0x12345678, maxData, 512, 20, 0, 0);
    TEST_ASSERT_EQ(ret, TRM_OK, "Send max length data should succeed");
    
    /* 发送超过最大长度应该失�?*/
    uint8_t overData[513];
    ret = TRM_SendData(0x12345678, overData, 513, 20, 0, 0);
    TEST_ASSERT_EQ(ret, TRM_ERR_PARAM, "Send over max length should fail");
    
    TRM_Stop();
    TRM_Deinit();
}

TEST_CASE(send_data_queue_full)
{
    SetupDefaultConfig();
    TRM_Init(&g_defaultConfig);
    TRM_Start();
    
    /* 填满发送队�?(64�? */
    uint8_t data[16] = {0};
    int successCount = 0;
    for (int i = 0; i < 64; i++) {
        if (TRM_SendData(0x12345678, data, sizeof(data), 20, 0, 0) == TRM_OK) {
            successCount++;
        }
    }
    TEST_ASSERT_EQ(successCount, 64, "Should queue 64 packets");
    
    /* 第65个应该失败 */
    int ret = TRM_SendData(0x12345678, data, sizeof(data), 20, 0, 0);
    TEST_ASSERT_EQ(ret, TRM_ERR_QUEUE_FULL, "65th packet should fail with QUEUE_FULL");
    
    /* 清除队列后应该可以再发�?*/
    TRM_ClearTxData(0xFFFFFFFF);
    ret = TRM_SendData(0x12345678, data, sizeof(data), 20, 0, 0);
    TEST_ASSERT_EQ(ret, TRM_OK, "Send after clear should succeed");
    
    TRM_Stop();
    TRM_Deinit();
}

/*==============================================================================
 * 边界条件测试 - 状态机
 *============================================================================*/

TEST_CASE(state_machine_transitions)
{
    SetupDefaultConfig();
    
    /* 未初始化状�?*/
    int ret = TRM_Start();
    TEST_ASSERT_NE(ret, TRM_OK, "Start without init should fail");
    
    ret = TRM_Stop();
    TEST_ASSERT_NE(ret, TRM_OK, "Stop without init should fail");
    
    /* 初始化后 */
    TRM_Init(&g_defaultConfig);
    TEST_ASSERT_EQ(TRM_IsRunning(), 0, "Should not be running after init");
    
    /* 启动 */
    ret = TRM_Start();
    TEST_ASSERT_EQ(ret, TRM_OK, "Start should succeed");
    TEST_ASSERT_EQ(TRM_IsRunning(), 1, "Should be running after start");
    
    /* 重复启动 */
    ret = TRM_Start();
    TEST_ASSERT_NE(ret, TRM_OK, "Double start should fail");
    
    /* 停止 */
    ret = TRM_Stop();
    TEST_ASSERT_EQ(ret, TRM_OK, "Stop should succeed");
    TEST_ASSERT_EQ(TRM_IsRunning(), 0, "Should not be running after stop");
    
    /* 重复停止 */
    ret = TRM_Stop();
    TEST_ASSERT_NE(ret, TRM_OK, "Double stop should fail");
    
    /* 停止后重新启�?*/
    ret = TRM_Start();
    TEST_ASSERT_EQ(ret, TRM_OK, "Restart should succeed");
    
    TRM_Stop();
    TRM_Deinit();
}

/*==============================================================================
 * 压力测试 - 波束管理
 *============================================================================*/

TEST_CASE(stress_beam_add_remove)
{
    SetupDefaultConfig();
    g_defaultConfig.beamMaxUsers = 1000;
    TRM_Init(&g_defaultConfig);
    
    /* 添加500个用�?*/
    for (int i = 0; i < 500; i++) {
        TRM_BeamInfo beam;
        memset(&beam, 0, sizeof(beam));
        beam.userId = 0x10000 + i;
        beam.freq = 2400000000 + i;
        TRM_SetBeamInfo(beam.userId, &beam);
    }
    
    /* 验证都能查到 */
    int foundCount = 0;
    for (int i = 0; i < 500; i++) {
        TRM_BeamInfo beam;
        if (TRM_GetBeamInfo(0x10000 + i, &beam) == TRM_OK) {
            foundCount++;
        }
    }
    TEST_ASSERT_EQ(foundCount, 500, "Should find all 500 beams");
    
    /* 删除偶数用户 */
    for (int i = 0; i < 500; i += 2) {
        TRM_ClearBeamInfo(0x10000 + i);
    }
    
    /* 验证奇数用户还在 */
    foundCount = 0;
    for (int i = 1; i < 500; i += 2) {
        TRM_BeamInfo beam;
        if (TRM_GetBeamInfo(0x10000 + i, &beam) == TRM_OK) {
            foundCount++;
        }
    }
    TEST_ASSERT_EQ(foundCount, 250, "Should find 250 odd beams");
    
    /* 验证偶数用户已删�?*/
    foundCount = 0;
    for (int i = 0; i < 500; i += 2) {
        TRM_BeamInfo beam;
        if (TRM_GetBeamInfo(0x10000 + i, &beam) == TRM_OK) {
            foundCount++;
        }
    }
    TEST_ASSERT_EQ(foundCount, 0, "Should not find any even beams");
    
    TRM_Deinit();
}

TEST_CASE(stress_beam_update)
{
    SetupDefaultConfig();
    TRM_Init(&g_defaultConfig);
    
    /* 对同一用户反复更新波束 */
    for (int i = 0; i < 1000; i++) {
        TRM_BeamInfo beam;
        memset(&beam, 0, sizeof(beam));
        beam.userId = 0xAAAAAAAA;
        beam.freq = 2400000000 + i;
        beam.pilotPower = i;
        TRM_SetBeamInfo(0xAAAAAAAA, &beam);
    }
    
    /* 验证最后的�?*/
    TRM_BeamInfo beamOut;
    int ret = TRM_GetBeamInfo(0xAAAAAAAA, &beamOut);
    TEST_ASSERT_EQ(ret, TRM_OK, "Should find beam after 1000 updates");
    TEST_ASSERT_EQ(beamOut.freq, 2400000999, "Freq should be last value");
    TEST_ASSERT_EQ(beamOut.pilotPower, 999, "PilotPower should be last value");
    
    TRM_Deinit();
}

/*==============================================================================
 * 压力测试 - 数据发�? *============================================================================*/

TEST_CASE(stress_send_clear_cycle)
{
    SetupDefaultConfig();
    TRM_Init(&g_defaultConfig);
    TRM_Start();
    
    uint8_t data[32];
    memset(data, 0x55, sizeof(data));
    
    /* 反复填满队列并清�?*/
    for (int cycle = 0; cycle < 10; cycle++) {
        /* 填满队列 */
        int queued = 0;
        for (int i = 0; i < 64; i++) {
            if (TRM_SendData(0x12345678, data, sizeof(data), 20, 0, 0) == TRM_OK) {
                queued++;
            }
        }
        TEST_ASSERT_EQ(queued, 64, "Should queue 64 packets in each cycle");
        
        /* 清除队列 */
        TRM_ClearTxData(0xFFFFFFFF);
    }
    
    /* 最后验证队列可用 */
    int ret = TRM_SendData(0x12345678, data, sizeof(data), 20, 0);
    TEST_ASSERT_EQ(ret, TRM_OK, "Queue should be usable after stress test");
    
    TRM_Stop();
    TRM_Deinit();
}

/*==============================================================================
 * 压力测试 - 调试功能
 *============================================================================*/

TEST_CASE(stress_debug_counters)
{
    /* 反复重置和获取计数器 */
    for (int i = 0; i < 100; i++) {
        TK8710ResetIrqCounters();
        
        uint32_t counters[10];
        TK8710GetAllIrqCounters(counters);
        
        /* 验证都是0 */
        int allZero = 1;
        for (int j = 0; j < 10; j++) {
            if (counters[j] != 0) {
                allZero = 0;
                break;
            }
        }
        if (!allZero) {
            TEST_ASSERT(0, "Counters should be zero after reset");
            return;
        }
    }
    TEST_ASSERT(1, "100 reset cycles completed successfully");
}

TEST_CASE(stress_debug_time_stats)
{
    /* 反复重置和获取时间统�?*/
    for (int i = 0; i < 100; i++) {
        TK8710ResetIrqTimeStats(0xFF);
        
        for (int j = 0; j < 10; j++) {
            uint32_t totalTime, maxTime, minTime, count;
            TK8710GetIrqTimeStats(j, &totalTime, &maxTime, &minTime, &count);
            
            if (count != 0 || totalTime != 0) {
                TEST_ASSERT(0, "Stats should be zero after reset");
                return;
            }
        }
    }
    TEST_ASSERT(1, "100 time stats reset cycles completed successfully");
}

/*==============================================================================
 * 主函�? *============================================================================*/

int main(void)
{
    printf("\n");
    printf("TK8710 HAL Boundary & Stress Tests\n");
    printf("===================================\n");
    
    /* 初始化日�?*/
    TK8710LogSimpleConfig(TK8710_LOG_WARN, TK8710_LOG_MODULE_ALL);
    
    /* 边界条件测试 - 波束管理 */
    TEST_SUITE_BEGIN("Beam Boundary Tests");
    RUN_TEST(beam_max_users);
    RUN_TEST(beam_update_existing);
    RUN_TEST(beam_null_param);
    TEST_SUITE_END();
    
    /* 边界条件测试 - 数据发�?*/
    TEST_SUITE_BEGIN("Data Send Boundary Tests");
    RUN_TEST(send_data_max_length);
    RUN_TEST(send_data_queue_full);
    TEST_SUITE_END();
    
    /* 边界条件测试 - 状态机 */
    TEST_SUITE_BEGIN("State Machine Tests");
    RUN_TEST(state_machine_transitions);
    TEST_SUITE_END();
    
    /* 压力测试 - 波束管理 */
    TEST_SUITE_BEGIN("Beam Stress Tests");
    RUN_TEST(stress_beam_add_remove);
    RUN_TEST(stress_beam_update);
    TEST_SUITE_END();
    
    /* 压力测试 - 数据发�?*/
    TEST_SUITE_BEGIN("Data Send Stress Tests");
    RUN_TEST(stress_send_clear_cycle);
    TEST_SUITE_END();
    
    /* 压力测试 - 调试功能 */
    TEST_SUITE_BEGIN("Debug Stress Tests");
    RUN_TEST(stress_debug_counters);
    RUN_TEST(stress_debug_time_stats);
    TEST_SUITE_END();
    
    printf("\n");
    return TEST_RESULT();
}
