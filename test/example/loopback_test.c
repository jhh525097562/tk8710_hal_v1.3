/**
 * @file loopback_test.c
 * @brief TK8710回环测试示例
 * @note 演示如何进行回环测试验证通信功能
 */

#include "../../inc/trm/trm.h"
#include "../../inc/trm/trm_types.h"
#include "../../inc/trm/trm_config.h"
#include "../../inc/driver/tk8710.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

/*============================================================================
 * 回环测试配置
 *============================================================================*/

#define LOOPBACK_TEST_USERS         2
#define LOOPBACK_PACKET_SIZE        256
#define LOOPBACK_TEST_PACKETS       100
#define LOOPBACK_TIMEOUT_MS         5000
#define LOOPBACK_FREQUENCY_HZ       503100000
#define LOOPBACK_POWER_DBM          20

/*============================================================================
 * 回环测试数据结构
 *============================================================================*/

/** 测试包 */
typedef struct {
    uint32_t packet_id;             /**< 包ID */
    uint32_t timestamp;             /**< 时间戳 */
    uint16_t sequence;              /**< 序列号 */
    uint8_t source_user;            /**< 源用户 */
    uint8_t dest_user;              /**< 目标用户 */
    uint8_t data[LOOPBACK_PACKET_SIZE - 16]; /**< 数据 */
    uint16_t checksum;              /**< 校验和 */
} test_packet_t;

/** 用户测试上下文 */
typedef struct {
    uint8_t user_id;                /**< 用户ID */
    uint16_t tx_beam_id;            /**< 发送波束ID */
    uint16_t rx_beam_id;            /**< 接收波束ID */
    uint32_t tx_packets;            /**< 发送包数 */
    uint32_t rx_packets;            /**< 接收包数 */
    uint32_t tx_bytes;              /**< 发送字节数 */
    uint32_t rx_bytes;              /**< 接收字节数 */
    uint32_t lost_packets;          /**< 丢失包数 */
    uint32_t error_packets;         /**< 错误包数 */
    uint32_t last_rx_seq;           /**< 最后接收序列号 */
    uint32_t min_latency_us;        /**< 最小延迟(微秒) */
    uint32_t max_latency_us;        /**< 最大延迟(微秒) */
    uint32_t total_latency_us;      /**< 总延迟(微秒) */
    bool test_active;                /**< 测试活动标志 */
} user_test_context_t;

/** 回环测试上下文 */
typedef struct {
    trm_config_t trm_config;          /**< TRM配置 */
    user_test_context_t users[LOOPBACK_TEST_USERS]; /**< 用户上下文 */
    uint32_t test_start_time;         /**< 测试开始时间 */
    uint32_t packet_counter;          /**< 包计数器 */
    bool test_running;                /**< 测试运行标志 */
    bool all_tests_completed;         /**< 所有测试完成标志 */
} loopback_test_context_t;

/* 全局测试上下文 */
static loopback_test_context_t g_test_ctx = {0};

/*============================================================================
 * 回环测试辅助函数
 *============================================================================*/

/**
 * @brief 计算校验和
 * @param data 数据
 * @param length 长度
 * @return 校验和
 */
static uint16_t calculate_checksum(const uint8_t* data, size_t length)
{
    uint16_t checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum += data[i];
    }
    return checksum;
}

/**
 * @brief 验证测试包
 * @param packet 测试包
 * @return 0-有效, 非0-无效
 */
static int verify_test_packet(const test_packet_t* packet)
{
    if (!packet) {
        return -1;
    }
    
    /* 验证校验和 */
    uint16_t calculated_checksum = calculate_checksum((const uint8_t*)packet, 
                                                    sizeof(test_packet_t) - 2);
    if (calculated_checksum != packet->checksum) {
        return -2; /* 校验和错误 */
    }
    
    /* 验证用户ID范围 */
    if (packet->source_user >= LOOPBACK_TEST_USERS || 
        packet->dest_user >= LOOPBACK_TEST_USERS) {
        return -3; /* 用户ID无效 */
    }
    
    /* 验证序列号 */
    if (packet->sequence > LOOPBACK_TEST_PACKETS) {
        return -4; /* 序列号超出范围 */
    }
    
    return 0; /* 有效 */
}

