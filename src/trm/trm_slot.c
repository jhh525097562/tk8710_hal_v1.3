/**
 * @file trm_slot.c
 * @brief TRM时域资源管理实现
 * @note 负责时隙分配、调度和管理
 */

#include "trm.h"
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include "trm_api.h"
#include "trm_internal.h"
#include "driver/tk8710_log.h"

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

/*============================================================================
 * TRM时隙计算器 - 基于8710_HAL用户指南v1.0 7.2.4章节
 *============================================================================*/

/* 各模式基础时隙长度(us)，包块数=1时的值 */
static const uint32_t g_bcnSlotLen[] = {
    [5] = 69583,  [6] = 36340,  [7] = 19129,  [8] = 10732,
    [9] = 6510,   [10] = 6510,  [11] = 6510,  [18] = 6510
};

static const uint32_t g_brdBaseBody[] = {
    [5] = 131072, [6] = 65536, [7] = 32768,  [8] = 16384,
    [9] = 8192,  [10] = 4096, [11] = 2048,  [18] = 2048
};

static const uint32_t g_brdBaseGap[] = {
    // [5] = 13000,  [6] = 14000,  [7] = 7000,   [8] = 3600,
    // [9] = 1900,   [10] = 1200,  [11] = 800,   [18] = 800
    [5] = 21492,  [6] = 19728,  [7] = 12000,   [8] = 5600,
    [9] = 2800,   [10] = 1400,  [11] = 800,   [18] = 800
};

static const uint32_t g_ulBaseBody[] = {
    [5] = 131072, [6] = 65536, [7] = 32768,  [8] = 16384,
    [9] = 8192,  [10] = 4096, [11] = 2048,  [18] = 2048
};

static const uint32_t g_ulBaseGap[] = {
    [5] = 21492,  [6] = 19728,  [7] = 12000,   [8] = 5600,
    [9] = 2800,   [10] = 1400,  [11] = 800,   [18] = 800
};

static const uint32_t g_dlBaseBody[] = {
    [5] = 131072, [6] = 65536, [7] = 32768,  [8] = 16384,
    [9] = 8192,  [10] = 4096, [11] = 2048,  [18] = 2048
};

static const uint32_t g_dlBaseGap[] = {
    [5] = 21492,  [6] = 19728,  [7] = 12000,   [8] = 5600,
    [9] = 2800,   [10] = 1400,  [11] = 800,   [18] = 800
};

#define INTERVAL_US     1024
#define ONE_SECOND_US   1000000

/**
 * @brief 求最大公约数
 * @param a 第一个数
 * @param b 第二个数
 * @return 最大公约数
 */
static uint32_t trm_gcd(uint32_t a, uint32_t b)
{
    while (b != 0) {
        uint32_t t = b;
        b = a % b;
        a = t;
    }
    return a;
}

/**
 * @brief TRM时隙计算器
 * @param input 输入参数
 * @param output 输出结果
 * @return 0-成功, 非0-失败
 * @note 基于8710_HAL用户指南v1.0 7.2.4章节实现
 */
