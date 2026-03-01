/**
 * @file basic_example.c
 * @brief TK8710 HAL协同测试程序 - TRM和Driver的共同上层
 * @note 作为TRM和Driver的上层应用，负责数据生成、接收处理和协同控制
 * @version 3.0
 * @date 2026-02-14
 */

#include "trm/trm.h"
#include "driver/tk8710_api.h"
#include "driver/tk8710_log.h"
#include "driver/tk8710_types.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*==============================================================================
 * 应用层配置
 *============================================================================*/

#define MAX_TEST_USERS      8       /* 最大测试用户数 */
#define TEST_SEQ_COUNTER    1000    /* 测试序列计数器 */

/* 测试用户管理 */
typedef struct {
    uint32_t userId;
    uint8_t  active;
    uint32_t sendCount;
    uint32_t rxCount;
} TestUser;

static TestUser g_testUsers[MAX_TEST_USERS];
static uint32_t g_seqCounter = 0;

/*==============================================================================
 * 回调函数
 *============================================================================*/

static void OnRxData(const TRM_RxDataList* rxDataList)
{
    printf("=== 接收数据事件 ===\n");
    printf("时隙: %d, 用户数: %d, 帧号: %u\n", 
           rxDataList->slotIndex, rxDataList->userCount, rxDataList->frameNo);
    
    for (uint8_t i = 0; i < rxDataList->userCount; i++) {
        TRM_RxUserData* user = &rxDataList->users[i];
        
        /* 更新用户接收统计 */
        for (int j = 0; j < MAX_TEST_USERS; j++) {
            if (g_testUsers[j].userId == user->userId) {
                g_testUsers[j].rxCount++;
                break;
            }
        }
        
        printf("  用户[%d]: ID=0x%08X, 长度=%d, RSSI=%d\n", 
               i, user->userId, user->dataLen, user->rssi);
        
        /* 处理接收到的数据 */
        if (user->data != NULL && user->dataLen > 0) {
            printf("    数据: ");
            for (int k = 0; k < user->dataLen && k < 8; k++) {
                printf("%02X ", user->data[k]);
            }
            if (user->dataLen > 8) printf("...");
            printf("\n");
            
            /* 验证测试数据 */
            if (user->dataLen >= 4) {
                uint32_t pattern = *(uint32_t*)user->data;
                if (pattern == 0x12345678) {
                    printf("    ✓ 测试数据验证成功\n");
                }
            }
        }
    }
    printf("==================\n\n");
}

static void OnTxComplete(const TRM_TxCompleteResult* txResult)
{
    if (!txResult) return;
    
    const char* resultStr[] = {"OK", "NO_BEAM", "TIMEOUT", "ERROR"};
    printf("=== 发送完成事件 ===\n");
    printf("发送用户总数: %u, 剩余队列: %u\n", txResult->totalUsers, txResult->remainingQueue);
    
    /* 处理每个用户的发送结果 */
    for (uint32_t i = 0; i < txResult->userCount; i++) {
        const TRM_TxUserResult* userResult = &txResult->users[i];
        printf("  用户ID: 0x%08X, 结果: %s\n", userResult->userId, resultStr[userResult->result]);
        
        /* 更新用户发送统计 */
        for (int j = 0; j < MAX_TEST_USERS; j++) {
            if (g_testUsers[j].userId == userResult->userId) {
                if (userResult->result == TRM_TX_OK) {
                    g_testUsers[j].sendCount++;
                    printf("    ✓ 发送成功，累计发送: %u\n", g_testUsers[j].sendCount);
                } else {
                    printf("    ✗ 发送失败，错误: %s\n", resultStr[userResult->result]);
                }
                break;
            }
        }
    }
    printf("==================\n\n");
}


/*==============================================================================
 * 数据生成和用户管理
 *============================================================================*/

static void initTestUsers(void)
{
    for (int i = 0; i < MAX_TEST_USERS; i++) {
        g_testUsers[i].userId = 0x10000000 + i;  /* 用户ID: 0x10000000-0x10000007 */
        g_testUsers[i].active = 1;
        g_testUsers[i].sendCount = 0;
        g_testUsers[i].rxCount = 0;
        printf("初始化测试用户[%d]: ID=0x%08X\n", i, g_testUsers[i].userId);
    }
}

