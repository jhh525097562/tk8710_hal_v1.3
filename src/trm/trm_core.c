/**
 * @file trm_core.c
 * @brief TRM核心功能实现
 */

#include "../inc/trm/trm.h"
#include "../inc/trm/trm_log.h"
#include "../inc/trm/trm_beam.h"
#include "../inc/trm/trm_data.h"
#include "../inc/driver/tk8710.h"
#include "../port/tk8710_hal.h"
#include "driver/tk8710_log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* 外部函数声明 - 来自trm_beam.c和trm_data.c */
extern void TRM_BeamInit(uint32_t maxUsers, uint32_t timeoutMs);
extern void TRM_BeamDeinit(void);
extern void TRM_DataInit(void);
extern void TRM_DataDeinit(void);
extern int TRM_ProcessTxSlot(uint8_t slotIndex, uint8_t maxUserCount, TK8710IrqResult* irqResult);

/*==============================================================================
 * 私有定义
 *============================================================================*/

/* TRM上下文 */
static TrmContext g_trmCtx;

/* 全局帧号管理变量 */
uint32_t g_trmCurrentFrame = 0;
uint32_t g_trmMaxFrameCount = 100;

/* 内部函数声明 */
TrmContext* TRM_GetContext(void);

/*==============================================================================
 * Driver回调函数声明
 *============================================================================*/

static void TRM_OnDriverSlotEnd(uint8_t slotType, uint8_t slotIndex, uint32_t frameNo);
static void TRM_OnDriverTxSlot(uint8_t slotIndex, uint8_t maxUserCount, TK8710IrqResult* irqResult);
static void TRM_OnDriverSlotRx(TK8710IrqResult* irqResult);
static void TRM_OnDriverError(TK8710IrqResult* irqResult);

/* 多回调适配函数 */
static void TRM_OnDriverSlotEndAdapter(TK8710IrqResult* irqResult);
static void TRM_OnDriverTxSlotAdapter(TK8710IrqResult* irqResult);
static void TRM_OnDriverSlotRxAdapter(TK8710IrqResult* irqResult);
static void TRM_OnDriverErrorAdapter(TK8710IrqResult* irqResult);

/*==============================================================================
 * 公共接口实现
 *============================================================================*/

int TRM_Init(const TRM_InitConfig* config)
{
    if (config == NULL) {
        TRM_LOG_ERROR("TRM初始化失败: 配置参数为空");
        return TRM_ERR_PARAM;
    }
    
    if (g_trmCtx.state != TRM_STATE_UNINIT) {
        TRM_LOG_WARN("TRM已经初始化，当前状态: %d", g_trmCtx.state);
        return TRM_ERR_STATE;
    }
    
    /* 初始化TRM日志系统 */
    /* 注意：不重复初始化日志系统，使用全局设置 */
    TRM_LOG_INFO("开始初始化TRM系统");
    
    /* 清零上下文 */
    memset(&g_trmCtx, 0, sizeof(g_trmCtx));
    
    /* 保存配置 */
    memcpy(&g_trmCtx.config, config, sizeof(TRM_InitConfig));
    TRM_LOG_DEBUG("保存TRM配置: beamMaxUsers=%u, beamTimeoutMs=%u, maxFrameCount=%u", 
                  config->beamMaxUsers, config->beamTimeoutMs, config->maxFrameCount);
    
    /* 设置默认值 */
    if (g_trmCtx.config.beamMaxUsers == 0) {
        g_trmCtx.config.beamMaxUsers = TRM_BEAM_MAX_USERS_DEFAULT;
        TRM_LOG_DEBUG("使用默认波束最大用户数: %u", g_trmCtx.config.beamMaxUsers);
    }
    if (g_trmCtx.config.beamTimeoutMs == 0) {
        g_trmCtx.config.beamTimeoutMs = TRM_BEAM_TIMEOUT_DEFAULT;
        TRM_LOG_DEBUG("使用默认波束超时时间: %u ms", g_trmCtx.config.beamTimeoutMs);
    }
    if (g_trmCtx.config.maxFrameCount == 0) {
        g_trmCtx.config.maxFrameCount = 100;  /* 默认最大帧数 */
        TRM_LOG_DEBUG("使用默认最大帧数: %u", g_trmCtx.config.maxFrameCount);
    }
    
    /* 设置全局帧管理参数 */
    g_trmMaxFrameCount = g_trmCtx.config.maxFrameCount;
    TRM_LOG_DEBUG("设置全局最大帧数: %u", g_trmMaxFrameCount);
    
    /* 初始化波束管理 */
    TRM_BeamInit(g_trmCtx.config.beamMaxUsers, g_trmCtx.config.beamTimeoutMs);
    TRM_LOG_INFO("波束管理初始化完成");
    
    /* 初始化发送队列 */
    TRM_DataInit();
    TRM_LOG_INFO("发送队列初始化完成");
    
    g_trmCtx.state = TRM_STATE_INIT;
    TRM_LOG_INFO("TRM系统初始化完成，状态: INIT");
    
    return TRM_OK;
}

