/**
 * @file test_trm.c
 * @brief TRM模块集成测试
 * @version 1.0
 * @date 2026-02-09
 */

#include "test_framework.h"
#include "trm/trm.h"
#include "common/tk8710_log.h"
#include <string.h>

/*==============================================================================
 * 测试回调函数
 *============================================================================*/

static int g_rxDataCount = 0;
static int g_rxBrdCount = 0;
static int g_txCompleteCount = 0;
static int g_errorCount = 0;
static TRM_TxResult g_lastTxResult = TRM_TX_OK;

static void TestOnRxData(const TRM_RxDataList* rxDataList)
{
    if (rxDataList != NULL) {
        g_rxDataCount++;
    }
}

static void TestOnRxBroadcast(const TRM_RxBrdData* brdData)
{
    if (brdData != NULL) {
        g_rxBrdCount++;
    }
}

static void TestOnTxComplete(uint32_t userId, TRM_TxResult result)
{
    (void)userId;
    g_lastTxResult = result;
    g_txCompleteCount++;
}

static void TestOnError(int errorCode, const char* message)
{
    (void)errorCode;
    (void)message;
    g_errorCount++;
}

static void ResetTestCounters(void)
{
    g_rxDataCount = 0;
    g_rxBrdCount = 0;
    g_txCompleteCount = 0;
    g_errorCount = 0;
    g_lastTxResult = TRM_TX_OK;
}

/*==============================================================================
 * TRM初始化测? *============================================================================*/

TEST_CASE(trm_init_null_config)
{
    int ret = TRM_Init(NULL);
    TEST_ASSERT_EQ(ret, TRM_ERR_PARAM, "Init with NULL config should fail");
}

TEST_CASE(trm_init_valid_config)
{
    TRM_InitConfig config;
    memset(&config, 0, sizeof(config));
    
    config.beamMode = TRM_BEAM_MODE_FULL_STORE;
    config.beamMaxUsers = 1000;
    config.beamTimeoutMs = 3000;
    
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
    
    config.chipConfig = &defaultChipConfig;
    config.slotConfig = &defaultSlotConfig;
    config.rfConfig = &defaultRfConfig;
    
    config.callbacks.onRxData = TestOnRxData;
    config.callbacks.onRxBroadcast = TestOnRxBroadcast;
    config.callbacks.onTxComplete = TestOnTxComplete;
    config.callbacks.onError = TestOnError;
    
    int ret = TRM_Init(&config);
    TEST_ASSERT_EQ(ret, TRM_OK, "Init with valid config should succeed");
    
    TRM_Deinit();
}

TEST_CASE(trm_double_init)
{
    TRM_InitConfig config;
    memset(&config, 0, sizeof(config));
    
    config.beamMode = TRM_BEAM_MODE_FULL_STORE;
    config.beamMaxUsers = 100;
    config.beamTimeoutMs = 3000;
    
    /* 使用简单的默认配置 */
    static ChipConfig simpleChipConfig = {
        .bcn_agc = 32,
        .interval = 32,
        .tx_dly = 1,
        .conti_mode = 1,
        .ant_en = 0xFF,
        .tx_bcn_en = 1
    };
    
    static slotCfg_t simpleSlotConfig = {
        .msMode = TK8710_MODE_MASTER,
        .plCrcEn = 1,
        .rateCount = 1,
        .rateModes = {TK8710_RATE_MODE_8}
    };
    
    static ChiprfConfig simpleRfConfig = {
        .rftype = TK8710_RF_TYPE_1255_1M,
        .Freq = 2400000000
    };
    
    config.chipConfig = &simpleChipConfig;
    config.slotConfig = &simpleSlotConfig;
    config.rfConfig = &simpleRfConfig;
    
    int ret1 = TRM_Init(&config);
    TEST_ASSERT_EQ(ret1, TRM_OK, "First init should succeed");
    
    int ret2 = TRM_Init(&config);
    TEST_ASSERT_EQ(ret2, TRM_ERR_STATE, "Double init should fail");
    
    TRM_Deinit();
}

/*==============================================================================
 * TRM启动/停止测试
 *============================================================================*/

