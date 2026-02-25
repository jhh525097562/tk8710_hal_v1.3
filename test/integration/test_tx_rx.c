/**
 * @file test_tx_rx.c
 * @brief TK8710收发集成测试
 * @note 测试完整的发送和接收流程
 */

#include "../../inc/trm/trm.h"
#include "../../inc/trm/trm_types.h"
#include "../../inc/trm/trm_config.h"
#include "../../inc/driver/tk8710.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*============================================================================
 * 测试宏定义
 *============================================================================*/

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s - %s\n", __FUNCTION__, message); \
            return -1; \
        } else { \
            printf("PASS: %s - %s\n", __FUNCTION__, message); \
        } \
    } while(0)

#define TEST_SUCCESS() \
    do { \
        printf("SUCCESS: %s\n", __FUNCTION__); \
        return 0; \
    } while(0)

#define TEST_DATA_SIZE     256
#define TEST_USER_ID       1
#define TEST_FREQUENCY_HZ  503100000
#define TEST_POWER_DBM     30

/*============================================================================
 * 测试数据结构
 *============================================================================*/

/** 测试上下文 */
typedef struct {
    trm_config_t trm_config;          /**< TRM配置 */
    uint8_t test_data[TEST_DATA_SIZE]; /**< 测试数据 */
    uint8_t rx_data[TEST_DATA_SIZE];   /**< 接收数据 */
    uint16_t tx_beam_id;              /**< 发送波束ID */
    uint16_t rx_beam_id;              /**< 接收波束ID */
    bool data_received;               /**< 数据接收标志 */
    uint32_t rx_bytes;                /**< 接收字节数 */
} test_context_t;

/* 全局测试上下文 */
static test_context_t g_test_ctx = {0};

/*============================================================================
 * 测试回调函数
 *============================================================================*/

/**
 * @brief 数据接收回调
 * @param user_id 用户ID
 * @param data 接收数据
 * @param length 数据长度
 * @param beam_id 波束ID
 */
static void test_data_receive_callback(uint8_t user_id, const uint8_t* data, 
                                       uint16_t length, uint16_t beam_id)
{
    printf("Data received: user=%d, length=%d, beam=%d\n", user_id, length, beam_id);
    
    if (length <= TEST_DATA_SIZE && data) {
        memcpy(g_test_ctx.rx_data, data, length);
        g_test_ctx.rx_bytes = length;
        g_test_ctx.data_received = true;
        g_test_ctx.rx_beam_id = beam_id;
    }
}

/**
 * @brief 发送完成回调
 * @param user_id 用户ID
 * @param beam_id 波束ID
 * @param result 发送结果
 */
static void test_tx_complete_callback(uint8_t user_id, uint16_t beam_id, int result)
{
    printf("TX complete: user=%d, beam=%d, result=%d\n", user_id, beam_id, result);
}

/**
 * @brief 错误回调
 * @param error_code 错误代码
 * @param error_info 错误信息
 */
static void test_error_callback(int error_code, const char* error_info)
{
    printf("Error: code=%d, info=%s\n", error_code, error_info ? error_info : "Unknown");
}

/*============================================================================
 * 测试辅助函数
 *============================================================================*/

/**
 * @brief 初始化测试环境
 */
static int test_init(void)
{
    /* 清零测试上下文 */
    memset(&g_test_ctx, 0, sizeof(test_context_t));
    
    /* 生成测试数据 */
    for (int i = 0; i < TEST_DATA_SIZE; i++) {
        g_test_ctx.test_data[i] = (uint8_t)(i & 0xFF);
    }
    
    /* 配置TRM */
    g_test_ctx.trm_config.mode = TRM_MODE_MASTER;
    g_test_ctx.trm_config.beam_mode = TRM_BEAM_STORE_ENABLE;
    g_test_ctx.trm_config.data_mode = TRM_DATA_MODE_BROADCAST;
    g_test_ctx.trm_config.max_users = 64;
    g_test_ctx.trm_config.max_beams = 128;
    g_test_ctx.trm_config.max_slots = 16;
    g_test_ctx.trm_config.buffer_size = 4096;
    g_test_ctx.trm_config.enable_irq = true;
    g_test_ctx.trm_config.enable_log = true;
    
    /* 初始化TRM */
    int ret = trm_init(&g_test_ctx.trm_config);
    TEST_ASSERT(ret == 0, "TRM initialization failed");
    
    /* 注册回调函数 */
    trm_callbacks_t callbacks = {
        .on_data_receive = test_data_receive_callback,
        .on_tx_complete = test_tx_complete_callback,
        .on_error = test_error_callback
    };
    
    ret = trm_register_callbacks(&callbacks);
    TEST_ASSERT(ret == 0, "Register callbacks failed");
    
    return 0;
}

/**
 * @brief 清理测试环境
 */
