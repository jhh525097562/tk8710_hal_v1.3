/**
 * @file trm_freq.c
 * @brief TRM频域资源管理实现
 * @note 负责频率分配、调度和管理（接口预留）
 */

#include "trm.h"
#include "trm_config.h"
#include "tk8710_hal.h"
#include "../../inc/common/tk8710_log.h"
#include <string.h>
#include <stdlib.h>

/*============================================================================
 * 频域资源管理内部数据结构
 *============================================================================*/

/** 频率状态枚举 */
typedef enum {
    FREQ_STATE_IDLE = 0,             /**< 空闲状态 */
    FREQ_STATE_ALLOCATED = 1,         /**< 已分配状态 */
    FREQ_STATE_RESERVED = 2,          /**< 预留状态 */
    FREQ_STATE_ERROR = 3              /**< 错误状态 */
} freq_state_t;

/** 频率描述符 */
typedef struct {
    uint32_t frequency_hz;            /**< 频率(Hz) */
    uint8_t freq_index;               /**< 频率索引 */
    freq_state_t state;               /**< 频率状态 */
    uint8_t assigned_user;            /**< 分配用户 */
    uint16_t bandwidth_khz;           /**< 带宽(kHz) */
    uint8_t power_level;              /**< 功率等级 */
    bool is_tx_freq;                  /**< 是否为发送频率 */
    bool is_rx_freq;                  /**< 是否为接收频率 */
} freq_descriptor_t;

/** 频域资源管理器 */
typedef struct {
    freq_descriptor_t frequencies[TRM_MAX_FREQUENCIES]; /**< 频率描述符数组 */
    uint8_t freq_count;               /**< 频率总数 */
    uint8_t active_freqs;              /**< 活动频率数 */
    uint32_t base_frequency_hz;        /**< 基准频率(Hz) */
    uint32_t frequency_step_hz;        /**< 频率步进(Hz) */
    bool is_initialized;              /**< 初始化标志 */
} trm_freq_manager_t;

/* 全局频域资源管理器实例 */
static trm_freq_manager_t g_freq_manager = {0};

/*============================================================================
 * 内部辅助函数
 *============================================================================*/

/**
 * @brief 验证频率参数
 * @param frequency_hz 频率(Hz)
 * @param bandwidth_khz 带宽(kHz)
 * @return 0-有效, 非0-无效
 */
static int trm_freq_validate_params(uint32_t frequency_hz, uint16_t bandwidth_khz)
{
    /* 检查频率范围 */
    if (frequency_hz < 100000000 || frequency_hz > 6000000000) { /* 100MHz - 6GHz */
        TK8710_LOG_CORE_ERROR("Invalid frequency: %u Hz (range: 100MHz - 6GHz)", frequency_hz);
        return -1;
    }
    
    /* 检查带宽 */
    if (bandwidth_khz == 0 || bandwidth_khz > 10000) { /* 最大10MHz */
        TK8710_LOG_CORE_ERROR("Invalid bandwidth: %u kHz (max: 10000 kHz)", bandwidth_khz);
        return -2;
    }
    
    return 0;
}

/**
 * @brief 查找频率索引
 * @param frequency_hz 频率(Hz)
 * @return 频率索引，未找到返回0xFF
 */
static uint8_t trm_freq_find_index(uint32_t frequency_hz)
{
    for (uint8_t i = 0; i < TRM_MAX_FREQUENCIES; i++) {
        if (g_freq_manager.frequencies[i].frequency_hz == frequency_hz &&
            g_freq_manager.frequencies[i].state != FREQ_STATE_IDLE) {
            return i;
        }
    }
    return 0xFF;
}

/*============================================================================
 * 频域资源管理API实现
 *============================================================================*/

/**
 * @brief 初始化频域资源管理器
 * @param base_frequency_hz 基准频率(Hz)
 * @param frequency_step_hz 频率步进(Hz)
 * @return 0-成功, 非0-失败
 */
