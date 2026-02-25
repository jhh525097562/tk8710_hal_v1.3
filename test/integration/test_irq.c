/**
 * @file test_irq.c
 * @brief TK8710中断处理集成测试
 * @note 测试中断触发、处理和回调机制
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

#define TEST_IRQ_TIMEOUT_MS    5000
#define TEST_IRQ_COUNT         10

/*============================================================================
 * 测试数据结构
 *============================================================================*/

/** 中断测试统计 */
typedef struct {
    uint32_t slot_end_count;        /**< 时隙结束中断次数 */
    uint32_t tx_slot_count;          /**< 发送时隙中断次数 */
    uint32_t rx_slot_count;          /**< 接收时隙中断次数 */
    uint32_t error_count;            /**< 错误中断次数 */
    uint32_t total_irq_count;        /**< 总中断次数 */
    uint32_t last_irq_time;          /**< 最后中断时间 */
    bool irq_received;               /**< 中断接收标志 */
    uint8_t last_slot_type;          /**< 最后时隙类型 */
    uint8_t last_slot_index;         /**< 最后时隙索引 */
    uint32_t last_frame_no;          /**< 最后帧号 */
} irq_test_stats_t;

/** 测试上下文 */
typedef struct {
    trm_config_t trm_config;          /**< TRM配置 */
    irq_test_stats_t irq_stats;       /**< 中断统计 */
    bool test_running;                 /**< 测试运行标志 */
} irq_test_context_t;

/* 全局测试上下文 */
static irq_test_context_t g_irq_test_ctx = {0};

/*============================================================================
 * 测试回调函数
 *============================================================================*/

/**
 * @brief 时隙结束回调
 * @param slot_type 时隙类型
 * @param slot_index 时隙索引
 * @param frame_no 帧号
 */
static void test_slot_end_callback(uint8_t slot_type, uint8_t slot_index, uint32_t frame_no)
{
    g_irq_test_ctx.irq_stats.slot_end_count++;
    g_irq_test_ctx.irq_stats.total_irq_count++;
    g_irq_test_ctx.irq_stats.last_irq_time = trm_get_tick_ms();
    g_irq_test_ctx.irq_stats.irq_received = true;
    g_irq_test_ctx.irq_stats.last_slot_type = slot_type;
    g_irq_test_ctx.irq_stats.last_slot_index = slot_index;
    g_irq_test_ctx.irq_stats.last_frame_no = frame_no;
    
    printf("Slot end: type=%d, index=%d, frame=%u\n", slot_type, slot_index, frame_no);
}

/**
 * @brief 发送时隙回调
 * @param slot_index 时隙索引
 * @param max_user_count 最大用户数
 */
static void test_tx_slot_callback(uint8_t slot_index, uint8_t max_user_count)
{
    g_irq_test_ctx.irq_stats.tx_slot_count++;
    g_irq_test_ctx.irq_stats.total_irq_count++;
    g_irq_test_ctx.irq_stats.last_irq_time = trm_get_tick_ms();
    g_irq_test_ctx.irq_stats.irq_received = true;
    
    printf("TX slot: index=%d, max_users=%d\n", slot_index, max_user_count);
}

/**
 * @brief 接收时隙回调
 * @param slot_index 时隙索引
 * @param max_user_count 最大用户数
 */
static void test_rx_slot_callback(uint8_t slot_index, uint8_t max_user_count)
{
    g_irq_test_ctx.irq_stats.rx_slot_count++;
    g_irq_test_ctx.irq_stats.total_irq_count++;
    g_irq_test_ctx.irq_stats.last_irq_time = trm_get_tick_ms();
    g_irq_test_ctx.irq_stats.irq_received = true;
    
    printf("RX slot: index=%d, max_users=%d\n", slot_index, max_user_count);
}

/**
 * @brief 错误回调
 * @param error_code 错误代码
 * @param error_info 错误信息
 */
