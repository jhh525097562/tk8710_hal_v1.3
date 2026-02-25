/**
 * @file trm_slot.c
 * @brief TRM时域资源管理实现
 * @note 负责时隙分配、调度和管理
 */

#include "trm.h"
#include "trm_config.h"
#include "tk8710_hal.h"
#include "../../inc/common/tk8710_log.h"
#include <string.h>
#include <stdlib.h>

/*============================================================================
 * 时域资源管理内部数据结构
 *============================================================================*/

/** 时隙状态枚举 */
typedef enum {
    SLOT_STATE_IDLE = 0,              /**< 空闲状态 */
    SLOT_STATE_RESERVED = 1,          /**< 预留状态 */
    SLOT_STATE_ACTIVE = 2,            /**< 活动状态 */
    SLOT_STATE_ERROR = 3              /**< 错误状态 */
} slot_state_t;

/** 时隙描述符 */
typedef struct {
    uint8_t slot_index;               /**< 时隙索引 */
    uint8_t slot_type;                /**< 时隙类型 */
    slot_state_t state;               /**< 时隙状态 */
    uint32_t start_time_us;           /**< 开始时间(微秒) */
    uint32_t duration_us;             /**< 持续时间(微秒) */
    uint8_t assigned_user;            /**< 分配用户 */
    uint16_t beam_id;                 /**< 波束ID */
    bool is_tx_slot;                  /**< 是否为发送时隙 */
    bool is_rx_slot;                  /**< 是否为接收时隙 */
} slot_descriptor_t;

/** 时域资源管理器 */
typedef struct {
    slot_descriptor_t slots[TRM_MAX_SLOTS]; /**< 时隙描述符数组 */
    uint8_t slot_count;               /**< 时隙总数 */
    uint8_t active_slots;             /**< 活动时隙数 */
    uint8_t current_slot;             /**< 当前时隙索引 */
    uint32_t frame_start_time_us;     /**< 帧开始时间 */
    uint32_t frame_duration_us;       /**< 帧持续时间 */
    bool is_initialized;              /**< 初始化标志 */
} trm_slot_manager_t;

/* 全局时域资源管理器实例 */
static trm_slot_manager_t g_slot_manager = {0};

/*============================================================================
 * 内部辅助函数
 *============================================================================*/

/**
 * @brief 获取时隙类型字符串
 * @param slot_type 时隙类型
 * @return 类型字符串
 */
static const char* trm_slot_get_type_string(uint8_t slot_type)
{
    switch (slot_type) {
        case 0: return "BCN";    /* 信标时隙 */
        case 1: return "DATA";   /* 数据时隙 */
        case 2: return "CTRL";   /* 控制时隙 */
        case 3: return "SYNC";   /* 同步时隙 */
        default: return "UNKNOWN";
    }
}

/**
 * @brief 验证时隙参数
 * @param slot_index 时隙索引
 * @param duration_us 持续时间
 * @return 0-有效, 非0-无效
 */
static int trm_slot_validate_params(uint8_t slot_index, uint32_t duration_us)
{
    if (slot_index >= TRM_MAX_SLOTS) {
        TK8710_LOG_CORE_ERROR("Invalid slot index: %d (max: %d)", slot_index, TRM_MAX_SLOTS - 1);
        return -1;
    }
    
    if (duration_us == 0 || duration_us > g_slot_manager.frame_duration_us) {
        TK8710_LOG_CORE_ERROR("Invalid slot duration: %u us", duration_us);
        return -2;
    }
    
    return 0;
}

/*============================================================================
 * 时域资源管理API实现
 *============================================================================*/

/**
 * @brief 初始化时域资源管理器
 * @param frame_duration_us 帧持续时间(微秒)
 * @return 0-成功, 非0-失败
 */
