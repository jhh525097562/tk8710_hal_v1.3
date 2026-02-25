/**
 * @file gateway_demo.c
 * @brief TK8710网关示例程序
 * @note 演示如何使用TK8710 SDK构建网关应用
 */

#include "../../inc/trm/trm.h"
#include "../../inc/trm/trm_types.h"
#include "../../inc/trm/trm_config.h"
#include "../../inc/driver/tk8710.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

/*============================================================================
 * 示例配置
 *============================================================================*/

#define GATEWAY_MAX_USERS           64
#define GATEWAY_MAX_BEAMS           128
#define GATEWAY_BUFFER_SIZE         8192
#define GATEWAY_FREQUENCY_HZ        503100000
#define GATEWAY_POWER_DBM           30
#define GATEWAY_DEMO_DURATION_SEC   60

/*============================================================================
 * 网关数据结构
 *============================================================================*/

/** 用户信息 */
typedef struct {
    uint8_t user_id;                 /**< 用户ID */
    uint16_t tx_beam_id;             /**< 发送波束ID */
    uint16_t rx_beam_id;             /**< 接收波束ID */
    uint32_t tx_packets;             /**< 发送包数 */
    uint32_t rx_packets;             /**< 接收包数 */
    uint32_t tx_bytes;               /**< 发送字节数 */
    uint32_t rx_bytes;               /**< 接收字节数 */
    bool is_active;                  /**< 活动标志 */
    uint32_t last_activity;          /**< 最后活动时间 */
} gateway_user_t;

/** 网关上下文 */
typedef struct {
    trm_config_t trm_config;          /**< TRM配置 */
    gateway_user_t users[GATEWAY_MAX_USERS]; /**< 用户数组 */
    uint8_t active_user_count;        /**< 活动用户数 */
    uint32_t start_time;              /**< 开始时间 */
    bool running;                     /**< 运行标志 */
    uint8_t gateway_id;               /**< 网关ID */
} gateway_context_t;

/* 全局网关上下文 */
static gateway_context_t g_gateway = {0};

/*============================================================================
 * 网关回调函数
 *============================================================================*/

/**
 * @brief 数据接收回调
 * @param user_id 用户ID
 * @param data 接收数据
 * @param length 数据长度
 * @param beam_id 波束ID
 */
static void gateway_data_receive_callback(uint8_t user_id, const uint8_t* data, 
                                          uint16_t length, uint16_t beam_id)
{
    if (user_id >= GATEWAY_MAX_USERS || !g_gateway.users[user_id].is_active) {
        printf("Received data from unknown user %d\n", user_id);
        return;
    }
    
    gateway_user_t* user = &g_gateway.users[user_id];
    user->rx_packets++;
    user->rx_bytes += length;
    user->last_activity = trm_get_tick_ms();
    
    printf("RX: User %d, %d bytes, beam %d\n", user_id, length, beam_id);
    
    /* 转发数据到其他用户（示例） */
    if (length > 0 && data) {
        /* 这里可以实现数据转发逻辑 */
        printf("Forwarding data from user %d to other users\n", user_id);
    }
}

/**
 * @brief 发送完成回调
 * @param user_id 用户ID
 * @param beam_id 波束ID
 * @param result 发送结果
 */
static void gateway_tx_complete_callback(uint8_t user_id, uint16_t beam_id, int result)
{
    if (user_id >= GATEWAY_MAX_USERS || !g_gateway.users[user_id].is_active) {
        return;
    }
    
    gateway_user_t* user = &g_gateway.users[user_id];
    
    if (result == 0) {
        printf("TX complete: User %d, beam %d - SUCCESS\n", user_id, beam_id);
    } else {
        printf("TX complete: User %d, beam %d - FAILED (%d)\n", user_id, beam_id, result);
    }
}

/**
 * @brief 错误回调
 * @param error_code 错误代码
 * @param error_info 错误信息
 */
static void gateway_error_callback(int error_code, const char* error_info)
{
    printf("Gateway Error: code=%d, info=%s\n", error_code, error_info ? error_info : "Unknown");
}

