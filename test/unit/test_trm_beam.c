/**
 * @file test_trm_beam.c
 * @brief TRM波束资源管理单元测试
 * @note 测试波束分配、存储、查询等功能
 */

#include "test_framework.h"
#include "trm/trm.h"
#include "driver/tk8710.h"
#include "common/tk8710_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
 * 波束管理功能测试
 *============================================================================*/

TEST_CASE(beam_set_get_basic)
{
    SetupDefaultConfig();
    TRM_Init(&g_defaultConfig);
    
    /* 设置波束信息 */
    TRM_BeamInfo beam;
    memset(&beam, 0, sizeof(beam));
    beam.userId = 0x12345678;
    beam.freq = 2400000000;
    beam.pilotPower = 100;
    beam.timestamp = 12345;
    beam.valid = 1;
    
    /* 设置AH数据 */
    for (int i = 0; i < 16; i++) {
        beam.ahData[i] = 0x1000 + i;
    }
    
    int ret = TRM_SetBeamInfo(beam.userId, &beam);
    TEST_ASSERT_EQ(ret, TRM_OK, "Set beam info should succeed");
    
    /* 获取波束信息 */
    TRM_BeamInfo beamOut;
    ret = TRM_GetBeamInfo(beam.userId, &beamOut);
    TEST_ASSERT_EQ(ret, TRM_OK, "Get beam info should succeed");
    
    /* 验证波束信息 */
    TEST_ASSERT_EQ(beamOut.userId, beam.userId, "User ID should match");
    TEST_ASSERT_EQ(beamOut.freq, beam.freq, "Frequency should match");
    TEST_ASSERT_EQ(beamOut.pilotPower, beam.pilotPower, "Pilot power should match");
    TEST_ASSERT_EQ(beamOut.valid, beam.valid, "Valid flag should match");
    
    /* 验证AH数据 */
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT_EQ(beamOut.ahData[i], beam.ahData[i], "AH data should match");
    }
    
    TRM_Deinit();
}

TEST_CASE(beam_update_existing)
{
    SetupDefaultConfig();
    TRM_Init(&g_defaultConfig);
    
    /* 设置初始波束 */
    TRM_BeamInfo beam1;
    memset(&beam1, 0, sizeof(beam1));
    beam1.userId = 0x12345678;
    beam1.freq = 2400000000;
    beam1.pilotPower = 100;
    TRM_SetBeamInfo(beam1.userId, &beam1);
    
    /* 更新波束信息 */
    TRM_BeamInfo beam2;
    memset(&beam2, 0, sizeof(beam2));
    beam2.userId = 0x12345678;
    beam2.freq = 2450000000;
    beam2.pilotPower = 200;
    beam2.timestamp = 54321;
    
    int ret = TRM_SetBeamInfo(beam2.userId, &beam2);
    TEST_ASSERT_EQ(ret, TRM_OK, "Update beam info should succeed");
    
    /* 验证更新后的信息 */
    TRM_BeamInfo beamOut;
    ret = TRM_GetBeamInfo(beam2.userId, &beamOut);
    TEST_ASSERT_EQ(ret, TRM_OK, "Get updated beam info should succeed");
    TEST_ASSERT_EQ(beamOut.freq, 2450000000, "Frequency should be updated");
    TEST_ASSERT_EQ(beamOut.pilotPower, 200, "Pilot power should be updated");
    TEST_ASSERT_EQ(beamOut.timestamp, 54321, "Timestamp should be updated");
    
    TRM_Deinit();
}

TEST_CASE(beam_null_parameters)
{
    SetupDefaultConfig();
    TRM_Init(&g_defaultConfig);
    
    /* 测试NULL参数 */
    int ret = TRM_SetBeamInfo(0x12345678, NULL);
    TEST_ASSERT_EQ(ret, TRM_ERR_PARAM, "SetBeamInfo with NULL should fail");
    
    ret = TRM_GetBeamInfo(0x12345678, NULL);
    TEST_ASSERT_EQ(ret, TRM_ERR_PARAM, "GetBeamInfo with NULL should fail");
    
    TRM_Deinit();
}

TEST_CASE(beam_max_users_limit)
{
    SetupDefaultConfig();
    g_defaultConfig.beamMaxUsers = 5;  /* 限制最大用户数为5 */
    TRM_Init(&g_defaultConfig);
    
    /* 添加5个用户应该成功 */
    int successCount = 0;
    for (int i = 0; i < 5; i++) {
        TRM_BeamInfo beam;
        memset(&beam, 0, sizeof(beam));
        beam.userId = 0x1000 + i;
        beam.freq = 2400000000 + i * 1000000;
        beam.pilotPower = 50 + i * 10;
        
        if (TRM_SetBeamInfo(beam.userId, &beam) == TRM_OK) {
            successCount++;
        }
    }
    TEST_ASSERT_EQ(successCount, 5, "Should add 5 beams successfully");
    
    /* 第6个用户应该失败 */
    TRM_BeamInfo beam;
    memset(&beam, 0, sizeof(beam));
    beam.userId = 0x2000;
    beam.freq = 2500000000;
    beam.pilotPower = 100;
    
    int ret = TRM_SetBeamInfo(beam.userId, &beam);
    TEST_ASSERT_EQ(ret, TRM_ERR_NO_MEM, "6th beam should fail with NO_MEM");
    
    TRM_Deinit();
}