/**
 * @brief 生成测试包
 * @param source_user 源用户
 * @param dest_user 目标用户
 * @param packet 输出测试包
 * @return 0-成功, 非0-失败
 */
static int generate_test_packet(uint8_t source_user, uint8_t dest_user, test_packet_t* packet)
{
    if (!packet) {
        return -1;
    }
    
    /* 填充包信息 */
    packet->packet_id = g_test_ctx.packet_counter++;
    packet->timestamp = trm_get_tick_ms();
    packet->sequence = g_test_ctx.users[source_user].tx_packets + 1;
    packet->source_user = source_user;
    packet->dest_user = dest_user;
    
    /* 填充测试数据 */
    for (int i = 0; i < sizeof(packet->data); i++) {
        packet->data[i] = (uint8_t)(packet->sequence + i);
    }
    
    /* 计算校验和 */
    packet->checksum = calculate_checksum((const uint8_t*)packet, 
                                        sizeof(test_packet_t) - 2);
    
    return 0;
}

/**
 * @brief 获取当前时间(微秒)
 * @return 时间(微秒)
 */
static uint32_t get_time_us(void)
{
    return trm_get_tick_ms() * 1000;
}

/*============================================================================
 * 回环测试回调函数
 *============================================================================*/

/**
 * @brief 数据接收回调
 * @param user_id 用户ID
 * @param data 接收数据
 * @param length 数据长度
 * @param beam_id 波束ID
 */
static void loopback_data_receive_callback(uint8_t user_id, const uint8_t* data, 
                                          uint16_t length, uint16_t beam_id)
{
    if (user_id >= LOOPBACK_TEST_USERS || !g_test_ctx.users[user_id].test_active) {
        return;
    }
    
    user_test_context_t* user = &g_test_ctx.users[user_id];
    
    /* 验证数据长度 */
    if (length != sizeof(test_packet_t)) {
        user->error_packets++;
        printf("RX Error: User %d, invalid length %d\n", user_id, length);
        return;
    }
    
    /* 验证测试包 */
    const test_packet_t* packet = (const test_packet_t*)data;
    int ret = verify_test_packet(packet);
    if (ret != 0) {
        user->error_packets++;
        printf("RX Error: User %d, packet verification failed (%d)\n", user_id, ret);
        return;
    }
    
    /* 验证目标用户 */
    if (packet->dest_user != user_id) {
        printf("RX Warning: User %d received packet for user %d\n", 
               user_id, packet->dest_user);
        return;
    }
    
    /* 计算延迟 */
    uint32_t current_time = get_time_us();
    uint32_t latency = current_time - packet->timestamp;
    
    /* 更新统计信息 */
    user->rx_packets++;
    user->rx_bytes += length;
    
    if (user->rx_packets == 1) {
        user->min_latency_us = latency;
        user->max_latency_us = latency;
    } else {
        if (latency < user->min_latency_us) {
            user->min_latency_us = latency;
        }
        if (latency > user->max_latency_us) {
            user->max_latency_us = latency;
        }
    }
    user->total_latency_us += latency;
    
    /* 检查丢包 */
    if (user->last_rx_seq > 0 && packet->sequence != user->last_rx_seq + 1) {
        uint32_t lost = packet->sequence - user->last_rx_seq - 1;
        user->lost_packets += lost;
        printf("Packet loss detected: user %d, lost %u packets\n", user_id, lost);
    }
    user->last_rx_seq = packet->sequence;
    
    printf("RX: User %d <- User %d, seq %u, latency %u us\n", 
           user_id, packet->source_user, packet->sequence, latency);
}

/**
 * @brief 发送完成回调
 * @param user_id 用户ID
 * @param beam_id 波束ID
 * @param result 发送结果
 */
static void loopback_tx_complete_callback(uint8_t user_id, uint16_t beam_id, int result)
{
    if (user_id >= LOOPBACK_TEST_USERS || !g_test_ctx.users[user_id].test_active) {
        return;
    }
    
    user_test_context_t* user = &g_test_ctx.users[user_id];
    
    if (result == 0) {
        printf("TX Complete: User %d, beam %d - SUCCESS\n", user_id, beam_id);
    } else {
        user->error_packets++;
        printf("TX Complete: User %d, beam %d - FAILED (%d)\n", user_id, beam_id, result);
    }
}