static void test_error_callback(int error_code, const char* error_info)
{
    g_irq_test_ctx.irq_stats.error_count++;
    g_irq_test_ctx.irq_stats.total_irq_count++;
    g_irq_test_ctx.irq_stats.last_irq_time = trm_get_tick_ms();
    
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
    memset(&g_irq_test_ctx, 0, sizeof(irq_test_context_t));
    
    /* 配置TRM */
    g_irq_test_ctx.trm_config.mode = TRM_MODE_MASTER;
    g_irq_test_ctx.trm_config.beam_mode = TRM_BEAM_STORE_ENABLE;
    g_irq_test_ctx.trm_config.data_mode = TRM_DATA_MODE_BROADCAST;
    g_irq_test_ctx.trm_config.max_users = 64;
    g_irq_test_ctx.trm_config.max_beams = 128;
    g_irq_test_ctx.trm_config.max_slots = 16;
    g_irq_test_ctx.trm_config.buffer_size = 4096;
    g_irq_test_ctx.trm_config.enable_irq = true;
    g_irq_test_ctx.trm_config.enable_log = true;
    
    /* 初始化TRM */
    int ret = trm_init(&g_irq_test_ctx.trm_config);
    TEST_ASSERT(ret == 0, "TRM initialization failed");
    
    /* 注册中断回调 */
    trm_irq_callbacks_t irq_callbacks = {
        .on_slot_end = test_slot_end_callback,
        .on_tx_slot = test_tx_slot_callback,
        .on_rx_slot = test_rx_slot_callback,
        .on_error = test_error_callback
    };
    
    ret = trm_register_irq_callbacks(&irq_callbacks);
    TEST_ASSERT(ret == 0, "Register IRQ callbacks failed");
    
    g_irq_test_ctx.test_running = true;
    
    return 0;
}

/**
 * @brief 清理测试环境
 */
static void test_cleanup(void)
{
    g_irq_test_ctx.test_running = false;
    trm_deinit();
    memset(&g_irq_test_ctx, 0, sizeof(irq_test_context_t));
}

/**
 * @brief 等待中断
 * @param timeout_ms 超时时间(毫秒)
 * @return 0-成功, 非0-失败
 */
static int test_wait_for_irq(uint32_t timeout_ms)
{
    uint32_t start_time = trm_get_tick_ms();
    
    while (!g_irq_test_ctx.irq_stats.irq_received && g_irq_test_ctx.test_running) {
        uint32_t current_time = trm_get_tick_ms();
        if ((current_time - start_time) > timeout_ms) {
            printf("Timeout waiting for IRQ\n");
            return -1;
        }
        
        /* 处理TRM事件 */
        trm_process_events();
        
        /* 短暂延时 */
        trm_delay_ms(10);
    }
    
    return 0;
}

/**
 * @brief 重置中断统计
 */
static void test_reset_irq_stats(void)
{
    memset(&g_irq_test_ctx.irq_stats, 0, sizeof(irq_test_stats_t));
}

/*============================================================================
 * 中断功能测试
 *============================================================================*/

/**
 * @brief 测试中断初始化
 */
static int test_irq_init(void)
{
    int ret = test_init();
    TEST_ASSERT(ret == 0, "Test initialization failed");
    
    /* 验证中断使能 */
    bool irq_enabled = trm_is_irq_enabled();
    TEST_ASSERT(irq_enabled, "IRQ should be enabled");
    
    /* 获取中断配置 */
    trm_irq_config_t irq_config;
    ret = trm_get_irq_config(&irq_config);
    TEST_ASSERT(ret == 0, "Get IRQ config failed");
    TEST_ASSERT(irq_config.enabled, "IRQ config should be enabled");
    
    test_cleanup();
    TEST_SUCCESS();
}

/**
 * @brief 测试时隙中断
 */
