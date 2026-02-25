/**
 * @file trm_power.c
 * @brief TRM功率资源管理实现
 * @note 负责功率分配、调度和管理（接口预留）
 */

#include "trm.h"
#include "trm_config.h"
#include "tk8710_hal.h"
#include "../../inc/driver/tk8710_log.h"
#include <string.h>
#include <stdlib.h>

/*============================================================================
 * 功率资源管理内部数据结构
 *============================================================================*/

/** 功率状态枚举 */
typedef enum {
    POWER_STATE_IDLE = 0,             /**< 空闲状态 */
    POWER_STATE_ALLOCATED = 1,        /**< 已分配状态 */
    POWER_STATE_RESERVED = 2,         /**< 预留状态 */
    POWER_STATE_ERROR = 3             /**< 错误状态 */
} power_state_t;

/** 功率描述符 */
typedef struct {
    uint8_t power_index;              /**< 功率索引 */
    uint16_t power_level_dBm;         /**< 功率等级(dBm) */
    power_state_t state;              /**< 功率状态 */
    uint8_t assigned_user;            /**< 分配用户 */
    uint8_t antenna_id;               /**< 天线ID */
    uint16_t frequency_hz;            /**< 频率(Hz) */
    bool is_tx_power;                 /**< 是否为发送功率 */
    bool is_rx_power;                 /**< 是否为接收功率 */
    uint32_t last_update_time;        /**< 最后更新时间 */
} power_descriptor_t;

/** 功率资源管理器 */
typedef struct {
    power_descriptor_t powers[TRM_MAX_POWER_LEVELS]; /**< 功率描述符数组 */
    uint8_t power_count;              /**< 功率总数 */
    uint8_t active_powers;            /**< 活动功率数 */
    uint16_t min_power_dBm;           /**< 最小功率(dBm) */
    uint16_t max_power_dBm;           /**< 最大功率(dBm) */
    uint16_t power_step_dBm;          /**< 功率步进(dBm) */
    bool is_initialized;              /**< 初始化标志 */
} trm_power_manager_t;

/* 全局功率资源管理器实例 */
static trm_power_manager_t g_power_manager = {0};

/*============================================================================
 * 内部辅助函数
 *============================================================================*/

/**
 * @brief 验证功率参数
 * @param power_level_dBm 功率等级(dBm)
 * @param antenna_id 天线ID
 * @return 0-有效, 非0-无效
 */
static int trm_power_validate_params(int16_t power_level_dBm, uint8_t antenna_id)
{
    /* 检查功率范围 */
    if (power_level_dBm < -120 || power_level_dBm > 30) { /* -120dBm - +30dBm */
        TK8710_LOG_CORE_ERROR("Invalid power level: %d dBm (range: -120 - +30 dBm)", power_level_dBm);
        return -1;
    }
    
    /* 检查天线ID */
    if (antenna_id >= 8) { /* 最大8个天线 */
        TK8710_LOG_CORE_ERROR("Invalid antenna ID: %d (max: 7)", antenna_id);
        return -2;
    }
    
    return 0;
}

/**
 * @brief 查找功率索引
 * @param power_level_dBm 功率等级(dBm)
 * @param antenna_id 天线ID
 * @return 功率索引，未找到返回0xFF
 */
static uint8_t trm_power_find_index(int16_t power_level_dBm, uint8_t antenna_id)
{
    for (uint8_t i = 0; i < TRM_MAX_POWER_LEVELS; i++) {
        if (g_power_manager.powers[i].power_level_dBm == power_level_dBm &&
            g_power_manager.powers[i].antenna_id == antenna_id &&
            g_power_manager.powers[i].state != POWER_STATE_IDLE) {
            return i;
        }
    }
    return 0xFF;
}

/**
 * @brief 计算功率控制字
 * @param power_level_dBm 功率等级(dBm)
 * @return 功率控制字
 */
static uint16_t trm_power_calculate_control_word(int16_t power_level_dBm)
{
    /* 简化的功率控制字计算 */
    /* 实际实现需要根据硬件规格进行精确计算 */
    if (power_level_dBm <= 0) {
        return (uint16_t)(0x8000 + power_level_dBm * 10); /* 负功率 */
    } else {
        return (uint16_t)(power_level_dBm * 10); /* 正功率 */
    }
}

/*============================================================================
 * 功率资源管理API实现
 *============================================================================*/

/**
 * @brief 初始化功率资源管理器
 * @param min_power_dBm 最小功率(dBm)
 * @param max_power_dBm 最大功率(dBm)
 * @param power_step_dBm 功率步进(dBm)
 * @return 0-成功, 非0-失败
 */