TEST_CASE(trm_start_stop)
{
    TRM_InitConfig config;
    memset(&config, 0, sizeof(config));
    
    config.beamMode = TRM_BEAM_MODE_FULL_STORE;
    config.beamMaxUsers = 100;
    config.beamTimeoutMs = 3000;
    
    /* 使用简单的默认配置 */
    static ChipConfig simpleChipConfig = {
        .bcn_agc = 32,
        .interval = 32,
        .tx_dly = 1,
        .conti_mode = 1,
        .ant_en = 0xFF,
        .tx_bcn_en = 1
    };
    
    static slotCfg_t simpleSlotConfig = {
        .msMode = TK8710_MODE_MASTER,
        .plCrcEn = 1,
        .rateCount = 1,
        .rateModes = {TK8710_RATE_MODE_8}
    };
    
    static ChiprfConfig simpleRfConfig = {
        .rftype = TK8710_RF_TYPE_1255_1M,
        .Freq = 2400000000
    };
    
    config.chipConfig = &simpleChipConfig;
    config.slotConfig = &simpleSlotConfig;
    config.rfConfig = &simpleRfConfig;
    
    TRM_Init(&config);
    
    int ret = TRM_Start();
    TEST_ASSERT_EQ(ret, TRM_OK, "Start should succeed");
    TEST_ASSERT_EQ(TRM_IsRunning(), 1, "Should be running after start");
    
    ret = TRM_Stop();
    TEST_ASSERT_EQ(ret, TRM_OK, "Stop should succeed");
    TEST_ASSERT_EQ(TRM_IsRunning(), 0, "Should not be running after stop");
    
    TRM_Deinit();
}

/*==============================================================================
 * 波束管理测试
 *============================================================================*/

TEST_CASE(beam_set_get)
{
    TRM_InitConfig config;
    memset(&config, 0, sizeof(config));
    config.beamMaxUsers = 100;
    config.beamTimeoutMs = 5000;
    
    TRM_Init(&config);
    
    /* 设置波束 */
    TRM_BeamInfo beamIn;
    memset(&beamIn, 0, sizeof(beamIn));
    beamIn.userId = 0x12345678;
    beamIn.freq = 2400000000;
    beamIn.pilotPower = 100;
    
    int ret = TRM_SetBeamInfo(0x12345678, &beamIn);
    TEST_ASSERT_EQ(ret, TRM_OK, "SetBeamInfo should succeed");
    
    /* 获取波束 */
    TRM_BeamInfo beamOut;
    ret = TRM_GetBeamInfo(0x12345678, &beamOut);
    TEST_ASSERT_EQ(ret, TRM_OK, "GetBeamInfo should succeed");
    TEST_ASSERT_EQ(beamOut.userId, 0x12345678, "UserId should match");
    TEST_ASSERT_EQ(beamOut.freq, 2400000000, "Freq should match");
    
    TRM_Deinit();
}

TEST_CASE(beam_get_nonexistent)
{
    TRM_InitConfig config;
    memset(&config, 0, sizeof(config));
    
    TRM_Init(&config);
    
    TRM_BeamInfo beam;
    int ret = TRM_GetBeamInfo(0xDEADBEEF, &beam);
    TEST_ASSERT_EQ(ret, TRM_ERR_NO_BEAM, "GetBeamInfo for nonexistent user should fail");
    
    TRM_Deinit();
}

TEST_CASE(beam_clear)
{
    TRM_InitConfig config;
    memset(&config, 0, sizeof(config));
    
    TRM_Init(&config);
    
    /* 设置波束 */
    TRM_BeamInfo beam;
    memset(&beam, 0, sizeof(beam));
    beam.userId = 0x11111111;
    TRM_SetBeamInfo(0x11111111, &beam);
    
    /* 清除波束 */
    int ret = TRM_ClearBeamInfo(0x11111111);
    TEST_ASSERT_EQ(ret, TRM_OK, "ClearBeamInfo should succeed");
    
    /* 验证已清?*/
    ret = TRM_GetBeamInfo(0x11111111, &beam);
    TEST_ASSERT_EQ(ret, TRM_ERR_NO_BEAM, "Beam should be cleared");
    
    TRM_Deinit();
}