int trm_slot_init(uint32_t frame_duration_us)
{
    if (g_slot_manager.is_initialized) {
        TK8710_LOG_CORE_WARN("Slot manager already initialized");
        return 0;
    }
    
    /* 清零时域资源管理器 */
    memset(&g_slot_manager, 0, sizeof(trm_slot_manager_t));
    
    /* 设置帧参数 */
    g_slot_manager.frame_duration_us = frame_duration_us;
    g_slot_manager.frame_start_time_us = TK8710GetTickMs() * 1000; /* 转换为微秒 */
    
    /* 初始化所有时隙为空闲状态 */
    for (uint8_t i = 0; i < TRM_MAX_SLOTS; i++) {
        g_slot_manager.slots[i].slot_index = i;
        g_slot_manager.slots[i].state = SLOT_STATE_IDLE;
        g_slot_manager.slots[i].assigned_user = 0xFF; /* 未分配 */
        g_slot_manager.slots[i].beam_id = 0xFFFF; /* 无效波束ID */
    }
    
    g_slot_manager.is_initialized = true;
    
    TK8710_LOG_CORE_INFO("Slot manager initialized: frame_duration=%u us, max_slots=%d", 
                    frame_duration_us, TRM_MAX_SLOTS);
    
    return 0;
}

/**
 * @brief 分配时隙
 * @param slot_type 时隙类型
 * @param duration_us 持续时间(微秒)
 * @param user_id 用户ID
 * @param beam_id 波束ID
 * @return 时隙索引，失败返回0xFF
 */
uint8_t trm_slot_allocate(uint8_t slot_type, uint32_t duration_us, 
                          uint8_t user_id, uint16_t beam_id)
{
    if (!g_slot_manager.is_initialized) {
        TK8710_LOG_CORE_ERROR("Slot manager not initialized");
        return 0xFF;
    }
    
    /* 验证参数 */
    if (trm_slot_validate_params(0, duration_us) != 0) {
        return 0xFF;
    }
    
    /* 查找空闲时隙 */
    for (uint8_t i = 0; i < TRM_MAX_SLOTS; i++) {
        if (g_slot_manager.slots[i].state == SLOT_STATE_IDLE) {
            /* 分配时隙 */
            g_slot_manager.slots[i].slot_type = slot_type;
            g_slot_manager.slots[i].duration_us = duration_us;
            g_slot_manager.slots[i].assigned_user = user_id;
            g_slot_manager.slots[i].beam_id = beam_id;
            g_slot_manager.slots[i].state = SLOT_STATE_RESERVED;
            
            /* 根据类型设置发送/接收标志 */
            g_slot_manager.slots[i].is_tx_slot = (slot_type == 0 || slot_type == 2); /* BCN或CTRL */
            g_slot_manager.slots[i].is_rx_slot = (slot_type == 1); /* DATA */
            
            /* 计算开始时间 */
            uint32_t total_duration = 0;
            for (uint8_t j = 0; j < i; j++) {
                if (g_slot_manager.slots[j].state != SLOT_STATE_IDLE) {
                    total_duration += g_slot_manager.slots[j].duration_us;
                }
            }
            g_slot_manager.slots[i].start_time_us = total_duration;
            
            g_slot_manager.active_slots++;
            
            TK8710_LOG_CORE_INFO("Slot allocated: index=%d, type=%s, duration=%u us, user=%d, beam=%d",
                            i, trm_slot_get_type_string(slot_type), duration_us, user_id, beam_id);
            
            return i;
        }
    }
    
    TK8710_LOG_CORE_ERROR("No available slots for allocation");
    return 0xFF;
}

/**
 * @brief 释放时隙
 * @param slot_index 时隙索引
 * @return 0-成功, 非0-失败
 */