static void test_cleanup(void)
{
    trm_deinit();
    memset(&g_test_ctx, 0, sizeof(test_context_t));
}

/**
 * @brief 等待数据接收
 * @param timeout_ms 超时时间(毫秒)
 * @return 0-成功, 非0-失败
 */
static int test_wait_for_receive(uint32_t timeout_ms)
{
    uint32_t start_time = trm_get_tick_ms();
    
    while (!g_test_ctx.data_received) {
        uint32_t current_time = trm_get_tick_ms();
        if ((current_time - start_time) > timeout_ms) {
            printf("Timeout waiting for data receive\n");
            return -1;
        }
        
        /* 处理TRM事件 */
        trm_process_events();
        
        /* 短暂延时 */
        trm_delay_ms(10);
    }
    
    return 0;
}

/*============================================================================
 * 收发功能测试
 *============================================================================*/

/**
 * @brief 测试基本发送功能
 */
static int test_basic_transmit(void)
{
    int ret = test_init();
    TEST_ASSERT(ret == 0, "Test initialization failed");
    
    /* 分配发送波束 */
    g_test_ctx.tx_beam_id = trm_beam_allocate(TEST_USER_ID, TEST_FREQUENCY_HZ, 
                                              TEST_POWER_DBM, 0);
    TEST_ASSERT(g_test_ctx.tx_beam_id != 0xFFFF, "TX beam allocation failed");
    
    /* 发送数据 */
    ret = trm_send_broadcast(TEST_USER_ID, g_test_ctx.test_data, 
                            TEST_DATA_SIZE, g_test_ctx.tx_beam_id);
    TEST_ASSERT(ret == 0, "Data transmission failed");
    
    /* 等待发送完成 */
    trm_delay_ms(100);
    
    /* 清理 */
    trm_beam_release(g_test_ctx.tx_beam_id);
    
    test_cleanup();
    TEST_SUCCESS();
}

/**
 * @brief 测试基本接收功能
 */
static int test_basic_receive(void)
{
    int ret = test_init();
    TEST_ASSERT(ret == 0, "Test initialization failed");
    
    /* 分配接收波束 */
    g_test_ctx.rx_beam_id = trm_beam_allocate(TEST_USER_ID, TEST_FREQUENCY_HZ, 
                                              TEST_POWER_DBM, 0);
    TEST_ASSERT(g_test_ctx.rx_beam_id != 0xFFFF, "RX beam allocation failed");
    
    /* 启动接收 */
    ret = trm_start_receive(TEST_USER_ID, g_test_ctx.rx_beam_id);
    TEST_ASSERT(ret == 0, "Start receive failed");
    
    /* 模拟数据接收（实际测试中需要硬件支持） */
    trm_delay_ms(100);
    
    /* 清理 */
    trm_stop_receive(TEST_USER_ID);
    trm_beam_release(g_test_ctx.rx_beam_id);
    
    test_cleanup();
    TEST_SUCCESS();
}

/**
 * @brief 测试双向通信
 */
static int test_bidirectional_communication(void)
{
    int ret = test_init();
    TEST_ASSERT(ret == 0, "Test initialization failed");
    
    /* 分配发送和接收波束 */
    g_test_ctx.tx_beam_id = trm_beam_allocate(TEST_USER_ID, TEST_FREQUENCY_HZ, 
                                              TEST_POWER_DBM, 0);
    g_test_ctx.rx_beam_id = trm_beam_allocate(TEST_USER_ID, TEST_FREQUENCY_HZ + 1000000, 
                                              TEST_POWER_DBM, 1);
    
    TEST_ASSERT(g_test_ctx.tx_beam_id != 0xFFFF, "TX beam allocation failed");
    TEST_ASSERT(g_test_ctx.rx_beam_id != 0xFFFF, "RX beam allocation failed");
    
    /* 启动接收 */
    ret = trm_start_receive(TEST_USER_ID, g_test_ctx.rx_beam_id);
    TEST_ASSERT(ret == 0, "Start receive failed");
    
    /* 发送数据 */
    ret = trm_send_broadcast(TEST_USER_ID, g_test_ctx.test_data, 
                            TEST_DATA_SIZE, g_test_ctx.tx_beam_id);
    TEST_ASSERT(ret == 0, "Data transmission failed");
    
    /* 等待接收 */
    ret = test_wait_for_receive(1000);
    TEST_ASSERT(ret == 0, "Data receive timeout");
    
    /* 验证接收数据 */
    TEST_ASSERT(g_test_ctx.data_received, "Data not received");
    TEST_ASSERT(g_test_ctx.rx_bytes == TEST_DATA_SIZE, "Data length mismatch");
    ret = memcmp(g_test_ctx.test_data, g_test_ctx.rx_data, TEST_DATA_SIZE);
    TEST_ASSERT(ret == 0, "Data content mismatch");
    
    /* 清理 */
    trm_stop_receive(TEST_USER_ID);
    trm_beam_release(g_test_ctx.tx_beam_id);
    trm_beam_release(g_test_ctx.rx_beam_id);
    
    test_cleanup();
    TEST_SUCCESS();
}

