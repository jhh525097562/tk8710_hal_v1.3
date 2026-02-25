/**
 * @file advanced_example.c
 * @brief TK8710 HAL高级使用示例
 * @version 1.0
 * @date 2026-02-09
 * 
 * 本示例展示HAL的完整功能：
 * - 日志系统配置和使? * - 调试功能（中断时间统计）
 * - 波束管理（添加、查询、超时）
 * - 数据收发
 * - 统计信息
 */

#include "trm/trm.h"
#include "driver/tk8710_debug.h"
#include "common/tk8710_log.h"
#include <stdio.h>
#include <string.h>

/*==============================================================================
 * 全局变量
 *============================================================================*/

static uint32_t g_rxCount = 0;
static uint32_t g_txCount = 0;

/*==============================================================================
 * 回调函数
 *============================================================================*/

static void OnRxData(const TRM_RxDataList* rxDataList)
{
    g_rxCount++;
    LOG_TRM_INFO("RX: slot=%d, users=%d, frame=%u", 
                 rxDataList->slotIndex, rxDataList->userCount, rxDataList->frameNo);
    
    for (uint8_t i = 0; i < rxDataList->userCount; i++) {
        const TRM_RxUserData* user = &rxDataList->users[i];
        LOG_TRM_DEBUG("  User[%d]: id=0x%08X, len=%d, rssi=%d", 
                      i, user->userId, user->dataLen, user->rssi);
    }
}

static void OnRxBroadcast(const TRM_RxBrdData* brdData)
{
    LOG_TRM_INFO("RX Broadcast: index=%d, len=%d", brdData->brdIndex, brdData->dataLen);
}

static void OnTxComplete(uint32_t userId, TRM_TxResult result)
{
    g_txCount++;
    const char* resultStr[] = {"OK", "NO_BEAM", "TIMEOUT", "ERROR"};
    LOG_TRM_INFO("TX Complete: userId=0x%08X, result=%s", userId, resultStr[result]);
}

static void OnError(int errorCode, const char* message)
{
    LOG_TRM_ERROR("Error: code=%d, msg=%s", errorCode, message);
}

/*==============================================================================
 * 演示函数
 *============================================================================*/

static void DemoLogSystem(void)
{
    printf("\n");
    printf("========================================\n");
    printf("1. Log System Demo\n");
    printf("========================================\n");
    
    /* 设置日志级别为DEBUG，显示所有日?*/
    TK8710_LogSetLevel(TK8710_LOG_DEBUG);
    TK8710_LogSetModuleMask(TK8710_LOG_MOD_ALL);
    
    printf("Log level: DEBUG, Module mask: ALL\n");
    LOG_CORE_ERROR("This is an ERROR log");
    LOG_CORE_WARN("This is a WARN log");
    LOG_CORE_INFO("This is an INFO log");
    LOG_CORE_DEBUG("This is a DEBUG log");
    
    /* 只显示ERROR和WARN */
    printf("\nLog level: WARN\n");
    TK8710_LogSetLevel(TK8710_LOG_WARN);
    LOG_CORE_ERROR("ERROR is visible");
    LOG_CORE_WARN("WARN is visible");
    LOG_CORE_INFO("INFO is NOT visible");
    LOG_CORE_DEBUG("DEBUG is NOT visible");
    
    /* 只显示特定模?*/
    printf("\nModule mask: CORE only\n");
    TK8710_LogSetLevel(TK8710_LOG_INFO);
    TK8710_LogSetModuleMask(TK8710_LOG_MOD_CORE);
    LOG_CORE_INFO("CORE module log - visible");
    LOG_TRM_INFO("TRM module log - NOT visible");
    LOG_IRQ_INFO("IRQ module log - NOT visible");
    
    /* 恢复默认 */
    TK8710_LogSetLevel(TK8710_LOG_INFO);
    TK8710_LogSetModuleMask(TK8710_LOG_MOD_ALL);
}