static int test_slot_irq(void)
{
    int ret = test_init();
    TEST_ASSERT(ret == 0, "Test initialization failed");
    
    /* 分配时隙 */
    uint8_t slot_id = trm_slot_allocate(0, 1000, 1, 100);
    TEST_ASSERT(slot_id != 0xFF, "Slot allocation failed");
    
    /* 启动时隙 */
    ret = trm_slot_start(slot_id);
    TEST_ASSERT(ret == 0, "Slot start failed");
    
    /* 等待时隙中断 */
    ret = test_wait_for_irq(TEST_IRQ_TIMEOUT_MS);
    TEST_ASSERT(ret == 0, "Slot IRQ timeout");
    
    /* 验证中断统计 */
    TEST_ASSERT(g_irq_test_ctx.irq_stats.slot_end_count > 0, "Slot end IRQ not received");
    TEST_ASSERT(g_irq_test_ctx.irq_stats.last_slot_type == 0, "Slot type mismatch");
    
    /* 停止时隙 */
    trm_slot_stop(slot_id);
    trm_slot_release(slot_id);
    
    test_cleanup();
    TEST_SUCCESS();
}

/**
 * @brief 测试发送中断
 */
static int test_tx_irq(void)
{
    int ret = test_init();
    TEST_ASSERT(ret == 0, "Test initialization failed");
    
    /* 分配发送波束 */
    uint16_t beam_id = trm_beam_allocate(1, 503100000, 30, 0);
    TEST_ASSERT(beam_id != 0xFFFF, "Beam allocation failed");
    
    /* 分配发送时隙 */
    uint8_t slot_id = trm_slot_allocate(1, 1000, 1, beam_id);
    TEST_ASSERT(slot_id != 0xFF, "TX slot allocation failed");
    
    /* 启动发送 */
    uint8_t test_data[64] = {0};
    ret = trm_send_broadcast(1, test_data, 64, beam_id);
    TEST_ASSERT(ret == 0, "Send broadcast failed");
    
    /* 等待发送中断 */
    ret = test_wait_for_irq(TEST_IRQ_TIMEOUT_MS);
    TEST_ASSERT(ret == 0, "TX IRQ timeout");
    
    /* 验证中断统计 */
    TEST_ASSERT(g_irq_test_ctx.irq_stats.tx_slot_count > 0, "TX slot IRQ not received");
    
    /* 清理 */
    trm_slot_stop(slot_id);
    trm_slot_release(slot_id);
    trm_beam_release(beam_id);
    
    test_cleanup();
    TEST_SUCCESS();
}

/**
 * @brief 测试接收中断
 */
static int test_rx_irq(void)
{
    int ret = test_init();
    TEST_ASSERT(ret == 0, "Test initialization failed");
    
    /* 分配接收波束 */
    uint16_t beam_id = trm_beam_allocate(2, 503100000, 30, 1);
    TEST_ASSERT(beam_id != 0xFFFF, "Beam allocation failed");
    
    /* 分配接收时隙 */
    uint8_t slot_id = trm_slot_allocate(2, 1000, 2, beam_id);
    TEST_ASSERT(slot_id != 0xFF, "RX slot allocation failed");
    
    /* 启动接收 */
    ret = trm_start_receive(2, beam_id);
    TEST_ASSERT(ret == 0, "Start receive failed");
    
    /* 等待接收中断 */
    ret = test_wait_for_irq(TEST_IRQ_TIMEOUT_MS);
    TEST_ASSERT(ret == 0, "RX IRQ timeout");
    
    /* 验证中断统计 */
    TEST_ASSERT(g_irq_test_ctx.irq_stats.rx_slot_count > 0, "RX slot IRQ not received");
    
    /* 清理 */
    trm_stop_receive(2);
    trm_slot_stop(slot_id);
    trm_slot_release(slot_id);
    trm_beam_release(beam_id);
    
    test_cleanup();
    TEST_SUCCESS();
}

/**
 * @brief 测试中断频率
 */