void TRM_Deinit(void)
{
    if (g_trmCtx.state == TRM_STATE_UNINIT) {
        TRM_LOG_WARN("TRM未初始化，无需清理");
        return;
    }
    
    TRM_LOG_INFO("开始清理TRM系统");
    
    /* 停止 */
    if (g_trmCtx.state == TRM_STATE_RUNNING) {
        TRM_Stop();
    }
    
    /* 清理波束管理 */
    TRM_BeamDeinit();
    TRM_LOG_INFO("波束管理清理完成");
    
    /* 清理发送队列 */
    TRM_DataDeinit();
    TRM_LOG_INFO("发送队列清理完成");
    
    g_trmCtx.state = TRM_STATE_UNINIT;
    TRM_LOG_INFO("TRM系统清理完成，状态: UNINIT");
}

int TRM_Start(void)
{
    if (g_trmCtx.state != TRM_STATE_INIT && 
        g_trmCtx.state != TRM_STATE_STOPPED) {
        TRM_LOG_ERROR("TRM启动失败: 状态错误，当前状态: %d", g_trmCtx.state);
        return TRM_ERR_STATE;
    }
    
    TRM_LOG_INFO("启动TRM系统");
    
    /* TRM不直接控制Driver启动，由DriverManager控制 */
    g_trmCtx.state = TRM_STATE_RUNNING;
    
    TRM_LOG_INFO("TRM系统启动完成，状态: RUNNING");
    return TRM_OK;
}

int TRM_Stop(void)
{
    if (g_trmCtx.state != TRM_STATE_RUNNING) {
        TRM_LOG_ERROR("TRM停止失败: 状态错误，当前状态: %d", g_trmCtx.state);
        return TRM_ERR_STATE;
    }
    
    TRM_LOG_INFO("停止TRM系统");
    
    /* TRM不直接控制Driver停止，由DriverManager控制 */
    g_trmCtx.state = TRM_STATE_STOPPED;
    
    TRM_LOG_INFO("TRM系统停止完成，状态: STOPPED");
    return TRM_OK;
}

int TRM_Reset(void)
{
    /* TRM不直接控制Driver复位，由DriverManager控制 */
    
    /* 清理波束 */
    TRM_ClearBeamInfo(0xFFFFFFFF);
    
    /* 清理发送队列 */
    TRM_ClearTxData(0xFFFFFFFF);
    
    return TRM_OK;
}

int TRM_IsRunning(void)
{
    return (g_trmCtx.state == TRM_STATE_RUNNING) ? 1 : 0;
}

int TRM_GetStats(TRM_Stats* stats)
{
    if (stats == NULL) {
        return TRM_ERR_PARAM;
    }
    
    memcpy(stats, &g_trmCtx.stats, sizeof(TRM_Stats));
    
    return TRM_OK;
}