/**
 * @brief 错误回调
 * @param error_code 错误代码
 * @param error_info 错误信息
 */
static void loopback_error_callback(int error_code, const char* error_info)
{
    printf("Loopback Error: code=%d, info=%s\n", error_code, error_info ? error_info : "Unknown");
}

/*============================================================================
 * 回环测试核心函数
 *============================================================================*/

/**
 * @brief 初始化回环测试
 * @return 0-成功, 非0-失败
 */
static int loopback_test_init(void)
{
    printf("Initializing Loopback Test...\n");
    
    /* 清零测试上下文 */
    memset(&g_test_ctx, 0, sizeof(loopback_test_context_t));
    g_test_ctx.test_start_time = trm_get_tick_ms();
    
    /* 配置TRM */
    g_test_ctx.trm_config.mode = TRM_MODE_MASTER;
    g_test_ctx.trm_config.beam_mode = TRM_BEAM_STORE_ENABLE;
    g_test_ctx.trm_config.data_mode = TRM_DATA_MODE_BROADCAST;
    g_test_ctx.trm_config.max_users = LOOPBACK_TEST_USERS;
    g_test_ctx.trm_config.max_beams = 32;
    g_test_ctx.trm_config.max_slots = 16;
    g_test_ctx.trm_config.buffer_size = 4096;
    g_test_ctx.trm_config.enable_irq = true;
    g_test_ctx.trm_config.enable_log = true;
    
    /* 初始化TRM */
    int ret = trm_init(&g_test_ctx.trm_config);
    if (ret != 0) {
        printf("TRM initialization failed: %d\n", ret);
        return -1;
    }
    
    /* 注册回调函数 */
    trm_callbacks_t callbacks = {
        .on_data_receive = loopback_data_receive_callback,
        .on_tx_complete = loopback_tx_complete_callback,
        .on_error = loopback_error_callback
    };
    
    ret = trm_register_callbacks(&callbacks);
    if (ret != 0) {
        printf("Register callbacks failed: %d\n", ret);
        return -2;
    }
    
    g_test_ctx.test_running = true;
    
    printf("Loopback test initialized\n");
    return 0;
}

/**
 * @brief 设置用户测试
 * @param user_id 用户ID
 * @return 0-成功, 非0-失败
 */
static int setup_user_test(uint8_t user_id)
{
    if (user_id >= LOOPBACK_TEST_USERS) {
        return -1;
    }
    
    user_test_context_t* user = &g_test_ctx.users[user_id];
    
    /* 分配发送波束 */
    user->tx_beam_id = trm_beam_allocate(user_id, 
                                        LOOPBACK_FREQUENCY_HZ + user_id * 1000000,
                                        LOOPBACK_POWER_DBM, user_id);
    if (user->tx_beam_id == 0xFFFF) {
        printf("Failed to allocate TX beam for user %d\n", user_id);
        return -2;
    }
    
    /* 分配接收波束 */
    user->rx_beam_id = trm_beam_allocate(user_id,
                                        LOOPBACK_FREQUENCY_HZ + (user_id + 10) * 1000000,
                                        LOOPBACK_POWER_DBM, user_id);
    if (user->rx_beam_id == 0xFFFF) {
        printf("Failed to allocate RX beam for user %d\n", user_id);
        trm_beam_release(user->tx_beam_id);
        return -3;
    }
    
    /* 启动接收 */
    int ret = trm_start_receive(user_id, user->rx_beam_id);
    if (ret != 0) {
        printf("Failed to start receive for user %d\n", user_id);
        trm_beam_release(user->tx_beam_id);
        trm_beam_release(user->rx_beam_id);
        return -4;
    }
    
    /* 初始化用户上下文 */
    user->user_id = user_id;
    user->tx_packets = 0;
    user->rx_packets = 0;
    user->tx_bytes = 0;
    user->rx_bytes = 0;
    user->lost_packets = 0;
    user->error_packets = 0;
    user->last_rx_seq = 0;
    user->min_latency_us = 0;
    user->max_latency_us = 0;
    user->total_latency_us = 0;
    user->test_active = true;
    
    printf("User %d test setup: TX beam %d, RX beam %d\n", 
           user_id, user->tx_beam_id, user->rx_beam_id);
    
    return 0;
}

/**
 * @brief 清理用户测试
 * @param user_id 用户ID
 */