/**
 * @brief 测试多用户通信
 */
static int test_multi_user_communication(void)
{
    int ret = test_init();
    TEST_ASSERT(ret == 0, "Test initialization failed");
    
    const uint8_t user_count = 4;
    uint16_t tx_beams[user_count];
    uint16_t rx_beams[user_count];
    
    /* 为每个用户分配波束 */
    for (uint8_t i = 0; i < user_count; i++) {
        tx_beams[i] = trm_beam_allocate(i, TEST_FREQUENCY_HZ + i * 1000000, 
                                       TEST_POWER_DBM, i);
        rx_beams[i] = trm_beam_allocate(i, TEST_FREQUENCY_HZ + (i + 4) * 1000000, 
                                       TEST_POWER_DBM, i);
        
        TEST_ASSERT(tx_beams[i] != 0xFFFF, "TX beam allocation failed");
        TEST_ASSERT(rx_beams[i] != 0xFFFF, "RX beam allocation failed");
        
        /* 启动接收 */
        ret = trm_start_receive(i, rx_beams[i]);
        TEST_ASSERT(ret == 0, "Start receive failed");
    }
    
    /* 每个用户发送数据 */
    for (uint8_t i = 0; i < user_count; i++) {
        /* 生成用户特定数据 */
        for (int j = 0; j < TEST_DATA_SIZE; j++) {
            g_test_ctx.test_data[j] = (uint8_t)((i << 4) | (j & 0x0F));
        }
        
        ret = trm_send_broadcast(i, g_test_ctx.test_data, 
                                 TEST_DATA_SIZE, tx_beams[i]);
        TEST_ASSERT(ret == 0, "Data transmission failed");
        
        trm_delay_ms(50); /* 避免冲突 */
    }
    
    /* 等待所有数据接收 */
    trm_delay_ms(1000);
    
    /* 清理 */
    for (uint8_t i = 0; i < user_count; i++) {
        trm_stop_receive(i);
        trm_beam_release(tx_beams[i]);
        trm_beam_release(rx_beams[i]);
    }
    
    test_cleanup();
    TEST_SUCCESS();
}

/**
 * @brief 测试错误处理
 */
static int test_error_handling(void)
{
    int ret = test_init();
    TEST_ASSERT(ret == 0, "Test initialization failed");
    
    /* 测试无效波束ID发送 */
    ret = trm_send_broadcast(TEST_USER_ID, g_test_ctx.test_data, 
                            TEST_DATA_SIZE, 0xFFFF);
    TEST_ASSERT(ret != 0, "Invalid beam ID should fail");
    
    /* 测试无效用户ID接收 */
    ret = trm_start_receive(0xFF, 0);
    TEST_ASSERT(ret != 0, "Invalid user ID should fail");
    
    /* 测试空指针发送 */
    ret = trm_send_broadcast(TEST_USER_ID, NULL, 0, 0);
    TEST_ASSERT(ret != 0, "Null pointer should fail");
    
    /* 测试过大数据发送 */
    ret = trm_send_broadcast(TEST_USER_ID, g_test_ctx.test_data, 
                            10000, 0);
    TEST_ASSERT(ret != 0, "Too large data should fail");
    
    test_cleanup();
    TEST_SUCCESS();
}

/*============================================================================
 * 测试套件
 *============================================================================*/

/**
 * @brief 收发测试套件
 */
static const struct {
    const char* name;
    int (*test_func)(void);
} tx_rx_test_suite[] = {
    {"Basic Transmit", test_basic_transmit},
    {"Basic Receive", test_basic_receive},
    {"Bidirectional Communication", test_bidirectional_communication},
    {"Multi-User Communication", test_multi_user_communication},
    {"Error Handling", test_error_handling},
    {NULL, NULL}
};

/**
 * @brief 运行收发测试套件
 */
int main(void)
{
    printf("=== TK8710 TX/RX Integration Test Suite ===\n");
    
    int passed = 0;
    int failed = 0;
    int total = 0;
    
    for (int i = 0; tx_rx_test_suite[i].name != NULL; i++) {
        printf("\n[%d] %s\n", total + 1, tx_rx_test_suite[i].name);
        
        int result = tx_rx_test_suite[i].test_func();
        
        if (result == 0) {
            passed++;
        } else {
            failed++;
        }
        
        total++;
    }
    
    printf("\n=== Test Results ===\n");
    printf("Total: %d\n", total);
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", failed);
    printf("Success Rate: %.1f%%\n", total > 0 ? (float)passed / total * 100.0f : 0.0f);
    
    return failed > 0 ? -1 : 0;
}