int trm_slot_release(uint8_t slot_index)
{
    if (!g_slot_manager.is_initialized) {
        TK8710_LOG_CORE_ERROR("Slot manager not initialized");
        return -1;
    }
    
    if (slot_index >= TRM_MAX_SLOTS) {
        TK8710_LOG_CORE_ERROR("Invalid slot index: %d", slot_index);
        return -2;
    }
    
    slot_descriptor_t* slot = &g_slot_manager.slots[slot_index];
    
    if (slot->state == SLOT_STATE_IDLE) {
        TK8710_LOG_CORE_WARN("Slot %d already idle", slot_index);
        return 0;
    }
    
    TK8710_LOG_CORE_INFO("Slot released: index=%d, type=%s, user=%d",
                    slot_index, trm_slot_get_type_string(slot->state), slot->assigned_user);
    
    /* 重置时隙状态 */
    slot->state = SLOT_STATE_IDLE;
    slot->assigned_user = 0xFF;
    slot->beam_id = 0xFFFF;
    slot->is_tx_slot = false;
    slot->is_rx_slot = false;
    
    g_slot_manager.active_slots--;
    
    return 0;
}

/**
 * @brief 获取时隙信息
 * @param slot_index 时隙索引
 * @param info 输出时隙信息
 * @return 0-成功, 非0-失败
 */
int trm_slot_get_info(uint8_t slot_index, trm_slot_info_t* info)
{
    if (!g_slot_manager.is_initialized || !info) {
        return -1;
    }
    
    if (slot_index >= TRM_MAX_SLOTS) {
        return -2;
    }
    
    slot_descriptor_t* slot = &g_slot_manager.slots[slot_index];
    
    if (slot->state == SLOT_STATE_IDLE) {
        return -3;
    }
    
    /* 填充时隙信息 */
    info->slot_index = slot->slot_index;
    info->slot_type = slot->slot_type;
    info->start_time_us = slot->start_time_us;
    info->duration_us = slot->duration_us;
    info->assigned_user = slot->assigned_user;
    info->beam_id = slot->beam_id;
    info->is_tx_slot = slot->is_tx_slot;
    info->is_rx_slot = slot->is_rx_slot;
    
    return 0;
}

/**
 * @brief 获取活动时隙数量
 * @return 活动时隙数量
 */
uint8_t trm_slot_get_active_count(void)
{
    return g_slot_manager.active_slots;
}

/**
 * @brief 重置时域资源管理器
 */
void trm_slot_reset(void)
{
    if (!g_slot_manager.is_initialized) {
        return;
    }
    
    /* 释放所有活动时隙 */
    for (uint8_t i = 0; i < TRM_MAX_SLOTS; i++) {
        if (g_slot_manager.slots[i].state != SLOT_STATE_IDLE) {
            trm_slot_release(i);
        }
    }
    
    /* 重置管理器状态 */
    g_slot_manager.current_slot = 0;
    g_slot_manager.frame_start_time_us = TK8710GetTickMs() * 1000;
    
    TK8710_LOG_CORE_INFO("Slot manager reset");
}

/**
 * @brief 打印时域资源管理状态
 */
void trm_slot_print_status(void)
{
    if (!g_slot_manager.is_initialized) {
        TK8710_LOG_CORE_INFO("Slot manager not initialized");
        return;
    }
    
    TK8710_LOG_CORE_INFO("=== Slot Manager Status ===");
    TK8710_LOG_CORE_INFO("Frame duration: %u us", g_slot_manager.frame_duration_us);
    TK8710_LOG_CORE_INFO("Active slots: %d/%d", g_slot_manager.active_slots, TRM_MAX_SLOTS);
    TK8710_LOG_CORE_INFO("Current slot: %d", g_slot_manager.current_slot);
    
    TK8710_LOG_CORE_INFO("Active slots list:");
    for (uint8_t i = 0; i < TRM_MAX_SLOTS; i++) {
        slot_descriptor_t* slot = &g_slot_manager.slots[i];
        if (slot->state != SLOT_STATE_IDLE) {
            TK8710_LOG_CORE_INFO("  [%d] %s: user=%d, beam=%d, duration=%u us, %s%s",
                            i, trm_slot_get_type_string(slot->slot_type),
                            slot->assigned_user, slot->beam_id, slot->duration_us,
                            slot->is_tx_slot ? "TX" : "",
                            slot->is_rx_slot ? "RX" : "");
        }
    }
    TK8710_LOG_CORE_INFO("============================");
}