static void generateTestData(uint8_t* buffer, uint16_t len, uint32_t userId)
{
    /* 生成测试数据格式 */
    if (len >= 4) {
        *(uint32_t*)buffer = 0x12345678;  /* 测试数据模式标识 */
    }
    
    if (len >= 8) {
        *(uint32_t*)(buffer + 4) = userId;  /* 用户ID */
    }
    
    if (len >= 12) {
        *(uint32_t*)(buffer + 8) = g_seqCounter++;  /* 序列号 */
    }
    
    /* 填充剩余空间 */
    for (uint16_t i = 12; i < len; i++) {
        buffer[i] = (uint8_t)(g_seqCounter + i) & 0xFF;
    }
}

static void sendTestDataToAllUsers(void)
{
    uint8_t testData[32];
    uint16_t dataLen = sizeof(testData);
    uint32_t currentFrame = TRM_GetCurrentFrame();
    
    printf("=== 批量发送测试数据 ===\n");
    printf("当前帧号: %u\n", currentFrame);
    
    for (int i = 0; i < MAX_TEST_USERS; i++) {
        if (g_testUsers[i].active) {
            /* 生成测试数据 */
            generateTestData(testData, dataLen, g_testUsers[i].userId);
            
            /* 发送数据 */
            int ret = TRM_SendData(g_testUsers[i].userId, testData, dataLen, 20, currentFrame + 1, 0, TK8710_DATA_TYPE_DED);
            if (ret == TRM_OK) {
                printf("  发送到用户0x%08X: 成功\n", g_testUsers[i].userId);
            } else {
                printf("  发送到用户0x%08X: 失败 (错误=%d)\n", g_testUsers[i].userId, ret);
            }
        }
    }
    printf("==================\n\n");
}

static void printUserStats(void)
{
    printf("=== 用户统计报告 ===\n");
    for (int i = 0; i < MAX_TEST_USERS; i++) {
        if (g_testUsers[i].active) {
            printf("用户[%d]: ID=0x%08X, 发送=%u, 接收=%u\n", 
                   i, g_testUsers[i].userId, 
                   g_testUsers[i].sendCount, g_testUsers[i].rxCount);
        }
    }
    
    /* TRM统计 */
    TRM_Stats trmStats;
    if (TRM_GetStats(&trmStats) == TRM_OK) {
        printf("TRM统计: TX=%u, RX=%u, 波束=%u\n", 
               trmStats.txCount, trmStats.rxCount, trmStats.beamCount);
    }
    printf("==================\n\n");
}

/*==============================================================================
 * 主函数 *============================================================================*/