int trm_freq_init(uint32_t base_frequency_hz, uint32_t frequency_step_hz)
{
    if (g_freq_manager.is_initialized) {
        TK8710_LOG_CORE_WARN("Frequency manager already initialized");
        return 0;
    }
    
    /* 验证参数 */
    if (trm_freq_validate_params(base_frequency_hz, 1000) != 0) {
        return -1;
    }
    
    /* 清零频域资源管理器 */
    memset(&g_freq_manager, 0, sizeof(trm_freq_manager_t));
    
    /* 设置频率参数 */
    g_freq_manager.base_frequency_hz = base_frequency_hz;
    g_freq_manager.frequency_step_hz = frequency_step_hz;
    
    /* 初始化所有频率为空闲状态 */
    for (uint8_t i = 0; i < TRM_MAX_FREQUENCIES; i++) {
        g_freq_manager.frequencies[i].freq_index = i;
        g_freq_manager.frequencies[i].state = FREQ_STATE_IDLE;
        g_freq_manager.frequencies[i].assigned_user = 0xFF; /* 未分配 */
        g_freq_manager.frequencies[i].frequency_hz = base_frequency_hz + i * frequency_step_hz;
    }
    
    g_freq_manager.is_initialized = true;
    
    TK8710_LOG_CORE_INFO("Frequency manager initialized: base_freq=%u Hz, step=%u Hz, max_freqs=%d",
                    base_frequency_hz, frequency_step_hz, TRM_MAX_FREQUENCIES);
    
    return 0;
}

/**
 * @brief 分配频率
 * @param frequency_hz 频率(Hz)
 * @param bandwidth_khz 带宽(kHz)
 * @param user_id 用户ID
 * @param power_level 功率等级
 * @return 频率索引，失败返回0xFF
 */
uint8_t trm_freq_allocate(uint32_t frequency_hz, uint16_t bandwidth_khz,
                          uint8_t user_id, uint8_t power_level)
{
    if (!g_freq_manager.is_initialized) {
        TK8710_LOG_CORE_ERROR("Frequency manager not initialized");
        return 0xFF;
    }
    
    /* 验证参数 */
    if (trm_freq_validate_params(frequency_hz, bandwidth_khz) != 0) {
        return 0xFF;
    }
    
    /* 检查频率是否已存在 */
    uint8_t existing_index = trm_freq_find_index(frequency_hz);
    if (existing_index != 0xFF) {
        TK8710_LOG_CORE_WARN("Frequency %u Hz already allocated (index: %d)", frequency_hz, existing_index);
        return existing_index;
    }
    
    /* 查找空闲频率槽位 */
    for (uint8_t i = 0; i < TRM_MAX_FREQUENCIES; i++) {
        if (g_freq_manager.frequencies[i].state == FREQ_STATE_IDLE) {
            /* 分配频率 */
            g_freq_manager.frequencies[i].frequency_hz = frequency_hz;
            g_freq_manager.frequencies[i].bandwidth_khz = bandwidth_khz;
            g_freq_manager.frequencies[i].assigned_user = user_id;
            g_freq_manager.frequencies[i].power_level = power_level;
            g_freq_manager.frequencies[i].state = FREQ_STATE_ALLOCATED;
            
            /* 默认设置为双向频率 */
            g_freq_manager.frequencies[i].is_tx_freq = true;
            g_freq_manager.frequencies[i].is_rx_freq = true;
            
            g_freq_manager.active_freqs++;
            
            TK8710_LOG_CORE_INFO("Frequency allocated: index=%d, freq=%u Hz, bw=%u kHz, user=%d, power=%d",
                            i, frequency_hz, bandwidth_khz, user_id, power_level);
            
            return i;
        }
    }
    
    TK8710_LOG_CORE_ERROR("No available frequency slots for allocation");
    return 0xFF;
}

/**
 * @brief 释放频率
 * @param freq_index 频率索引
 * @return 0-成功, 非0-失败
 */
int trm_freq_release(uint8_t freq_index)
{
    if (!g_freq_manager.is_initialized) {
        TK8710_LOG_CORE_ERROR("Frequency manager not initialized");
        return -1;
    }
    
    if (freq_index >= TRM_MAX_FREQUENCIES) {
        TK8710_LOG_CORE_ERROR("Invalid frequency index: %d", freq_index);
        return -2;
    }
    
    freq_descriptor_t* freq = &g_freq_manager.frequencies[freq_index];
    
    if (freq->state == FREQ_STATE_IDLE) {
        TK8710_LOG_CORE_WARN("Frequency %d already idle", freq_index);
        return 0;
    }
    
    TK8710_LOG_CORE_INFO("Frequency released: index=%d, freq=%u Hz, user=%d",
                    freq_index, freq->frequency_hz, freq->assigned_user);
    
    /* 重置频率状态 */
    freq->state = FREQ_STATE_IDLE;
    freq->assigned_user = 0xFF;
    freq->power_level = 0;
    freq->is_tx_freq = false;
    freq->is_rx_freq = false;
    
    g_freq_manager.active_freqs--;
    
    return 0;
}

