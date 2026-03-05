/**
 * @file validator_example.c
 * @brief TRM发送验证器使用示例
 * @note 展示如何集成和使用发送验证器
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

/* 模拟TRM相关头文�?- 在实际使用中包含真实的头文件 */
#include "trm_tx_validator.h"

/* ============================================================================
 * 示例1: 基本使用
 * ============================================================================
 */

void example_basic_usage(void)
{
    printf("=== 示例1: 基本使用 ===\n");
    
    /* 1. 初始化验证器 */
    TRM_TxValidatorConfig config = {
        .txPower = 20,
        .frameOffset = 3,
        .userIdBase = 0x30000000,
        .enableAutoResponse = true,
        .enablePeriodicTest = false,
        .periodicIntervalFrames = 0
    };
    
    int ret = TRM_TxValidatorInit(&config);
    if (ret != TRM_OK) {
        printf("验证器初始化失败: %d\n", ret);
        return;
    }
    
    /* 2. 模拟接收数据回调 */
    TRM_RxDataList rxDataList = {
        ,
        .userCount = 2,
        .frameNo = 100,
        .users = {
            {
                .userId = 0x12345678,
                .data = (uint8_t[]){0x01, 0x02, 0x03, 0x04},
                .dataLen = 4,
                .rssi = -50,
                .snr = 10,
                .freq = 2400000000,
                .beam = {.userId = 0x12345678, .timestamp = 12345}
            },
            {
                .userId = 0x87654321,
                .data = (uint8_t[]){0x05, 0x06, 0x07, 0x08, 0x09},
                .dataLen = 5,
                .rssi = -45,
                .snr = 12,
                .freq = 2401000000,
                .beam = {.userId = 0x87654321, .timestamp = 12346}
            }
        }
    };
    
    /* 3. 处理接收数据 */
    ret = TRM_TxValidatorOnRxData(&rxDataList);
    printf("接收数据处理结果: %d\n", ret);
    
    /* 4. 显示统计信息 */
    TRM_TxValidatorStats stats;
    if (TRM_TxValidatorGetStats(&stats) == TRM_OK) {
        printf("发送统�? 总触�?%u, 成功=%u, 失败=%u\n", 
               stats.totalTriggerCount, stats.successSendCount, stats.failedSendCount);
    }
    
    /* 5. 清理 */
    TRM_TxValidatorDeinit();
    printf("\n");
}

/* ============================================================================
 * 示例2: 周期测试
 * ============================================================================
 */

void example_periodic_test(void)
{
    printf("=== 示例2: 周期测试 ===\n");
    
    /* 配置周期测试 */
    TRM_TxValidatorConfig config = {
        .txPower = 25,
        .frameOffset = 5,
        .userIdBase = 0x40000000,
        .enableAutoResponse = false,
        .enablePeriodicTest = true,
        .periodicIntervalFrames = 3
    };
    
    TRM_TxValidatorInit(&config);
    
    /* 模拟多帧接收 */
    for (uint32_t frame = 200; frame < 210; frame++) {
        TRM_RxDataList rxDataList = {
            ,
            .userCount = 1,
            .frameNo = frame,
            .users = {
                {
                    .userId = 0xABCDEF00,
                    .data = (uint8_t[]){frame & 0xFF},
                    .dataLen = 1,
                    .rssi = -40,
                    .snr = 15,
                    .freq = 2402000000,
                    .beam = {.userId = 0xABCDEF00, .timestamp = frame}
                }
            }
        };
        
        TRM_TxValidatorOnRxData(&rxDataList);
        
        /* 显示当前统计 */
        TRM_TxValidatorStats stats;
        if (TRM_TxValidatorGetStats(&stats) == TRM_OK) {
            printf("�?u: 总触�?%u, 成功=%u\n", 
                   frame, stats.totalTriggerCount, stats.successSendCount);
        }
        
        usleep(100000); /* 模拟帧间�?*/
    }
    
    TRM_TxValidatorDeinit();
    printf("\n");
}