static void cleanup_user_test(uint8_t user_id)
{
    if (user_id >= LOOPBACK_TEST_USERS) {
        return;
    }
    
    user_test_context_t* user = &g_test_ctx.users[user_id];
    
    if (user->test_active) {
        /* 停止接收 */
        trm_stop_receive(user_id);
        
        /* 释放波束 */
        trm_beam_release(user->tx_beam_id);
        trm_beam_release(user->rx_beam_id);
        
        /* 清零用户上下文 */
        memset(user, 0, sizeof(user_test_context_t));
        
        printf("User %d test cleaned up\n", user_id);
    }
}

/**
 * @brief 执行回环测试
 * @return 0-成功, 非0-失败
 */
static int run_loopback_test(void)
{
    printf("Starting Loopback Test...\n");
    printf("Test Users: %d\n", LOOPBACK_TEST_USERS);
    printf("Packets per User: %d\n", LOOPBACK_TEST_PACKETS);
    printf("Packet Size: %d bytes\n", sizeof(test_packet_t));
    printf("\n");
    
    /* 设置所有用户 */
    for (uint8_t i = 0; i < LOOPBACK_TEST_USERS; i++) {
        int ret = setup_user_test(i);
        if (ret != 0) {
            printf("Failed to setup user %d test: %d\n", i, ret);
            return -1;
        }
    }
    
    /* 等待系统稳定 */
    trm_delay_ms(1000);
    
    /* 执行双向回环测试 */
    for (uint32_t seq = 1; seq <= LOOPBACK_TEST_PACKETS; seq++) {
        /* 用户0 -> 用户1 */
        test_packet_t packet01;
        int ret = generate_test_packet(0, 1, &packet01);
        if (ret == 0) {
            ret = trm_send_broadcast(0, (const uint8_t*)&packet01, 
                                   sizeof(packet01), g_test_ctx.users[0].tx_beam_id);
            if (ret == 0) {
                g_test_ctx.users[0].tx_packets++;
                g_test_ctx.users[0].tx_bytes += sizeof(packet01);
                printf("TX: User 0 -> User 1, seq %u\n", seq);
            }
        }
        
        /* 用户1 -> 用户0 */
        test_packet_t packet10;
        ret = generate_test_packet(1, 0, &packet10);
        if (ret == 0) {
            ret = trm_send_broadcast(1, (const uint8_t*)&packet10, 
                                   sizeof(packet10), g_test_ctx.users[1].tx_beam_id);
            if (ret == 0) {
                g_test_ctx.users[1].tx_packets++;
                g_test_ctx.users[1].tx_bytes += sizeof(packet10);
                printf("TX: User 1 -> User 0, seq %u\n", seq);
            }
        }
        
        /* 等待传输完成 */
        trm_delay_ms(100);
        
        /* 处理事件 */
        trm_process_events();
        
        /* 检查测试完成 */
        if (g_test_ctx.users[0].rx_packets >= LOOPBACK_TEST_PACKETS &&
            g_test_ctx.users[1].rx_packets >= LOOPBACK_TEST_PACKETS) {
            printf("All packets received, completing test...\n");
            break;
        }
    }
    
    /* 等待剩余包接收 */
    uint32_t wait_start = trm_get_tick_ms();
    while ((trm_get_tick_ms() - wait_start) < LOOPBACK_TIMEOUT_MS) {
        trm_process_events();
        trm_delay_ms(100);
        
        if (g_test_ctx.users[0].rx_packets >= LOOPBACK_TEST_PACKETS &&
            g_test_ctx.users[1].rx_packets >= LOOPBACK_TEST_PACKETS) {
            break;
        }
    }
    
    g_test_ctx.all_tests_completed = true;
    
    printf("Loopback test completed\n");
    return 0;
}

/**
 * @brief 打印测试结果
 */