void TRM_SetCurrentFrame(uint32_t frameNo)
{
    g_trmCurrentFrame = frameNo;
}

uint32_t TRM_GetCurrentFrame(void)
{
    return g_trmCurrentFrame;
}

void TRM_SetMaxFrameCount(uint32_t maxCount)
{
    g_trmMaxFrameCount = maxCount;
}

int TRM_RegisterDriverCallbacks(void)
{
    if (g_trmCtx.state == TRM_STATE_UNINIT) {
        TRM_LOG_WARN("TRM未初始化，无法注册Driver回调");
        return TRM_ERR_STATE;
    }
    
    /* 设置Driver回调结构体 */
    TK8710DriverCallbacks callbacks = {
        .onRxData = TRM_OnDriverSlotRxAdapter,
        .onTxSlot = TRM_OnDriverTxSlotAdapter,
        .onSlotEnd = TRM_OnDriverSlotEndAdapter,
        .onError = TRM_OnDriverErrorAdapter
    };
    
    /* 注册到Driver */
    TK8710RegisterCallbacks(&callbacks);
    
    TRM_LOG_INFO("TRM Driver回调注册完成");
    return TRM_OK;
}

/* 内部函数实现 */
TrmContext* TRM_GetContext(void)
{
    return &g_trmCtx;
}

/* 多回调适配函数实现 */
static void TRM_OnDriverSlotRxAdapter(TK8710IrqResult* irqResult)
{
    /* 更新统计信息 */
    g_trmCtx.stats.rxCount++;
    
    /* 调试：记录中断类型 */
    TRM_LOG_DEBUG("TRM: Received RX interrupt type=%d", irqResult->irq_type);
    
    TRM_OnDriverSlotRx(irqResult);
}

static void TRM_OnDriverTxSlotAdapter(TK8710IrqResult* irqResult)
{
    /* 调试：记录中断类型 */
    TRM_LOG_DEBUG("TRM: Received TX interrupt type=%d", irqResult->irq_type);
    
    /* S1时隙，最大128用户 */
    TRM_OnDriverTxSlot(1, 128, irqResult);
}

static void TRM_OnDriverSlotEndAdapter(TK8710IrqResult* irqResult)
{
    /* 调试：记录中断类型 */
    TRM_LOG_DEBUG("TRM: Received SlotEnd interrupt type=%d", irqResult->irq_type);
    
    /* 根据中断类型确定时隙信息 */
    uint8_t slotType = 0;
    uint8_t slotIndex = 0;
    
    switch (irqResult->irq_type) {
        case TK8710_IRQ_S0:
            slotType = 0; slotIndex = 0;  /* BCN时隙 */
            break;
        case TK8710_IRQ_S2:
            slotType = 2; slotIndex = 2;  /* S2时隙 */
            break;
        case TK8710_IRQ_S3:
            slotType = 3; slotIndex = 3;  /* S3时隙 */
            break;
        default:
            TRM_LOG_WARN("TRM: Unexpected slot end interrupt type: %d", irqResult->irq_type);
            return;
    }
    
    TRM_OnDriverSlotEnd(slotType, slotIndex, g_trmCurrentFrame);
}

static void TRM_OnDriverErrorAdapter(TK8710IrqResult* irqResult)
{
    /* 调试：记录中断类型 */
    TRM_LOG_DEBUG("TRM: Received Error interrupt type=%d", irqResult->irq_type);
    
    /* 从irqResult中提取错误信息 */
    int errorCode = irqResult->irq_type;  /* 使用中断类型作为错误码 */
    TRM_OnDriverError(irqResult);
}