int main(void)
{
    printf("TK8710 HAL 协同测试程序\n");
    printf("========================\n");
    printf("作为TRM和Driver的共同上层应用\n\n");
    
    int ret;
    
    /* 初始化日志系统 */
    TK8710LogConfig_t logConfig = {
        .level = TK8710_LOG_INFO,
        .module_mask = TK8710_LOG_MODULE_ALL,
        .callback = NULL,
        .enable_timestamp = 1,
        .enable_module_name = 1
    };
    TK8710LogInit(&logConfig);
    
    /* 1. 独立初始化TRM */
    printf("初始化TRM...\n");
    TRM_InitConfig config;
    memset(&config, 0, sizeof(config));
    
    /* TRM配置 */
    config.beamMode = TRM_BEAM_MODE_FULL_STORE;
    config.beamMaxUsers = 3000;
    config.beamTimeoutMs = 3000;
    
    /* 回调函数 */
    config.callbacks.onRxData = OnRxData;
    config.callbacks.onTxComplete = OnTxComplete;
    
    ret = TRM_Init(&config);
    if (ret != TRM_OK) {
        printf("TRM_Init failed: %d\n", ret);
        return -1;
    }
    printf("TRM initialized successfully\n");
    
    /* 注册TRM到Driver的回调函数 */
    ret = TRM_RegisterDriverCallbacks();
    if (ret != TRM_OK) {
        printf("TRM_RegisterDriverCallbacks failed: %d\n", ret);
        TRM_Deinit();
        return -1;
    }
    printf("TRM Driver callbacks registered successfully\n");
    
    /* 2. 独立配置和初始化Driver */
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
        TK8710Reset(TK8710_RST_ALL);
        TRM_Deinit();
        return -1;
    }
    
    /* 初始化射频系统 */
    ret = TK8710RfConfig(&rfConfig);
    if (ret != TK8710_OK) {
        printf("RF init failed: %d\n", ret);
        TK8710Reset(TK8710_RST_ALL);
        TRM_Deinit();
        return -1;
    }
    
    /* 4. 平行启动系统 */
    printf("Starting system...\n");
    
    ret = TK8710Start(1, 0);
    if (ret != TK8710_OK) {
        printf("TK8710Start failed: %d\n", ret);
        TK8710Reset(TK8710_RST_ALL);
        TRM_Deinit();
        return -1;
    }
    
    printf("Driver started successfully\n");
    
    /* 检查状态 */
    TRM_Stats stats;
    TRM_GetStats(&stats);
    printf("TRM initialized: %s\n", stats.state == TRM_STATE_INIT ? "Yes" : "No");
    printf("Driver running: %s\n", "Yes");  /* Driver没有状态查询API */
    
    /* 获取统计信息 */
    TRM_GetStats(&stats);
    printf("Stats: tx=%u, txSuccess=%u, rx=%u, beams=%u\n",
           stats.txCount, stats.txSuccessCount, stats.rxCount, stats.beamCount);
    
    /* 5. 初始化测试用户 */
    printf("\n=== 初始化协同测试 ===\n");
    initTestUsers();
    
    /* 6. 协同测试主循环 */
    printf("\n=== 开始协同测试 ===\n");
    printf("测试流程:\n");
    printf("1. 发送测试数据到所有用户\n");
    printf("2. 等待接收数据处理\n");
    printf("3. 统计测试结果\n\n");
    
    /* 发送测试数据 */
    sendTestDataToAllUsers();
    
    /* 等待一段时间让系统处理 */
    printf("等待系统处理数据...\n");
    #ifdef _WIN32
    Sleep(2000);  /* Windows */
    #else
    usleep(2000000);  /* Linux */
    #endif
    
    /* 再次发送数据测试 */
    printf("\n第二次发送测试...\n");
    sendTestDataToAllUsers();
    
    /* 再次等待 */
    #ifdef _WIN32
    Sleep(2000);
    #else
    usleep(2000000);
    #endif
    
    /* 7. 打印最终统计 */
    printf("\n=== 最终测试报告 ===\n");
    printUserStats();
    
    /* 8. 测试波束管理 */
    printf("\n=== 测试波束管理 ===\n");
    TRM_BeamInfo beam;
    memset(&beam, 0, sizeof(beam));
    beam.userId = 0x12345678;
    beam.freq = 2400000000;
    beam.valid = 1;
    beam.timestamp = 1000;
    ret = TRM_SetBeamInfo(0x12345678, &beam);
    printf("TRM_SetBeamInfo result: %d\n", ret);
    
    TRM_BeamInfo beamOut;
    ret = TRM_GetBeamInfo(0x12345678, &beamOut);
    printf("TRM_GetBeamInfo result: %d, userId=0x%08X\n", ret, beamOut.userId);
    
    /* 9. 平行停止系统 */
    printf("\nStopping system...\n");
    
    /* 停止Driver - 直接复位芯片 */
    TK8710Reset(TK8710_RST_ALL);
    printf("Driver stopped\n");
    
    /* 平行反初始化 */
    printf("Deinitializing system...\n");
    
    /* 反初始化TRM */
    TRM_Deinit();
    printf("TRM deinitialized\n");
    
    printf("\n协同测试完成！\n");
    printf("========================\n");
    
    return 0;
}