/*==============================================================================
 * 数据发送测? *============================================================================*/

TEST_CASE(send_data_basic)
{
    TRM_InitConfig config;
    memset(&config, 0, sizeof(config));
    
    TRM_Init(&config);
    TRM_Start();
    
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    int ret = TRM_SendData(0x12345678, data, sizeof(data), 20, 0);
    TEST_ASSERT_EQ(ret, TRM_OK, "SendData should succeed");
    
    TRM_Stop();
    TRM_Deinit();
}

TEST_CASE(send_data_null)
{
    TRM_InitConfig config;
    memset(&config, 0, sizeof(config));
    
    TRM_Init(&config);
    TRM_Start();
    
    int ret = TRM_SendData(0x12345678, NULL, 10, 20, 0);
    TEST_ASSERT_EQ(ret, TRM_ERR_PARAM, "SendData with NULL data should fail");
    
    TRM_Stop();
    TRM_Deinit();
}

TEST_CASE(send_data_zero_len)
{
    TRM_InitConfig config;
    memset(&config, 0, sizeof(config));
    
    TRM_Init(&config);
    TRM_Start();
    
    uint8_t data[] = {0x01};
    int ret = TRM_SendData(0x12345678, data, 0, 20, 0);
    TEST_ASSERT_EQ(ret, TRM_ERR_PARAM, "SendData with zero length should fail");
    
    TRM_Stop();
    TRM_Deinit();
}

/*==============================================================================
 * 统计信息测试
 *============================================================================*/

TEST_CASE(stats_basic)
{
    TRM_InitConfig config;
    memset(&config, 0, sizeof(config));
    
    TRM_Init(&config);
    
    TRM_Stats stats;
    int ret = TRM_GetStats(&stats);
    TEST_ASSERT_EQ(ret, TRM_OK, "GetStats should succeed");
    TEST_ASSERT_EQ(stats.txCount, 0, "Initial txCount should be 0");
    TEST_ASSERT_EQ(stats.rxCount, 0, "Initial rxCount should be 0");
    
    TRM_Deinit();
}

TEST_CASE(stats_null)
{
    TRM_InitConfig config;
    memset(&config, 0, sizeof(config));
    
    TRM_Init(&config);
    
    int ret = TRM_GetStats(NULL);
    TEST_ASSERT_EQ(ret, TRM_ERR_PARAM, "GetStats with NULL should fail");
    
    TRM_Deinit();
}

/*==============================================================================
 * 主函数 *============================================================================*/

int main(void)
{
    printf("\n");
    printf("TK8710 HAL TRM Integration Tests\n");
    printf("=================================\n");
    
    /* 初始化日?*/
    TK8710LogSimpleInit(TK8710_LOG_WARN, TK8710_LOG_MODULE_ALL);
    
    /* TRM初始化测?*/
    TEST_SUITE_BEGIN("TRM Init Tests");
    RUN_TEST(trm_init_null_config);
    RUN_TEST(trm_init_valid_config);
    RUN_TEST(trm_double_init);
    TEST_SUITE_END();
    
    /* TRM启动/停止测试 */
    TEST_SUITE_BEGIN("TRM Start/Stop Tests");
    RUN_TEST(trm_start_stop);
    TEST_SUITE_END();
    
    /* 波束管理测试 */
    TEST_SUITE_BEGIN("Beam Management Tests");
    RUN_TEST(beam_set_get);
    RUN_TEST(beam_get_nonexistent);
    RUN_TEST(beam_clear);
    TEST_SUITE_END();
    
    /* 数据发送测?*/
    TEST_SUITE_BEGIN("Data Send Tests");
    RUN_TEST(send_data_basic);
    RUN_TEST(send_data_null);
    RUN_TEST(send_data_zero_len);
    TEST_SUITE_END();
    
    /* 统计信息测试 */
    TEST_SUITE_BEGIN("Stats Tests");
    RUN_TEST(stats_basic);
    RUN_TEST(stats_null);
    TEST_SUITE_END();
    
    printf("\n");
    return TEST_RESULT();
}