static void TRM_OnDriverError(TK8710IrqResult* irqResult)
{
    int errorCode = irqResult->irq_type;
    TRM_LOG_WARN("TRM: Driver error callback, error code: %d", errorCode);
    
    /* 可以根据错误类型进行不同的处理 */
    switch (errorCode) {
        case TK8710_IRQ_RX_BCN:
            TRM_LOG_DEBUG("TRM: BCN receive error");
            break;
        case TK8710_IRQ_BRD_UD:
            TRM_LOG_DEBUG("TRM: Broadcast UD error");
            break;
        case TK8710_IRQ_BRD_DATA:
            TRM_LOG_DEBUG("TRM: Broadcast DATA error");
            break;
        case TK8710_IRQ_MD_UD:
            TRM_LOG_DEBUG("TRM: MD UD error");
            break;
        case TK8710_IRQ_ACM:
            TRM_LOG_DEBUG("TRM: ACM calibration error");
            break;
        default:
            TRM_LOG_WARN("TRM: Unknown error type: %d", errorCode);
            break;
    }
}

/*==============================================================================
 * 内部函数实现
 *============================================================================*/

static void TRM_OnDriverSlotRx(TK8710IrqResult* irqResult)
{
    TRM_LOG_DEBUG("TRM: Processing MD_DATA interrupt");
    if (irqResult->mdDataValid) {
        TRM_LOG_DEBUG("TRM: RxData valid_users=%d, crc_errors=%d", 
               irqResult->crcValidCount, irqResult->crcErrorCount);
        
        /* 收集所有CRC正确的用户索引 */
        uint8_t validUserIndices[128];
        uint8_t validUserCount = 0;
        
        for (uint8_t i = 0; i < 128; i++) {
            if (irqResult->crcResults[i].userIndex < 128 && 
                irqResult->crcResults[i].dataValid) {
                validUserIndices[validUserCount++] = i;
            }
        }
        
        /* 批量处理所有CRC正确的用户 */
        if (validUserCount > 0) {
            TRM_ProcessRxUserDataBatch(validUserIndices, validUserCount, irqResult->crcResults, irqResult);
        }
    } else {
        TRM_LOG_DEBUG("TRM: MD_DATA interrupt but mdDataValid=0, skipping");
    }
}

/*==============================================================================
 * Driver回调函数实现
 *============================================================================*/

static void TRM_OnDriverSlotEnd(uint8_t slotType, uint8_t slotIndex, uint32_t frameNo)
{
    TRM_LOG_DEBUG("TRM: Slot end: type=%d, index=%d, frame=%u", slotType, slotIndex, frameNo);
    
    /* 根据时隙类型处理 */
    switch (slotType) {
        case 0: /* TK8710_IRQ_S0 */
            /* Slot0时隙结束 */
            break;
            
        case 1: /* TK8710_IRQ_S1 */
            /* Slot1时隙结束 */
            break;
            
        case 2: /* TK8710_IRQ_S2 */
            /* Slot2时隙结束 */
            break;
            
        case 3: /* TK8710_IRQ_S3 */
            /* Slot3时隙结束 - 系统帧号加1，并更新当前系统帧号 */
            frameNo++;
            TRM_SetCurrentFrame(frameNo);
            TRM_LOG_DEBUG("TRM: Frame updated to %u after S3 slot", frameNo);
            break;
            
        default:
            break;
    }
}

static void TRM_OnDriverTxSlot(uint8_t slotIndex, uint8_t maxUserCount, TK8710IrqResult* irqResult)
{
    TRM_LOG_DEBUG("TRM: TxSlot: slot=%d, maxUsers=%d\n", slotIndex, maxUserCount);
    
    /* 从发送队列取数据，查询波束，调用Driver发送 */
    int sentCount = TRM_ProcessTxSlot(slotIndex, maxUserCount, irqResult);
    
    if (sentCount > 0) {
        g_trmCtx.stats.txCount += sentCount;
        TRM_LOG_DEBUG("TRM: TxSlot: sent %d users\n", sentCount);
    }
}

/* 这些函数在trm_beam.c和trm_data.c中实现，这里不需要重复定义 */