static int test_irq_frequency(void)
{
    int ret = test_init();
    TEST_ASSERT(ret == 0, "Test initialization failed");
    
    /* 分配多个时隙 */
    uint8_t slot_ids[4];
    for (int i = 0; i < 4; i++) {
        slot_ids[i] = trm_slot_allocate(i, 500, i + 1, i * 100);
        TEST_ASSERT(slot_ids[i] != 0xFF, "Slot allocation failed");
        
        ret = trm_slot_start(slot_ids[i]);
        TEST_ASSERT(ret == 0, "Slot start failed");
    }
    
    /* 测量中断频率 */
    test_reset_irq_stats();
    uint32_t start_time = trm_get_tick_ms();
    uint32_t target_irq_count = TEST_IRQ_COUNT;
    
    while (g_irq_test_ctx.irq_stats.total_irq_count < target_irq_count && 
           g_irq_test_ctx.test_running) {
        trm_process_events();
        trm_delay_ms(10);
        
        /* 超时检查 */
        uint32_t current_time = trm_get_tick_ms();
        if ((current_time - start_time) > TEST_IRQ_TIMEOUT_MS) {
            break;
        }
    }
    
    uint32_t end_time = trm_get_tick_ms();
    uint32_t duration_ms = end_time - start_time;
    
    /* 验证中断频率 */
    TEST_ASSERT(g_irq_test_ctx.irq_stats.total_irq_count >= target_irq_count, 
                "IRQ count insufficient");
    
    float irq_freq = (float)g_irq_test_ctx.irq_stats.total_irq_count * 1000.0f / duration_ms;
    printf("IRQ frequency: %.1f Hz (%d IRQs in %u ms)\n", 
           irq_freq, g_irq_test_ctx.irq_stats.total_irq_count, duration_ms);
    
    /* 清理 */
    for (int i = 0; i < 4; i++) {
        trm_slot_stop(slot_ids[i]);
        trm_slot_release(slot_ids[i]);
    }
    
    test_cleanup();
    TEST_SUCCESS();
}

/**
 * @brief 测试中断错误处理
 */
static int test_irq_error_handling(void)
{
    int ret = test_init();
    TEST_ASSERT(ret == 0, "Test initialization failed");
    
    /* 测试无效时隙中断 */
    ret = trm_slot_start(0xFF);
    TEST_ASSERT(ret != 0, "Invalid slot start should fail");
    
    /* 测试重复启动时隙 */
    uint8_t slot_id = trm_slot_allocate(0, 1000, 1, 100);
    TEST_ASSERT(slot_id != 0xFF, "Slot allocation failed");
    
    ret = trm_slot_start(slot_id);
    TEST_ASSERT(ret == 0, "First slot start should succeed");
    
    ret = trm_slot_start(slot_id);
    TEST_ASSERT(ret != 0, "Duplicate slot start should fail");
    
    /* 等待可能的错误中断 */
    trm_delay_ms(100);
    
    /* 清理 */
    trm_slot_stop(slot_id);
    trm_slot_release(slot_id);
    
    test_cleanup();
    TEST_SUCCESS();
}

/*============================================================================
 * 测试套件
 *============================================================================*/

/**
 * @brief 中断测试套件
 */
static const struct {
    const char* name;
    int (*test_func)(void);
} irq_test_suite[] = {
    {"IRQ Initialization", test_irq_init},
    {"Slot IRQ", test_slot_irq},
    {"TX IRQ", test_tx_irq},
    {"RX IRQ", test_rx_irq},
    {"IRQ Frequency", test_irq_frequency},
    {"IRQ Error Handling", test_irq_error_handling},
    {NULL, NULL}
};

/**
 * @brief 运行中断测试套件
 */
int main(void)
{
    printf("=== TK8710 IRQ Integration Test Suite ===\n");
    
    int passed = 0;
    int failed = 0;
    int total = 0;
    
    for (int i = 0; irq_test_suite[i].name != NULL; i++) {
        printf("\n[%d] %s\n", total + 1, irq_test_suite[i].name);
        
        int result = irq_test_suite[i].test_func();
        
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