TEST_CASE(beam_clear_single)
{
    SetupDefaultConfig();
    TRM_Init(&g_defaultConfig);
    
    /* 设置波束 */
    TRM_BeamInfo beam;
    memset(&beam, 0, sizeof(beam));
    beam.userId = 0x12345678;
    beam.freq = 2400000000;
    beam.pilotPower = 100;
    TRM_SetBeamInfo(beam.userId, &beam);
    
    /* 验证波束存在 */
    TRM_BeamInfo beamOut;
    int ret = TRM_GetBeamInfo(beam.userId, &beamOut);
    TEST_ASSERT_EQ(ret, TRM_OK, "Beam should exist before clear");
    
    /* 清除波束 */
    ret = TRM_ClearBeamInfo(beam.userId);
    TEST_ASSERT_EQ(ret, TRM_OK, "Clear beam should succeed");
    
    /* 验证波束已清除 */
    ret = TRM_GetBeamInfo(beam.userId, &beamOut);
    TEST_ASSERT_EQ(ret, TRM_ERR_NO_BEAM, "Beam should not exist after clear");
    
    TRM_Deinit();
}

TEST_CASE(beam_clear_all)
{
    SetupDefaultConfig();
    TRM_Init(&g_defaultConfig);
    
    /* 设置多个波束 */
    for (int i = 0; i < 10; i++) {
        TRM_BeamInfo beam;
        memset(&beam, 0, sizeof(beam));
        beam.userId = 0x1000 + i;
        beam.freq = 2400000000 + i * 1000000;
        beam.pilotPower = 50 + i * 5;
        TRM_SetBeamInfo(beam.userId, &beam);
    }
    
    /* 验证波束存在 */
    TRM_BeamInfo beamOut;
    int ret = TRM_GetBeamInfo(0x1005, &beamOut);
    TEST_ASSERT_EQ(ret, TRM_OK, "Beam should exist before clear all");
    
    /* 清除所有波束 */
    ret = TRM_ClearBeamInfo(0xFFFFFFFF);
    TEST_ASSERT_EQ(ret, TRM_OK, "Clear all beams should succeed");
    
    /* 验证所有波束已清除 */
    ret = TRM_GetBeamInfo(0x1005, &beamOut);
    TEST_ASSERT_EQ(ret, TRM_ERR_NO_BEAM, "Beam should not exist after clear all");
    ret = TRM_GetBeamInfo(0x1000, &beamOut);
    TEST_ASSERT_EQ(ret, TRM_ERR_NO_BEAM, "Another beam should not exist after clear all");
    
    TRM_Deinit();
}

TEST_CASE(beam_timeout_config)
{
    SetupDefaultConfig();
    TRM_Init(&g_defaultConfig);
    
    /* 设置波束超时时间 */
    int ret = TRM_SetBeamTimeout(10000);  /* 10秒 */
    TEST_ASSERT_EQ(ret, TRM_OK, "Set beam timeout should succeed");
    
    /* 设置波束 */
    TRM_BeamInfo beam;
    memset(&beam, 0, sizeof(beam));
    beam.userId = 0x12345678;
    beam.freq = 2400000000;
    beam.pilotPower = 100;
    beam.timestamp = 1000;  /* 较早的时间戳 */
    TRM_SetBeamInfo(beam.userId, &beam);
    
    /* 验证波束仍然存在（超时清理需要时间触发） */
    TRM_BeamInfo beamOut;
    ret = TRM_GetBeamInfo(beam.userId, &beamOut);
    TEST_ASSERT_EQ(ret, TRM_OK, "Beam should exist immediately after set");
    
    TRM_Deinit();
}

/*==============================================================================
 * 主函数
 *============================================================================*/

int main(void)
{
    printf("\n");
    printf("TK8710 HAL TRM Beam Tests\n");
    printf("==========================\n");
    
    /* 初始化日志 */
    TK8710LogSimpleInit(TK8710_LOG_ERROR, TK8710_LOG_MODULE_ALL);
    
    /* 基础波束管理测试 */
    TEST_SUITE_BEGIN("Basic Beam Management Tests");
    RUN_TEST(beam_set_get_basic);
    RUN_TEST(beam_update_existing);
    RUN_TEST(beam_null_parameters);
    TEST_SUITE_END();
    
    /* 波束限制测试 */
    TEST_SUITE_BEGIN("Beam Limit Tests");
    RUN_TEST(beam_max_users_limit);
    TEST_SUITE_END();
    
    /* 波束清理测试 */
    TEST_SUITE_BEGIN("Beam Clear Tests");
    RUN_TEST(beam_clear_single);
    RUN_TEST(beam_clear_all);
    TEST_SUITE_END();
    
    /* 波束配置测试 */
    TEST_SUITE_BEGIN("Beam Config Tests");
    RUN_TEST(beam_timeout_config);
    TEST_SUITE_END();
    
    printf("\n");
    return TEST_RESULT();
}