/**
 * @brief 获取频率信息
 * @param freq_index 频率索引
 * @param info 输出频率信息
 * @return 0-成功, 非0-失败
 */
int trm_freq_get_info(uint8_t freq_index, trm_freq_info_t* info)
{
    if (!g_freq_manager.is_initialized || !info) {
        return -1;
    }
    
    if (freq_index >= TRM_MAX_FREQUENCIES) {
        return -2;
    }
    
    freq_descriptor_t* freq = &g_freq_manager.frequencies[freq_index];
    
    if (freq->state == FREQ_STATE_IDLE) {
        return -3;
    }
    
    /* 填充频率信息 */
    info->freq_index = freq->freq_index;
    info->frequency_hz = freq->frequency_hz;
    info->bandwidth_khz = freq->bandwidth_khz;
    info->assigned_user = freq->assigned_user;
    info->power_level = freq->power_level;
    info->is_tx_freq = freq->is_tx_freq;
    info->is_rx_freq = freq->is_rx_freq;
    
    return 0;
}

/**
 * @brief 设置频率方向
 * @param freq_index 频率索引
 * @param is_tx_freq 是否为发送频率
 * @param is_rx_freq 是否为接收频率
 * @return 0-成功, 非0-失败
 */
int trm_freq_set_direction(uint8_t freq_index, bool is_tx_freq, bool is_rx_freq)
{
    if (!g_freq_manager.is_initialized) {
        return -1;
    }
    
    if (freq_index >= TRM_MAX_FREQUENCIES) {
        return -2;
    }
    
    freq_descriptor_t* freq = &g_freq_manager.frequencies[freq_index];
    
    if (freq->state == FREQ_STATE_IDLE) {
        TK8710_LOG_CORE_ERROR("Cannot set direction for idle frequency %d", freq_index);
        return -3;
    }
    
    freq->is_tx_freq = is_tx_freq;
    freq->is_rx_freq = is_rx_freq;
    
    TK8710_LOG_CORE_INFO("Frequency direction set: index=%d, tx=%s, rx=%s",
                    freq_index, is_tx_freq ? "yes" : "no", is_rx_freq ? "yes" : "no");
    
    return 0;
}

/**
 * @brief 获取活动频率数量
 * @return 活动频率数量
 */
uint8_t trm_freq_get_active_count(void)
{
    return g_freq_manager.active_freqs;
}

/**
 * @brief 重置频域资源管理器
 */
void trm_freq_reset(void)
{
    if (!g_freq_manager.is_initialized) {
        return;
    }
    
    /* 释放所有活动频率 */
    for (uint8_t i = 0; i < TRM_MAX_FREQUENCIES; i++) {
        if (g_freq_manager.frequencies[i].state != FREQ_STATE_IDLE) {
            trm_freq_release(i);
        }
    }
    
    TK8710_LOG_CORE_INFO("Frequency manager reset");
}

/**
 * @brief 打印频域资源管理状态
 */
void trm_freq_print_status(void)
{
    if (!g_freq_manager.is_initialized) {
        TK8710_LOG_CORE_INFO("Frequency manager not initialized");
        return;
    }
    
    TK8710_LOG_CORE_INFO("=== Frequency Manager Status ===");
    TK8710_LOG_CORE_INFO("Base frequency: %u Hz", g_freq_manager.base_frequency_hz);
    TK8710_LOG_CORE_INFO("Frequency step: %u Hz", g_freq_manager.frequency_step_hz);
    TK8710_LOG_CORE_INFO("Active frequencies: %d/%d", g_freq_manager.active_freqs, TRM_MAX_FREQUENCIES);
    
    TK8710_LOG_CORE_INFO("Active frequencies list:");
    for (uint8_t i = 0; i < TRM_MAX_FREQUENCIES; i++) {
        freq_descriptor_t* freq = &g_freq_manager.frequencies[i];
        if (freq->state != FREQ_STATE_IDLE) {
            TK8710_LOG_CORE_INFO("  [%d] %u Hz: user=%d, bw=%u kHz, power=%d, %s%s",
                            i, freq->frequency_hz, freq->assigned_user, freq->bandwidth_khz,
                            freq->power_level, freq->is_tx_freq ? "TX" : "",
                            freq->is_rx_freq ? "RX" : "");
        }
    }
    TK8710_LOG_CORE_INFO("==================================");
}