int trm_calc_slot_config(const TRM_SlotCalcInput* input, TRM_SlotCalcOutput* output)
{
    if (!input || !output) {
        TK8710_LOG_CORE_ERROR("Invalid parameters: input=%p, output=%p", input, output);
        return -1;
    }
    
    uint8_t mode = input->rateMode;
    if ((mode < 5 || mode > 11) && mode != 18) {
        TK8710_LOG_CORE_ERROR("Invalid rate mode: %d (supported: 5-11, 18)", mode);
        return -1;
    }
    
    if (input->ulBlockNum == 0 || input->dlBlockNum == 0) {
        TK8710_LOG_CORE_ERROR("Invalid block numbers: ul=%d, dl=%d (must be > 0)", 
                            input->ulBlockNum, input->dlBlockNum);
        return -1;
    }
    
    TK8710_LOG_CORE_INFO("Calculating slot config: mode=%d, ulBlocks=%d, dlBlocks=%d", 
                        mode, input->ulBlockNum, input->dlBlockNum);
    
    /* 计算各时隙长度 */
    output->bcnSlotLen = g_bcnSlotLen[mode];
    output->brdSlotLen = INTERVAL_US + g_brdBaseBody[mode] * (input->brdBlockNum * 2 + 1) + g_brdBaseGap[mode];
    output->ulSlotLen  = INTERVAL_US + g_ulBaseBody[mode] * (input->ulBlockNum * 2 + 1) + g_ulBaseGap[mode];
    output->dlSlotLen  = INTERVAL_US + g_dlBaseBody[mode] * (input->dlBlockNum * 2 + 1) + g_dlBaseGap[mode];
    
    /* 初始间隔 */
    output->bcnGap = 0;
    output->brdGap = g_brdBaseGap[mode];
    output->ulGap  = g_ulBaseGap[mode];
    output->dlGap  = g_dlBaseGap[mode];
    
    /* 计算原始帧周期 */
    uint32_t rawPeriod = output->bcnSlotLen + output->brdSlotLen + 
                         output->ulSlotLen + output->dlSlotLen;
    
    TK8710_LOG_CORE_INFO("Raw frame period: %u us (BCN:%u + BRD:%u + UL:%u + DL:%u)", 
                         rawPeriod, output->bcnSlotLen, output->brdSlotLen, 
                         output->ulSlotLen, output->dlSlotLen);
    
    /* 新算法：严格满足 framePeriod * frameCount = M秒 */
    uint32_t bestPeriod = 0;
    uint32_t bestCount = 0;
    uint32_t minGap = UINT32_MAX;
    
    // 遍历所有可能的M和N组合，寻找最小间隔
    for (uint32_t M = 1; M <= 30; M++) {
        uint32_t totalUs = M * ONE_SECOND_US;
        
        // 寻找所有能整除totalUs的N
        for (uint32_t N = 1; N <= 254; N++) {
            if (totalUs % N != 0) continue; // N必须能整除totalUs
            
            uint32_t candidatePeriod = totalUs / N;
            
            // 只考虑candidatePeriod >= rawPeriod的情况
            if (candidatePeriod >= rawPeriod) {
                uint32_t gap = candidatePeriod - rawPeriod;
                
                // 优先选择间隔最小的解
                if (gap < minGap) {
                    minGap = gap;
                    bestPeriod = candidatePeriod;
                    bestCount = N;
                    
                    TK8710_LOG_CORE_INFO("Found better solution: M=%ds, N=%u, gap=%u us, period=%u us", 
                                       M, N, gap, candidatePeriod);
                    
                    // 如果gap为0，这是最优解，直接退出
                    if (gap == 0) {
                        goto found_solution;
                    }
                }
            }
        }
    }
    
found_solution:
    if (bestPeriod == 0) {
        // 兜底：使用原始周期
        bestPeriod = rawPeriod;
        bestCount = 1;
        TK8710_LOG_CORE_INFO("No solution found, using raw period");
    }
    
    /* 间隔加到下行时隙 */
    minGap = 0;//终端暂时未使用时隙计算器时，先注释掉
    output->dlGap = output->dlGap + minGap;
    output->framePeriod = bestPeriod;
    output->frameCount = bestCount;
    output->dlSlotLen = output->dlSlotLen + minGap;
    TK8710_LOG_CORE_INFO("Slot calculation completed:");
    TK8710_LOG_CORE_INFO("  Frame period: %u us, Frame count: %u (total %u ms)", 
                        output->framePeriod, output->frameCount,
                        output->framePeriod * output->frameCount / 1000);
    TK8710_LOG_CORE_INFO("  Gaps - BRD:%u, UL:%u, DL:%u us", 
                        output->brdGap, output->ulGap, output->dlGap);
    TK8710_LOG_CORE_INFO("  Slot lengths - BCN:%u, BRD:%u, UL:%u, DL:%u us", 
                        output->bcnSlotLen, output->brdSlotLen, 
                        output->ulSlotLen, output->dlSlotLen);
    
    return 0;
}

/**
 * @brief 打印时隙计算结果
 * @param output 计算结果
 */
void trm_print_slot_calc_result(const TRM_SlotCalcOutput* output)
{
    if (!output) {
        TK8710_LOG_CORE_ERROR("Invalid output parameter");
        return;
    }
    
    printf("\n=== TRM Slot Calculation Result ===\n");
    printf("Frame Period: %u us\n", output->framePeriod);
    printf("Frame Count: %u\n", output->frameCount);
    printf("Total Duration: %u ms\n", output->framePeriod * output->frameCount / 1000);
    printf("\nSlot Lengths:\n");
    printf("  BCN Slot: %u us\n", output->bcnSlotLen);
    printf("  Broadcast Slot: %u us\n", output->brdSlotLen);
    printf("  Uplink Slot: %u us\n", output->ulSlotLen);
    printf("  Downlink Slot: %u us\n", output->dlSlotLen);
    printf("\nGaps:\n");
    printf("  Downlink Gap: %u us\n", output->dlGap);
    printf("=====================================\n\n");
}