static void print_test_results(void)
{
    uint32_t test_duration = trm_get_tick_ms() - g_test_ctx.test_start_time;
    
    printf("\n=== Loopback Test Results ===\n");
    printf("Test Duration: %u ms (%.1f s)\n", test_duration, test_duration / 1000.0f);
    printf("Expected Packets per User: %d\n", LOOPBACK_TEST_PACKETS);
    printf("\n");
    
    for (uint8_t i = 0; i < LOOPBACK_TEST_USERS; i++) {
        user_test_context_t* user = &g_test_ctx.users[i];
        
        printf("User %d Results:\n", i);
        printf("  TX: %u packets, %u bytes\n", user->tx_packets, user->tx_bytes);
        printf("  RX: %u packets, %u bytes\n", user->rx_packets, user->rx_bytes);
        printf("  Lost: %u packets\n", user->lost_packets);
        printf("  Errors: %u packets\n", user->error_packets);
        
        if (user->rx_packets > 0) {
            float loss_rate = (float)user->lost_packets / 
                            (user->rx_packets + user->lost_packets) * 100.0f;
            float error_rate = (float)user->error_packets / user->rx_packets * 100.0f;
            uint32_t avg_latency = user->total_latency_us / user->rx_packets;
            
            printf("  Loss Rate: %.2f%%\n", loss_rate);
            printf("  Error Rate: %.2f%%\n", error_rate);
            printf("  Latency: min=%u us, max=%u us, avg=%u us\n",
                   user->min_latency_us, user->max_latency_us, avg_latency);
        }
        printf("\n");
    }
    
    /* 总体统计 */
    uint32_t total_tx = 0, total_rx = 0, total_lost = 0, total_errors = 0;
    for (uint8_t i = 0; i < LOOPBACK_TEST_USERS; i++) {
        total_tx += g_test_ctx.users[i].tx_packets;
        total_rx += g_test_ctx.users[i].rx_packets;
        total_lost += g_test_ctx.users[i].lost_packets;
        total_errors += g_test_ctx.users[i].error_packets;
    }
    
    printf("Overall Results:\n");
    printf("  Total TX: %u packets\n", total_tx);
    printf("  Total RX: %u packets\n", total_rx);
    printf("  Total Lost: %u packets\n", total_lost);
    printf("  Total Errors: %u packets\n", total_errors);
    
    if (total_tx > 0) {
        float overall_loss_rate = (float)total_lost / (total_rx + total_lost) * 100.0f;
        float overall_success_rate = (float)total_rx / total_tx * 100.0f;
        
        printf("  Overall Loss Rate: %.2f%%\n", overall_loss_rate);
        printf("  Overall Success Rate: %.2f%%\n", overall_success_rate);
    }
    
    printf("============================\n");
}

/**
 * @brief 反初始化回环测试
 */
static void loopback_test_deinit(void)
{
    printf("Deinitializing loopback test...\n");
    
    g_test_ctx.test_running = false;
    
    /* 清理所有用户测试 */
    for (uint8_t i = 0; i < LOOPBACK_TEST_USERS; i++) {
        cleanup_user_test(i);
    }
    
    /* 反初始化TRM */
    trm_deinit();
    
    printf("Loopback test deinitialized\n");
}

/*============================================================================
 * 主程序
 *============================================================================*/

/* 信号处理 */
static volatile bool g_stop_requested = false;

static void signal_handler(int sig)
{
    printf("\nReceived signal %d, stopping test...\n", sig);
    g_stop_requested = true;
    g_test_ctx.test_running = false;
}

/**
 * @brief 主函数
 */
int main(void)
{
    printf("=== TK8710 Loopback Test ===\n");
    printf("Test Configuration:\n");
    printf("  Users: %d\n", LOOPBACK_TEST_USERS);
    printf("  Packets per User: %d\n", LOOPBACK_TEST_PACKETS);
    printf("  Packet Size: %d bytes\n", sizeof(test_packet_t));
    printf("  Frequency: %u Hz\n", LOOPBACK_FREQUENCY_HZ);
    printf("  Power: %d dBm\n", LOOPBACK_POWER_DBM);
    printf("  Timeout: %u ms\n", LOOPBACK_TIMEOUT_MS);
    printf("\n");
    
    /* 设置信号处理 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* 初始化测试 */
    int ret = loopback_test_init();
    if (ret != 0) {
        printf("Loopback test initialization failed: %d\n", ret);
        return -1;
    }
    
    /* 运行测试 */
    ret = run_loopback_test();
    if (ret != 0) {
        printf("Loopback test failed: %d\n", ret);
    } else {
        /* 打印测试结果 */
        print_test_results();
    }
    
    /* 反初始化测试 */
    loopback_test_deinit();
    
    printf("Loopback test completed\n");
    
    return ret;
}