/* ============================================================================
 * 示例3: 手动触发
 * ============================================================================
 */

void example_manual_trigger(void)
{
    printf("=== 示例3: 手动触发 ===\n");
    
    /* 配置手动模式 */
    TRM_TxValidatorConfig config = {
        .txPower = 15,
        .frameOffset = 2,
        .userIdBase = 0x50000000,
        .enableAutoResponse = false,
        .enablePeriodicTest = false,
        .periodicIntervalFrames = 0
    };
    
    TRM_TxValidatorInit(&config);
    
    /* 手动触发多次发�?*/
    for (int i = 0; i < 5; i++) {
        uint8_t testData[] = {0xAA, 0xBB, 0xCC, 0xDD, (uint8_t)i};
        uint32_t userId = 0x50000000 + i;
        
        int ret = TRM_TxValidatorTriggerTest(testData, sizeof(testData), userId);
        printf("手动触发[%d]: 用户ID=0x%08X, 结果=%d\n", i, userId, ret);
        
        sleep(1); /* 等待1�?*/
    }
    
    /* 显示最终统�?*/
    TRM_TxValidatorStats stats;
    if (TRM_TxValidatorGetStats(&stats) == TRM_OK) {
        printf("最终统�? 总触�?%u, 成功=%u, 失败=%u\n", 
               stats.totalTriggerCount, stats.successSendCount, stats.failedSendCount);
    }
    
    TRM_TxValidatorDeinit();
    printf("\n");
}

/* ============================================================================
 * 示例4: 配置动态调�?
 * ============================================================================
 */

void example_config_adjustment(void)
{
    printf("=== 示例4: 配置动态调�?===\n");
    
    /* 初始配置 */
    TRM_TxValidatorConfig config = {
        .txPower = 10,
        .frameOffset = 3,
        .userIdBase = 0x60000000,
        .enableAutoResponse = true,
        .enablePeriodicTest = false,
        .periodicIntervalFrames = 0
    };
    
    TRM_TxValidatorInit(&config);
    
    /* 模拟一些接收数�?*/
    TRM_RxDataList rxDataList = {
        ,
        .userCount = 1,
        .frameNo = 300,
        .users = {
            {
                .userId = 0x11111111,
                .data = (uint8_t[]){0x11},
                .dataLen = 1,
                .rssi = -30,
                .snr = 20,
                .freq = 2403000000,
                .beam = {.userId = 0x11111111, .timestamp = 300}
            }
        }
    };
    
    TRM_TxValidatorOnRxData(&rxDataList);
    
    /* 调整配置 */
    config.txPower = 30;
    config.frameOffset = 8;
    config.enablePeriodicTest = true;
    config.periodicIntervalFrames = 5;
    
    TRM_TxValidatorSetConfig(&config);
    printf("配置已调�? 功率=%d, 帧偏�?%d, 启用周期测试\n", 
           config.txPower, config.frameOffset);
    
    /* 继续测试 */
    for (uint32_t frame = 301; frame < 310; frame++) {
        rxDataList.frameNo = frame;
        TRM_TxValidatorOnRxData(&rxDataList);
    }
    
    /* 显示统计 */
    TRM_TxValidatorStats stats;
    TRM_TxValidatorGetStats(&stats);
    printf("调整后统�? 总触�?%u, 成功=%u, 失败=%u\n", 
           stats.totalTriggerCount, stats.successSendCount, stats.failedSendCount);
    
    TRM_TxValidatorDeinit();
    printf("\n");
}

/* ============================================================================
 * 主函�?
 * ============================================================================
 */

int main(void)
{
    printf("TRM发送验证器使用示例\n");
    printf("====================\n\n");
    
    /* 运行所有示�?*/
    example_basic_usage();
    example_periodic_test();
    example_manual_trigger();
    example_config_adjustment();
    
    printf("所有示例运行完成\n");
    return 0;
}