/**
 * @brief 时隙结束回调
 * @param slot_type 时隙类型
 * @param slot_index 时隙索引
 * @param frame_no 帧号
 */
static void gateway_slot_end_callback(uint8_t slot_type, uint8_t slot_index, uint32_t frame_no)
{
    /* 定期打印统计信息 */
    static uint32_t last_stats_time = 0;
    uint32_t current_time = trm_get_tick_ms();
    
    if (current_time - last_stats_time > 10000) { /* 每10秒 */
        gateway_print_stats();
        last_stats_time = current_time;
    }
}

/*============================================================================
 * 网关辅助函数
 *============================================================================*/

/**
 * @brief 初始化网关
 * @return 0-成功, 非0-失败
 */
static int gateway_init(void)
{
    printf("Initializing TK8710 Gateway...\n");
    
    /* 清零网关上下文 */
    memset(&g_gateway, 0, sizeof(gateway_context_t));
    g_gateway.gateway_id = 0;
    g_gateway.start_time = trm_get_tick_ms();
    
    /* 配置TRM */
    g_gateway.trm_config.mode = TRM_MODE_MASTER;
    g_gateway.trm_config.beam_mode = TRM_BEAM_STORE_ENABLE;
    g_gateway.trm_config.data_mode = TRM_DATA_MODE_BROADCAST;
    g_gateway.trm_config.max_users = GATEWAY_MAX_USERS;
    g_gateway.trm_config.max_beams = GATEWAY_MAX_BEAMS;
    g_gateway.trm_config.max_slots = 32;
    g_gateway.trm_config.buffer_size = GATEWAY_BUFFER_SIZE;
    g_gateway.trm_config.enable_irq = true;
    g_gateway.trm_config.enable_log = true;
    
    /* 初始化TRM */
    int ret = trm_init(&g_gateway.trm_config);
    if (ret != 0) {
        printf("TRM initialization failed: %d\n", ret);
        return -1;
    }
    
    /* 注册回调函数 */
    trm_callbacks_t callbacks = {
        .on_data_receive = gateway_data_receive_callback,
        .on_tx_complete = gateway_tx_complete_callback,
        .on_error = gateway_error_callback
    };
    
    ret = trm_register_callbacks(&callbacks);
    if (ret != 0) {
        printf("Register callbacks failed: %d\n", ret);
        return -2;
    }
    
    /* 注册中断回调 */
    trm_irq_callbacks_t irq_callbacks = {
        .on_slot_end = gateway_slot_end_callback,
        .on_tx_slot = NULL,
        .on_rx_slot = NULL,
        .on_error = gateway_error_callback
    };
    
    ret = trm_register_irq_callbacks(&irq_callbacks);
    if (ret != 0) {
        printf("Register IRQ callbacks failed: %d\n", ret);
        return -3;
    }
    
    g_gateway.running = true;
    
    printf("Gateway initialized successfully\n");
    return 0;
}

/**
 * @brief 反初始化网关
 */
static void gateway_deinit(void)
{
    printf("Deinitializing gateway...\n");
    
    g_gateway.running = false;
    
    /* 停止所有用户 */
    for (uint8_t i = 0; i < GATEWAY_MAX_USERS; i++) {
        if (g_gateway.users[i].is_active) {
            gateway_remove_user(i);
        }
    }
    
    /* 反初始化TRM */
    trm_deinit();
    
    printf("Gateway deinitialized\n");
}

/**
 * @brief 添加用户
 * @param user_id 用户ID
 * @return 0-成功, 非0-失败
 */
