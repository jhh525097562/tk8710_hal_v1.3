/**
 * @file parallel_arch_example.c
 * @brief TK8710 HAL平行架构使用示例
 * @note 展示TRM和Driver平行初始化和配置的新架构
 * @version 1.0
 * @date 2026-02-14
 */

#include "trm/trm.h"
#include "driver/tk8710_api.h"
#include "driver/tk8710_log.h"
#include "driver/tk8710_types.h"
#include <stdio.h>
#include <string.h>

/*==============================================================================
 * 回调函数
 *============================================================================*/

static void OnRxData(const TRM_RxDataList* rxDataList)
{
    printf("TRM Received data: slot=%d, userCount=%d, frameNo=%u\n",
           rxDataList->slotIndex, rxDataList->userCount, rxDataList->frameNo);
    
    for (uint8_t i = 0; i < rxDataList->userCount; i++) {
        printf("  User[%d]: userId=0x%08X, len=%d, rssi=%d\n",
               i, rxDataList->users[i].userId,
               rxDataList->users[i].dataLen,
               rxDataList->users[i].rssi);
    }
}

static void OnTxComplete(uint32_t userId, TRM_TxResult result)
{
    const char* resultStr[] = {"OK", "NO_BEAM", "TIMEOUT", "ERROR"};
    printf("TRM Tx complete: userId=0x%08X, result=%s\n",
           userId, resultStr[result]);
}

/*==============================================================================
 * 主函数
 *============================================================================*/