int trm_power_init(int16_t min_power_dBm, int16_t max_power_dBm, uint16_t power_step_dBm)
{
    if (g_power_manager.is_initialized) {
        TK8710_LOG_CORE_WARN("Power manager already initialized");
        return 0;
    }
    
    /* 验证参数 */
    if (trm_power_validate_params(min_power_dBm, 0) != 0 ||
        trm_power_validate_params(max_power_dBm, 0) != 0) {
        return -1;
    }
    
    if (min_power_dBm >= max_power_dBm) {
        TK8710_LOG_CORE_ERROR("Invalid power range: min=%d >= max=%d", min_power_dBm, max_power_dBm);
        return -2;
    }
    
    /* 清零功率资源管理器 */
    memset(&g_power_manager, 0, sizeof(trm_power_manager_t));
    
    /* 设置功率参数 */
    g_power_manager.min_power_dBm = min_power_dBm;
    g_power_manager.max_power_dBm = max_power_dBm;
    g_power_manager.power_step_dBm = power_step_dBm;
    
    /* 初始化所有功率为空闲状态 */
    for (uint8_t i = 0; i < TRM_MAX_POWER_LEVELS; i++) {
        g_power_manager.powers[i].power_index = i;
        g_power_manager.powers[i].state = POWER_STATE_IDLE;
        g_power_manager.powers[i].assigned_user = 0xFF; /* 未分配 */
        g_power_manager.powers[i].power_level_dBm = min_power_dBm + i * power_step_dBm;
    }
    
    g_power_manager.is_initialized = true;
    
    TK8710_LOG_CORE_INFO("Power manager initialized: range=%d to %d dBm, step=%d dBm, max_levels=%d",
                    min_power_dBm, max_power_dBm, power_step_dBm, TRM_MAX_POWER_LEVELS);
    
    return 0;
}

/**
 * @brief 分配功率
 * @param power_level_dBm 功率等级(dBm)
 * @param antenna_id 天线ID
 * @param user_id 用户ID
 * @param frequency_hz 频率(Hz)
 * @return 功率索引，失败返回0xFF
 */
uint8_t trm_power_allocate(int16_t power_level_dBm, uint8_t antenna_id,
                            uint8_t user_id, uint32_t frequency_hz)
{
    if (!g_power_manager.is_initialized) {
        TK8710_LOG_CORE_ERROR("Power manager not initialized");
        return 0xFF;
    }
    
    /* 验证参数 */
    if (trm_power_validate_params(power_level_dBm, antenna_id) != 0) {
        return 0xFF;
    }
    
    /* 检查功率是否已存在 */
    uint8_t existing_index = trm_power_find_index(power_level_dBm, antenna_id);
    if (existing_index != 0xFF) {
        TK8710_LOG_CORE_WARN("Power %d dBm (antenna %d) already allocated (index: %d)",
                        power_level_dBm, antenna_id, existing_index);
        return existing_index;
    }
    
    /* 查找空闲功率槽位 */
    for (uint8_t i = 0; i < TRM_MAX_POWER_LEVELS; i++) {
        if (g_power_manager.powers[i].state == POWER_STATE_IDLE) {
            /* 分配功率 */
            g_power_manager.powers[i].power_level_dBm = power_level_dBm;
            g_power_manager.powers[i].antenna_id = antenna_id;
            g_power_manager.powers[i].assigned_user = user_id;
            g_power_manager.powers[i].frequency_hz = frequency_hz;
            g_power_manager.powers[i].state = POWER_STATE_ALLOCATED;
            g_power_manager.powers[i].last_update_time = TK8710GetTickMs();
            
            /* 默认设置为发送功率 */
            g_power_manager.powers[i].is_tx_power = true;
            g_power_manager.powers[i].is_rx_power = false;
            
            g_power_manager.active_powers++;
            
            TK8710_LOG_CORE_INFO("Power allocated: index=%d, level=%d dBm, antenna=%d, user=%d, freq=%u Hz",
                            i, power_level_dBm, antenna_id, user_id, frequency_hz);
            
            return i;
        }
    }
    
    TK8710_LOG_CORE_ERROR("No available power slots for allocation");
    return 0xFF;
}

/**
 * @brief 释放功率
 * @param power_index 功率索引
 * @return 0-成功, 非0-失败
 */
int trm_power_release(uint8_t power_index)
{
    if (!g_power_manager.is_initialized) {
        TK8710_LOG_CORE_ERROR("Power manager not initialized");
        return -1;
    }
    
    if (power_index >= TRM_MAX_POWER_LEVELS) {
        TK8710_LOG_CORE_ERROR("Invalid power index: %d", power_index);
        return -2;
    }
    
    power_descriptor_t* power = &g_power_manager.powers[power_index];
    
    if (power->state == POWER_STATE_IDLE) {
        TK8710_LOG_CORE_WARN("Power %d already idle", power_index);
        return 0;
    }
    
    TK8710_LOG_CORE_INFO("Power released: index=%d, level=%d dBm, antenna=%d, user=%d",
                    power_index, power->power_level_dBm, power->antenna_id, power->assigned_user);
    
    /* 重置功率状态 */
    power->state = POWER_STATE_IDLE;
    power->assigned_user = 0xFF;
    power->antenna_id = 0;
    power->frequency_hz = 0;
    power->is_tx_power = false;
    power->is_rx_power = false;
    
    g_power_manager.active_powers--;
    
    return 0;
}