static int gateway_add_user(uint8_t user_id)
{
    if (user_id >= GATEWAY_MAX_USERS) {
        printf("Invalid user ID: %d\n", user_id);
        return -1;
    }
    
    if (g_gateway.users[user_id].is_active) {
        printf("User %d already active\n", user_id);
        return -2;
    }
    
    gateway_user_t* user = &g_gateway.users[user_id];
    
    /* 分配发送波束 */
    user->tx_beam_id = trm_beam_allocate(user_id, 
                                        GATEWAY_FREQUENCY_HZ + user_id * 1000000,
                                        GATEWAY_POWER_DBM, user_id);
    if (user->tx_beam_id == 0xFFFF) {
        printf("Failed to allocate TX beam for user %d\n", user_id);
        return -3;
    }
    
    /* 分配接收波束 */
    user->rx_beam_id = trm_beam_allocate(user_id,
                                        GATEWAY_FREQUENCY_HZ + (user_id + 32) * 1000000,
                                        GATEWAY_POWER_DBM, user_id);
    if (user->rx_beam_id == 0xFFFF) {
        printf("Failed to allocate RX beam for user %d\n", user_id);
        trm_beam_release(user->tx_beam_id);
        return -4;
    }
    
    /* 启动接收 */
    int ret = trm_start_receive(user_id, user->rx_beam_id);
    if (ret != 0) {
        printf("Failed to start receive for user %d\n", user_id);
        trm_beam_release(user->tx_beam_id);
        trm_beam_release(user->rx_beam_id);
        return -5;
    }
    
    /* 初始化用户信息 */
    user->user_id = user_id;
    user->tx_packets = 0;
    user->rx_packets = 0;
    user->tx_bytes = 0;
    user->rx_bytes = 0;
    user->is_active = true;
    user->last_activity = trm_get_tick_ms();
    
    g_gateway.active_user_count++;
    
    printf("User %d added: TX beam %d, RX beam %d\n", 
           user_id, user->tx_beam_id, user->rx_beam_id);
    
    return 0;
}

/**
 * @brief 移除用户
 * @param user_id 用户ID
 * @return 0-成功, 非0-失败
 */
static int gateway_remove_user(uint8_t user_id)
{
    if (user_id >= GATEWAY_MAX_USERS || !g_gateway.users[user_id].is_active) {
        printf("Invalid user ID: %d\n", user_id);
        return -1;
    }
    
    gateway_user_t* user = &g_gateway.users[user_id];
    
    /* 停止接收 */
    trm_stop_receive(user_id);
    
    /* 释放波束 */
    trm_beam_release(user->tx_beam_id);
    trm_beam_release(user->rx_beam_id);
    
    /* 清零用户信息 */
    memset(user, 0, sizeof(gateway_user_t));
    
    g_gateway.active_user_count--;
    
    printf("User %d removed\n", user_id);
    
    return 0;
}

/**
 * @brief 发送广播消息
 * @param data 数据
 * @param length 长度
 * @return 0-成功, 非0-失败
 */
static int gateway_send_broadcast(const uint8_t* data, uint16_t length)
{
    if (!data || length == 0) {
        return -1;
    }
    
    int success_count = 0;
    
    /* 向所有活动用户发送广播 */
    for (uint8_t i = 0; i < GATEWAY_MAX_USERS; i++) {
        if (g_gateway.users[i].is_active) {
            int ret = trm_send_broadcast(i, data, length, g_gateway.users[i].tx_beam_id);
            if (ret == 0) {
                g_gateway.users[i].tx_packets++;
                g_gateway.users[i].tx_bytes += length;
                success_count++;
            } else {
                printf("Failed to send broadcast to user %d: %d\n", i, ret);
            }
        }
    }
    
    printf("Broadcast sent to %d users (%d bytes)\n", success_count, length);
    
    return success_count > 0 ? 0 : -1;
}

/**
 * @brief 打印网关统计信息
 */