int main(void)
{
    printf("TK8710 HAL Parallel Architecture Example\n");
    printf("=======================================\n\n");
    
    int ret;
    
    /* 1. 初始化日志系统 */
    TK8710LogConfig logConfig = {
        .level = TK8710_LOG_DEBUG,
        .module_mask = TK8710_LOG_MODULE_ALL,
        .callback = NULL,
        .enable_timestamp = 1,
        .enable_module_name = 1
    };
    TK8710LogInit(&logConfig);
    TK8710_LOG_CORE_INFO("Log system initialized");
    
    /* 2. 独立配置和初始化Driver (使用现有API) */
    printf("Initializing Driver...\n");
    
    /* 创建实际的配置结?*/
    static ChipConfig chipConfig = {
        .bcn_agc     = 32,
        .interval    = 32,
        .tx_dly      = 0,
        .tx_fix_info = 0,
        .offset_adj  = 0,
        .tx_pre      = 0,
        .conti_mode  = 1,
        .bcn_scan    = 0,
        .ant_en      = 0xFF,
        .rf_sel      = 0xFF,
        .tx_bcn_en   = 1,
        .ts_sync     = 0,
        .rf_model    = 1,
        .bcnbits     = 0,
        .anoiseThe1  = 0,
        .power2rssi  = 0,
        .irq_ctrl0   = 0x7FF,
        .irq_ctrl1   = 0,
    };
    
    static slotCfg_t slotConfig;
    memset(&slotConfig, 0, sizeof(slotCfg_t));
    slotConfig.msMode = TK8710_MODE_MASTER;
    slotConfig.plCrcEn = 0;
    slotConfig.rateCount = 1;
    slotConfig.rateModes[0] = TK8710_RATE_MODE_8;
    slotConfig.brdUserNum = 1;
    slotConfig.antEn = 0xFF;
    slotConfig.rfSel = 0xFF;
    slotConfig.txAutoMode = 1;
    slotConfig.txBcnEn = 0x7f;
    slotConfig.brdFreq[0] = 20000.0;
    slotConfig.rx_delay = 0;
    slotConfig.md_agc = 1024;
    slotConfig.s1Cfg[0].da_m = 5600;
    slotConfig.s2Cfg[0].da_m = 5600;
    slotConfig.s3Cfg[0].da_m = 5600;
    slotConfig.s1Cfg[0].byteLen = 26;
    slotConfig.s1Cfg[0].centerFreq = 503100000;
    slotConfig.s2Cfg[0].byteLen = 26;
    slotConfig.s2Cfg[0].centerFreq = 503100000;
    slotConfig.s3Cfg[0].byteLen = 26;
    slotConfig.s3Cfg[0].centerFreq = 503100000;
    
    static ChiprfConfig rfConfig;
    memset(&rfConfig, 0, sizeof(ChiprfConfig));
    rfConfig.rftype = TK8710_RF_TYPE_1255_32M;
    rfConfig.Freq = 503100000;
    rfConfig.rxgain = 0x7e;
    rfConfig.txgain = 0x2a;
    uint16_t txadc_i[8] = {0x0bc0,0x0a50,0x0750,0x0bc3,0x0e83,0xfbff,0x0880,0x02a0};
    uint16_t txadc_q[8] = {0x04a0,0x0780,0x0820,0x0940,0x05e0,0x0850,0x0500,0x06ff};
    for (int i = 0; i < 8; i++) {
        rfConfig.txadc[i].i = txadc_i[i];
        rfConfig.txadc[i].q = txadc_q[i];
    }
    
    /* 3. 建立平行连接 - 直接使用Driver API */
    /* 初始化Driver芯片 */
    ret = TK8710Init(&chipConfig);
    if (ret != TK8710_OK) {
        printf("TK8710Init failed: %d\n", ret);
        TRM_Deinit();
        return -1;
    }
    printf("Driver initialized successfully\n");
    
    /* 配置时隙参数 */
    ret = TK8710SetConfig(TK8710_CFG_TYPE_SLOT_CFG, &slotConfig);
    if (ret != TK8710_OK) {
        printf("Slot config failed: %d\n", ret);
        TK8710ResetChip(TK8710_RST_ALL);
        TRM_Deinit();
        return -1;
    }
    
    /* 初始化射频系统 */
    ret = TK8710RfInit(&rfConfig);
    if (ret != TK8710_OK) {
        printf("RF init failed: %d\n", ret);
        TK8710ResetChip(TK8710_RST_ALL);
        TRM_Deinit();
        return -1;
    }
    
    /* 4. 平行启动系统 */
    printf("Initializing TRM...\n");
    
    /* 配置TRM参数 */
    TRM_InitConfig trmConfig;
    memset(&trmConfig, 0, sizeof(trmConfig));
    
    /* TRM波束配置 */
    trmConfig.beamMode = TRM_BEAM_MODE_FULL_STORE;
    trmConfig.beamMaxUsers = 64;
    trmConfig.beamTimeoutMs = 5000;
    
    /* TRM回调配置 */
    trmConfig.callbacks.onRxData = OnRxData;
    trmConfig.callbacks.onTxComplete = OnTxComplete;
    
    /* 初始化TRM */
    ret = TRM_Init(&trmConfig);
    if (ret != TRM_OK) {
        printf("TRM_Init failed: %d\n", ret);
        TK8710ResetChip(TK8710_RST_ALL);
        return -1;
    }
    printf("TRM initialized successfully\n");
    
    /* 注册TRM到Driver的回调函数 */
    ret = TRM_RegisterDriverCallbacks();
    if (ret != TRM_OK) {
        printf("TRM_RegisterDriverCallbacks failed: %d\n", ret);
        TRM_Deinit();
        TK8710ResetChip(TK8710_RST_ALL);
        return -1;
    }
    printf("TRM Driver callbacks registered successfully\n");
    
    printf("Starting TRM...\n");
    ret = TRM_Start();
    if (ret != TRM_OK) {
        printf("TRM_Start failed: %d\n", ret);
        TK8710ResetChip(TK8710_RST_ALL);
        TRM_Deinit();
        return -1;
    }
    printf("TRM started successfully\n");
    
    printf("Starting Driver...\n");
    ret = TK8710Start(1, 0);  /* Master模式，连续工作 */
    if (ret != TK8710_OK) {
        printf("TK8710Start failed: %d\n", ret);
        TRM_Stop();
        TK8710ResetChip(TK8710_RST_ALL);
        TRM_Deinit();
        return -1;
    }
    printf("Driver started successfully\n");
    
    /* 5. 检查状态 */
    printf("\nSystem Status:\n");
    printf("TRM running: %s\n", TRM_IsRunning() ? "Yes" : "No");
    printf("Driver running: %s\n", "Yes");  /* Driver没有状态查询API，假设启动成功就是运行中 */
    
    /* 获取TRM统计信息 */
    TRM_Stats stats;
    TRM_GetStats(&stats);
    printf("TRM Stats: tx=%u, txSuccess=%u, rx=%u, beams=%u\n",
           stats.txCount, stats.txSuccessCount, stats.rxCount, stats.beamCount);
    
    /* 7. 测试TRM功能 */
    printf("\nTesting TRM functions...\n");
    
    /* 发送测试数据 */
    uint8_t testData[] = {0x01, 0x02, 0x03, 0x04};
    ret = TRM_SendData(0x12345678, testData, sizeof(testData), 20, 0, 0);
    printf("TRM_SendData result: %d\n", ret);
    
    /* 设置波束信息 */
    TRM_BeamInfo beam;
    memset(&beam, 0, sizeof(beam));
    beam.userId = 0x12345678;
    beam.freq = 503100000;
    beam.valid = 1;
    beam.timestamp = 1000;
    ret = TRM_SetBeamInfo(0x12345678, &beam);
    printf("TRM_SetBeamInfo result: %d\n", ret);
    
    /* 8. 运行一段时间 */
    printf("\nSystem is running... (Press Ctrl+C to exit)\n");
    
    /* 模拟运行 */
    for (int i = 0; i < 10; i++) {
        TK8710GetTickMs();  /* 模拟时间流逝 */
        printf("Running... %d/10\n", i + 1);
        
        /* 模拟中断处理 */
        /* 在实际系统中，这里会被Driver的中断回调触发 */
        
        /* 等待一段时间 */
        /* 注意：在实际应用中，这里应该是适当延时或事件等待 */
        #ifdef _WIN32
        Sleep(100);
        #else
        usleep(100000);
        #endif
    }
    
    /* 9. 清理资源 (平行清理) */
    printf("\nShutting down system...\n");
    
    /* 停止Driver - 直接复位芯片 */
    TK8710ResetChip(TK8710_RST_ALL);
    printf("Driver stopped\n");
    
    /* 停止TRM */
    TRM_Stop();
    printf("TRM stopped\n");
    
    /* 反初始化TRM */
    TRM_Deinit();
    printf("TRM deinitialized\n");
    
    printf("\nParallel architecture example completed successfully!\n");
    
    return 0;
}