/**
 * @brief 获取功率信息
 * @param power_index 功率索引
 * @param info 输出功率信息
 * @return 0-成功, 非0-失败
 */
int trm_power_get_info(uint8_t power_index, trm_power_info_t* info)
{
    if (!g_power_manager.is_initialized || !info) {
        return -1;
    }
    
    if (power_index >= TRM_MAX_POWER_LEVELS) {
        return -2;
    }
    
    power_descriptor_t* power = &g_power_manager.powers[power_index];
    
    if (power->state == POWER_STATE_IDLE) {
        return -3;
    }
    
    /* 填充功率信息 */
    info->power_index = power->power_index;
    info->power_level_dBm = power->power_level_dBm;
    info->antenna_id = power->antenna_id;
    info->assigned_user = power->assigned_user;
    info->frequency_hz = power->frequency_hz;
    info->is_tx_power = power->is_tx_power;
    info->is_rx_power = power->is_rx_power;
    info->control_word = trm_power_calculate_control_word(power->power_level_dBm);
    
    return 0;
}

/**
 * @brief 设置功率等级
 * @param power_index 功率索引
 * @param new_power_level_dBm 新功率等级(dBm)
 * @return 0-成功, 非0-失败
 */
int trm_power_set_level(uint8_t power_index, int16_t new_power_level_dBm)
{
    if (!g_power_manager.is_initialized) {
        return -1;
    }
    
    if (power_index >= TRM_MAX_POWER_LEVELS) {
        return -2;
    }
    
    power_descriptor_t* power = &g_power_manager.powers[power_index];
    
    if (power->state == POWER_STATE_IDLE) {
        TK8710_LOG_CORE_ERROR("Cannot set level for idle power %d", power_index);
        return -3;
    }
    
    /* 验证新功率等级 */
    if (trm_power_validate_params(new_power_level_dBm, power->antenna_id) != 0) {
        return -4;
    }
    
    int16_t old_level = power->power_level_dBm;
    power->power_level_dBm = new_power_level_dBm;
    power->last_update_time = TK8710GetTickMs();
    
    TK8710_LOG_CORE_INFO("Power level updated: index=%d, %d -> %d dBm, antenna=%d",
                    power_index, old_level, new_power_level_dBm, power->antenna_id);
    
    return 0;
}

/**
 * @brief 获取活动功率数量
 * @return 活动功率数量
 */
uint8_t trm_power_get_active_count(void)
{
    return g_power_manager.active_powers;
}

/**
 * @brief 重置功率资源管理器
 */
void trm_power_reset(void)
{
    if (!g_power_manager.is_initialized) {
        return;
    }
    
    /* 释放所有活动功率 */
    for (uint8_t i = 0; i < TRM_MAX_POWER_LEVELS; i++) {
        if (g_power_manager.powers[i].state != POWER_STATE_IDLE) {
            trm_power_release(i);
        }
    }
    
    TK8710_LOG_CORE_INFO("Power manager reset");
}

/**
 * @brief 打印功率资源管理状态
 */
void trm_power_print_status(void)
{
    if (!g_power_manager.is_initialized) {
        TK8710_LOG_CORE_INFO("Power manager not initialized");
        return;
    }
    
    TK8710_LOG_CORE_INFO("=== Power Manager Status ===");
    TK8710_LOG_CORE_INFO("Power range: %d to %d dBm", g_power_manager.min_power_dBm, g_power_manager.max_power_dBm);
    TK8710_LOG_CORE_INFO("Power step: %d dBm", g_power_manager.power_step_dBm);
    TK8710_LOG_CORE_INFO("Active powers: %d/%d", g_power_manager.active_powers, TRM_MAX_POWER_LEVELS);
    
    TK8710_LOG_CORE_INFO("Active powers list:");
    for (uint8_t i = 0; i < TRM_MAX_POWER_LEVELS; i++) {
        power_descriptor_t* power = &g_power_manager.powers[i];
        if (power->state != POWER_STATE_IDLE) {
            TK8710_LOG_CORE_INFO("  [%d] %d dBm: user=%d, antenna=%d, freq=%u Hz, %s%s",
                            i, power->power_level_dBm, power->assigned_user, power->antenna_id,
                            power->frequency_hz, power->is_tx_power ? "TX" : "",
                            power->is_rx_power ? "RX" : "");
        }
    }
    TK8710_LOG_CORE_INFO("============================");
}