static void DemoDebugFeatures(void)
{
    printf("\n");
    printf("========================================\n");
    printf("2. Debug Features Demo\n");
    printf("========================================\n");
    
    /* 重置中断计数器*/
    TK8710_ResetIrqCounters();
    printf("IRQ counters reset\n");
    
    /* 获取中断计数器*/
    uint32_t counters[10];
    TK8710_GetAllIrqCounters(counters);
    printf("IRQ counters: ");
    for (int i = 0; i < 10; i++) {
        printf("%u ", counters[i]);
    }
    printf("\n");
    
    /* 获取中断时间统计 */
    TK8710_IrqTimeStats stats;
    TK8710_GetIrqTimeStats(4, &stats);  /* MD_DATA */
    printf("MD_DATA IRQ stats: count=%u, avg=%u us, max=%u us\n",
           stats.count, stats.avgTime, stats.maxTime);
    
    /* 测试控制 */
    printf("\nTest control:\n");
    TK8710_SetForceProcessAllUsers(1);
    printf("  ForceProcessAllUsers: %d\n", TK8710_GetForceProcessAllUsers());
    TK8710_SetForceProcessAllUsers(0);
    
    TK8710_SetForceMaxUsersTx(1);
    printf("  ForceMaxUsersTx: %d\n", TK8710_GetForceMaxUsersTx());
    TK8710_SetForceMaxUsersTx(0);
}

static void DemoBeamManagement(void)
{
    printf("\n");
    printf("========================================\n");
    printf("3. Beam Management Demo\n");
    printf("========================================\n");
    
    /* 添加多个用户波束 */
    printf("Adding beams for 5 users...\n");
    for (int i = 0; i < 5; i++) {
        TRM_BeamInfo beam;
        memset(&beam, 0, sizeof(beam));
        beam.userId = 0x10000000 + i;
        beam.freq = 2400000000 + i * 1000;
        beam.pilotPower = 100 + i * 10;
        
        int ret = TRM_SetBeamInfo(beam.userId, &beam);
        printf("  User 0x%08X: %s\n", beam.userId, ret == TRM_OK ? "OK" : "FAIL");
    }
    
    /* 查询波束 */
    printf("\nQuerying beams...\n");
    for (int i = 0; i < 5; i++) {
        uint32_t userId = 0x10000000 + i;
        TRM_BeamInfo beam;
        int ret = TRM_GetBeamInfo(userId, &beam);
        if (ret == TRM_OK) {
            printf("  User 0x%08X: freq=%u, power=%llu\n", 
                   userId, beam.freq, (unsigned long long)beam.pilotPower);
        } else {
            printf("  User 0x%08X: NOT FOUND\n", userId);
        }
    }
    
    /* 查询不存在的用户 */
    printf("\nQuerying non-existent user...\n");
    TRM_BeamInfo beam;
    int ret = TRM_GetBeamInfo(0xDEADBEEF, &beam);
    printf("  User 0xDEADBEEF: %s\n", ret == TRM_OK ? "FOUND" : "NOT FOUND");
    
    /* 清除单个用户 */
    printf("\nClearing user 0x10000002...\n");
    TRM_ClearBeamInfo(0x10000002);
    ret = TRM_GetBeamInfo(0x10000002, &beam);
    printf("  User 0x10000002: %s\n", ret == TRM_OK ? "FOUND" : "CLEARED");
    
    /* 清除所有*/
    printf("\nClearing all beams...\n");
    TRM_ClearBeamInfo(0xFFFFFFFF);
}

static void DemoDataTransfer(void)
{
    printf("\n");
    printf("========================================\n");
    printf("4. Data Transfer Demo\n");
    printf("========================================\n");
    
    /* 先设置波?*/
    TRM_BeamInfo beam;
    memset(&beam, 0, sizeof(beam));
    beam.userId = 0xAABBCCDD;
    beam.freq = 2450000000;
    TRM_SetBeamInfo(0xAABBCCDD, &beam);
    printf("Beam set for user 0xAABBCCDD\n");
    
    /* 发送数?*/
    printf("\nSending data to user 0xAABBCCDD...\n");
    uint8_t txData[32];
    for (int i = 0; i < 32; i++) {
        txData[i] = i;
    }
    
    int ret = TRM_SendData(0xAABBCCDD, txData, sizeof(txData), 25);
    printf("  SendData result: %d\n", ret);
    
    /* 发送多个数据包 */
    printf("\nSending 10 packets...\n");
    for (int i = 0; i < 10; i++) {
        txData[0] = i;
        ret = TRM_SendData(0xAABBCCDD, txData, 16, 20);
        printf("  Packet %d: %s\n", i, ret == TRM_OK ? "queued" : "failed");
    }
    
    /* 清除发送队列*/
    printf("\nClearing TX queue...\n");
    TRM_ClearTxData(0xFFFFFFFF);
}