static void gateway_print_stats(void)
{
    uint32_t current_time = trm_get_tick_ms();
    uint32_t uptime = current_time - g_gateway.start_time;
    
    printf("\n=== Gateway Statistics ===\n");
    printf("Uptime: %u ms (%.1f s)\n", uptime, uptime / 1000.0f);
    printf("Active Users: %d/%d\n", g_gateway.active_user_count, GATEWAY_MAX_USERS);
    
    uint32_t total_tx_packets = 0;
    uint32_t total_rx_packets = 0;
    uint32_t total_tx_bytes = 0;
    uint32_t total_rx_bytes = 0;
    
    printf("\nUser Statistics:\n");
    for (uint8_t i = 0; i < GATEWAY_MAX_USERS; i++) {
        if (g_gateway.users[i].is_active) {
            gateway_user_t* user = &g_gateway.users[i];
            printf("  User %d: TX %u pkts/%u bytes, RX %u pkts/%u bytes\n",
                   i, user->tx_packets, user->tx_bytes,
                   user->rx_packets, user->rx_bytes);
            
            total_tx_packets += user->tx_packets;
            total_rx_packets += user->rx_packets;
            total_tx_bytes += user->tx_bytes;
            total_rx_bytes += user->rx_bytes;
        }
    }
    
    printf("\nTotal Statistics:\n");
    printf("  TX: %u packets, %u bytes\n", total_tx_packets, total_tx_bytes);
    printf("  RX: %u packets, %u bytes\n", total_rx_packets, total_rx_bytes);
    
    if (uptime > 0) {
        float tx_rate = (float)total_tx_bytes * 1000.0f / uptime;
        float rx_rate = (float)total_rx_bytes * 1000.0f / uptime;
        printf("  TX Rate: %.2f B/s\n", tx_rate);
        printf("  RX Rate: %.2f B/s\n", rx_rate);
    }
    
    printf("========================\n\n");
}

/*============================================================================
 * 主程序
 *============================================================================*/

/* 信号处理 */
static volatile bool g_stop_requested = false;

static void signal_handler(int sig)
{
    printf("\nReceived signal %d, stopping gateway...\n", sig);
    g_stop_requested = true;
}

/**
 * @brief 网关主循环
 */
static void gateway_main_loop(void)
{
    printf("Starting gateway main loop...\n");
    
    /* 添加示例用户 */
    for (uint8_t i = 0; i < 4; i++) {
        gateway_add_user(i);
    }
    
    uint32_t last_broadcast_time = trm_get_tick_ms();
    uint8_t broadcast_counter = 0;
    
    while (g_gateway.running && !g_stop_requested) {
        uint32_t current_time = trm_get_tick_ms();
        
        /* 定期发送广播消息 */
        if (current_time - last_broadcast_time > 5000) { /* 每5秒 */
            uint8_t broadcast_data[64];
            snprintf((char*)broadcast_data, sizeof(broadcast_data), 
                    "Gateway Broadcast #%u from GW %d", 
                    broadcast_counter++, g_gateway.gateway_id);
            
            gateway_send_broadcast(broadcast_data, strlen((char*)broadcast_data) + 1);
            last_broadcast_time = current_time;
        }
        
        /* 处理TRM事件 */
        trm_process_events();
        
        /* 短暂延时 */
        trm_delay_ms(100);
        
        /* 检查运行时间 */
        if ((current_time - g_gateway.start_time) > (GATEWAY_DEMO_DURATION_SEC * 1000)) {
            printf("Demo duration reached, stopping...\n");
            break;
        }
    }
    
    /* 移除所有用户 */
    for (uint8_t i = 0; i < GATEWAY_MAX_USERS; i++) {
        if (g_gateway.users[i].is_active) {
            gateway_remove_user(i);
        }
    }
    
    printf("Gateway main loop ended\n");
}

/**
 * @brief 主函数
 */
int main(void)
{
    printf("=== TK8710 Gateway Demo ===\n");
    printf("Demo Duration: %d seconds\n", GATEWAY_DEMO_DURATION_SEC);
    printf("Max Users: %d\n", GATEWAY_MAX_USERS);
    printf("Frequency: %u Hz\n", GATEWAY_FREQUENCY_HZ);
    printf("Power: %d dBm\n", GATEWAY_POWER_DBM);
    printf("\n");
    
    /* 设置信号处理 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* 初始化网关 */
    int ret = gateway_init();
    if (ret != 0) {
        printf("Gateway initialization failed: %d\n", ret);
        return -1;
    }
    
    /* 运行网关主循环 */
    gateway_main_loop();
    
    /* 打印最终统计 */
    gateway_print_stats();
    
    /* 反初始化网关 */
    gateway_deinit();
    
    printf("Gateway demo completed\n");
    
    return 0;
}