static void DemoStatistics(void)
{
    printf("\n");
    printf("========================================\n");
    printf("5. Statistics Demo\n");
    printf("========================================\n");
    
    TRM_Stats stats;
    TRM_GetStats(&stats);
    
    printf("TRM Statistics:\n");
    printf("  TX Count:         %u\n", stats.txCount);
    printf("  TX Success Count: %u\n", stats.txSuccessCount);
    printf("  RX Count:         %u\n", stats.rxCount);
    printf("  Beam Count:       %u\n", stats.beamCount);
    printf("  Mem Alloc Count:  %u\n", stats.memAllocCount);
    printf("  Mem Free Count:   %u\n", stats.memFreeCount);
    
    printf("\nCallback counters:\n");
    printf("  RX callbacks:     %u\n", g_rxCount);
    printf("  TX callbacks:     %u\n", g_txCount);
}

/*==============================================================================
 * 主函数 *============================================================================*/

int main(void)
{
    printf("\n");
    printf("########################################\n");
    printf("#  TK8710 HAL Advanced Example         #\n");
    printf("########################################\n");
    
    /* 初始化日志系?*/
    TK8710_LogSimpleInit(TK8710_LOG_INFO, TK8710_LOG_MOD_ALL);
    
    /* 配置TRM */
    TRM_InitConfig config;
    memset(&config, 0, sizeof(config));
    
    config.beamMode = TRM_BEAM_MODE_FULL_STORE;
    config.beamMaxUsers = 1000;
    config.beamTimeoutMs = 5000;
    config.rateMode = TRM_RATE_4M;
    config.antennaCount = 8;
    config.slotConfig.bcnSlotCount = 1;
    config.slotConfig.brdSlotCount = 1;
    config.slotConfig.ulSlotCount = 4;
    config.slotConfig.dlSlotCount = 4;
    config.rfFreq = 2400000000;
    config.txPower = 20;
    
    config.callbacks.onRxData = OnRxData;
    config.callbacks.onRxBroadcast = OnRxBroadcast;
    config.callbacks.onTxComplete = OnTxComplete;
    config.callbacks.onError = OnError;
    
    /* 初始化TRM */
    printf("\nInitializing TRM...\n");
    int ret = TRM_Init(&config);
    if (ret != TRM_OK) {
        printf("TRM_Init failed: %d\n", ret);
        return -1;
    }
    printf("TRM initialized successfully\n");
    
    /* 启动TRM */
    ret = TRM_Start();
    if (ret != TRM_OK) {
        printf("TRM_Start failed: %d\n", ret);
        TRM_Deinit();
        return -1;
    }
    printf("TRM started successfully\n");
    
    /* 运行各个演示 */
    DemoLogSystem();
    DemoDebugFeatures();
    DemoBeamManagement();
    DemoDataTransfer();
    DemoStatistics();
    
    /* 打印中断时间统计报告 */
    printf("\n");
    printf("========================================\n");
    printf("6. IRQ Time Statistics Report\n");
    printf("========================================\n");
    TK8710_PrintIrqTimeStats();
    
    /* 停止TRM */
    printf("\n");
    printf("========================================\n");
    printf("Cleanup\n");
    printf("========================================\n");
    TRM_Stop();
    printf("TRM stopped\n");
    
    TRM_Deinit();
    printf("TRM deinitialized\n");
    
    printf("\n");
    printf("########################################\n");
    printf("#  Example completed successfully!     #\n");
    printf("########################################\n\n");
    
    return 0;
}
