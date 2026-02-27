/**
 * @file trm_data.c
 * @brief TRM数据管理功能实现
 */

#include "../inc/trm/trm.h"
#include "../inc/trm/trm_log.h"
#include "../inc/driver/tk8710_api.h"
#include "../port/tk8710_hal.h"
#include <string.h>
#include <stdio.h>

/* 外部函数声明 */
extern void TK8710EnterCritical(void);
extern void TK8710ExitCritical(void);
extern uint64_t TK8710GetTimeUs(void);

/*==============================================================================
 * 私有定义
 *============================================================================*/

#define TX_QUEUE_SIZE   1024      /* 发送队列大小 */
#define TX_DATA_MAX_LEN 512     /* 最大发送数据长度 */
#define BEAM_RELEASE_QUEUE_SIZE 2048  /* 波束RAM释放队列大小 */

/* 发送数据项 */
typedef struct {
    uint32_t userId;
    uint8_t  data[TX_DATA_MAX_LEN];
    uint16_t len;
    uint8_t  power;
    uint8_t  valid;
    uint8_t  targetRateMode;  /**< 目标发送速率模式 (0=使用帧号, 5-11,18=使用速率模式) */
    uint32_t timestamp;
    uint32_t frameNo;      /**< 目标发送帧号 */
} TxItem;

/* 发送队列 */
typedef struct {
    TxItem items[TX_QUEUE_SIZE];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
} TxQueue;

/* 波束RAM延时释放项 */
typedef struct {
    uint32_t userId;
    uint32_t releaseFrame;
    uint8_t  valid;
} BeamReleaseItem;

/* 波束RAM释放队列 */
typedef struct {
    BeamReleaseItem items[BEAM_RELEASE_QUEUE_SIZE];  /* 最多支持延时释放任务 */
    uint32_t head;
    uint32_t tail;
    uint32_t count;
} BeamReleaseQueue;

/*==============================================================================
 * 私有变量
 *============================================================================*/

static TxQueue g_txQueue;
static BeamReleaseQueue g_beamReleaseQueue;  /* 波束RAM释放队列 */
// static MemPool* g_txMemPool __attribute__((unused)) = NULL;  /* 保留供将来内存池优化使用 */

/*==============================================================================
 * 公共接口实现
 *============================================================================*/

/*==============================================================================
 * 私有函数
 *============================================================================*/

/**
 * @brief 添加波束RAM延时释放任务
 * @param userId 用户ID
 * @param delayFrames 延时帧数
 */
void TRM_ScheduleBeamRamRelease(uint32_t userId, uint32_t delayFrames)
{
    /* 检查是否已存在该用户的释放任务 */
    uint32_t head = g_beamReleaseQueue.head;
    uint32_t count = 0;
    
    while (count < g_beamReleaseQueue.count) {
        uint32_t index = (head + count) % BEAM_RELEASE_QUEUE_SIZE;
        BeamReleaseItem* item = &g_beamReleaseQueue.items[index];
        
        if (item->valid && item->userId == userId) {
            /* 已存在该用户的释放任务，更新释放时间 */
            item->releaseFrame = g_trmCurrentFrame + delayFrames;
            TRM_LOG_DEBUG("TRM: Updated existing beam RAM release for user[%u] at frame=%u (delay=%u)", 
                          userId, item->releaseFrame, delayFrames);
            return;
        }
        count++;
    }
    
    /* 检查队列是否已满 */
    if (g_beamReleaseQueue.count >= BEAM_RELEASE_QUEUE_SIZE) {
        TRM_LOG_WARN("Beam release queue full, cannot schedule release for user[%u]", userId);
        return;
    }
    
    uint32_t tail = (g_beamReleaseQueue.head + g_beamReleaseQueue.count) % BEAM_RELEASE_QUEUE_SIZE;
    BeamReleaseItem* item = &g_beamReleaseQueue.items[tail];
    
    item->userId = userId;
    item->releaseFrame = g_trmCurrentFrame + delayFrames;
    item->valid = 1;
    
    g_beamReleaseQueue.count++;
    
    TRM_LOG_DEBUG("TRM: Scheduled beam RAM release for user[%u] at frame=%u (delay=%u)", 
                  userId, item->releaseFrame, delayFrames);
}

/**
 * @brief 处理波束RAM延时释放
 */
void TRM_ProcessBeamRamReleases(void)
{
    static uint32_t lastReportFrame = 0;
    uint32_t processedCount = 0;
    
    while (g_beamReleaseQueue.count > 0) {
        BeamReleaseItem* item = &g_beamReleaseQueue.items[g_beamReleaseQueue.head];
        
        if (!item->valid) {
            /* 无效项，直接跳过 */
            g_beamReleaseQueue.head = (g_beamReleaseQueue.head + 1) % BEAM_RELEASE_QUEUE_SIZE;
            g_beamReleaseQueue.count--;
            continue;
        }
        
        if (item->releaseFrame <= g_trmCurrentFrame) {
            /* 到达释放时间，执行释放 */
            TRM_LOG_DEBUG("TRM: Releasing beam RAM for user[%u] at frame=%u (scheduled=%u)", 
                          item->userId, g_trmCurrentFrame, item->releaseFrame);
            
            /* 调用波束信息清理函数 */
            TRM_ClearBeamInfo(item->userId);
            
            /* 移除已处理的项 */
            item->valid = 0;
            g_beamReleaseQueue.head = (g_beamReleaseQueue.head + 1) % BEAM_RELEASE_QUEUE_SIZE;
            g_beamReleaseQueue.count--;
            processedCount++;
        } else {
            /* 还未到释放时间，停止处理 */
            break;
        }
    }
    
    /* 每100帧报告一次队列状态 */
    if (g_trmCurrentFrame - lastReportFrame >= 100) {
        TRM_LOG_INFO("TRM: Queue status - BeamRelease: %u/%u, TxQueue: %u/%u, processed=%u this frame", 
                     g_beamReleaseQueue.count, BEAM_RELEASE_QUEUE_SIZE, 
                     g_txQueue.count, TX_QUEUE_SIZE, processedCount);
        lastReportFrame = g_trmCurrentFrame;
    }
}

/*==============================================================================
 * 公共函数实现
 *============================================================================*/


int TRM_SendData(uint32_t userId, const uint8_t* data, uint16_t len, uint8_t txPower, uint32_t frameNo, uint8_t targetRateMode)
{
    if (data == NULL || len == 0 || len > TX_DATA_MAX_LEN) {
        TRM_LOG_ERROR("TRM发送数据失败: 参数错误 - data=%p, len=%d", data, len);
        return TRM_ERR_PARAM;
    }
    
    /* 检查速率模式有效性 */
    if (targetRateMode != 0 && (targetRateMode < 5 || targetRateMode > 11) && targetRateMode != 18) {
        TRM_LOG_ERROR("TRM发送数据失败: 无效的速率模式 - targetRateMode=%d", targetRateMode);
        return TRM_ERR_PARAM;
    }
    
    /* 检查帧号有效性 - 帧号应该是循环的 */
    uint32_t normalizedFrameNo = frameNo % g_trmMaxFrameCount;
    
    TK8710EnterCritical();
    
    /* 检查队列是否满 */
    if (g_txQueue.count >= TX_QUEUE_SIZE) {
        TK8710ExitCritical();
        TRM_LOG_WARN("TRM发送数据失败: 队列已满 - 当前队列数=%u", g_txQueue.count);
        return TRM_ERR_QUEUE_FULL;
    }
    
    /* 查找空闲队列项 */
    uint32_t tail = (g_txQueue.head + g_txQueue.count) % TX_QUEUE_SIZE;
    TxItem* item = &g_txQueue.items[tail];
    
    /* 填充发送数据项 */
    item->userId = userId;
    // item->frameNo = normalizedFrameNo;  /* 使用归一化的帧号 */
    item->frameNo = frameNo;  /* 使用原始帧号 */
    item->len = len;
    item->power = txPower;
    item->targetRateMode = targetRateMode;  /* 设置目标速率模式 */
    item->valid = 1;
    memcpy(item->data, data, len);
    item->timestamp = (uint32_t)(TK8710GetTimeUs() / 1000);
    
    g_txQueue.tail = (g_txQueue.tail + 1) % TX_QUEUE_SIZE;
    g_txQueue.count++;
    
    TK8710ExitCritical();
    
    if (targetRateMode == 0) {
        TRM_LOG_DEBUG("TRM数据入队成功 - 用户ID=0x%08X, 长度=%d, 功率=%d, 目标帧=%u, 队列数=%u", 
                      userId, len, txPower, frameNo, g_txQueue.count);
    } else {
        TRM_LOG_DEBUG("TRM数据入队成功(速率模式) - 用户ID=0x%08X, 长度=%d, 功率=%d, 目标速率=%d, 队列数=%u", 
                      userId, len, txPower, targetRateMode, g_txQueue.count);
    }
    
    return TRM_OK;
}

int TRM_SendBroadcast(uint8_t brdIndex, const uint8_t* data, uint16_t len, uint8_t txPower, uint8_t dataType)
{
    if (data == NULL || len == 0) {
        TRM_LOG_ERROR("TRM发送广播失败: 参数错误 - data=%p, len=%d", data, len);
        return TRM_ERR_PARAM;
    }
    
    TRM_LOG_DEBUG("TRM发送广播 - 索引=%d, 长度=%d, 功率=%d, 数据类型=%d", brdIndex, len, txPower, dataType);
    
    /* 直接调用Driver发送下行1 */
    // int ret = TK8710SetTxBrdData(brdIndex, data, len);
    int ret = TK8710SetDownlink1DataWithPower(brdIndex, data, len, txPower, dataType);
    if (ret == TK8710_OK) {
        TRM_LOG_DEBUG("TRM广播发送成功");
    } else {
        TRM_LOG_ERROR("TRM广播发送失败 - 错误码=%d", ret);
    }
    
    return ret;
}

int TRM_ClearTxData(uint32_t userId)
{
    TK8710EnterCritical();
    
    if (userId == 0xFFFFFFFF) {
        /* 清除所有 */
        memset(&g_txQueue, 0, sizeof(g_txQueue));
    } else {
        /* 清除指定用户 */
        for (uint32_t i = 0; i < TX_QUEUE_SIZE; i++) {
            if (g_txQueue.items[i].valid && g_txQueue.items[i].userId == userId) {
                g_txQueue.items[i].valid = 0;
            }
        }
    }
    
    TK8710ExitCritical();
    
    return TRM_OK;
}

/* 内部函数 - 在发送时隙回调中调用 */
int TRM_ProcessTxSlot(uint8_t slotIndex, uint8_t maxUserCount, TK8710IrqResult* irqResult)
{
    uint8_t sentCount = 0;
    
    /* 首先处理波束RAM延时释放 */
    TRM_ProcessBeamRamReleases();
        
    /* 获取当前时隙配置以判断是否为多速率模式 */
    const slotCfg_t* slotCfg = TK8710GetSlotCfg();
    uint8_t isMultiRate = (slotCfg && slotCfg->rateCount > 1);
    uint8_t currentRateMode = 0;
    uint8_t nextRateMode = 0;
        
    if (isMultiRate && irqResult) {
        /* 多速率模式：使用Driver提供的当前速率索引 */
        uint8_t currentRateIndex = irqResult->currentRateIndex;
        if (currentRateIndex < slotCfg->rateCount) {
            currentRateMode = slotCfg->rateModes[currentRateIndex];
            /* 计算下一帧的速率模式 */
            uint8_t nextRateIndex = (currentRateIndex + 1) % slotCfg->rateCount;
            nextRateMode = slotCfg->rateModes[nextRateIndex];
            TRM_LOG_DEBUG("TRM: Multi-rate mode - currentIndex=%u, currentRate=%d, nextIndex=%u, nextRate=%d", 
                         currentRateIndex, currentRateMode, nextRateIndex, nextRateMode);
        } else {
            TRM_LOG_WARN("TRM: Invalid rate index %u, using default rate mode", currentRateIndex);
            currentRateMode = slotCfg->rateModes[0];
            nextRateMode = slotCfg->rateModes[0];
        }
    } else if (slotCfg && slotCfg->rateCount > 0) {
        /* 单速率模式或信号信息无效：使用第一个速率模式 */
        currentRateMode = slotCfg->rateModes[0];
        nextRateMode = slotCfg->rateModes[0];
        TRM_LOG_DEBUG("TRM: Single-rate mode - rate=%d", currentRateMode);
    } else {
        /* 配置无效：使用默认速率模式 */
        currentRateMode = TK8710_RATE_MODE_8;
        nextRateMode = TK8710_RATE_MODE_8;
        TRM_LOG_WARN("TRM: Failed to get slot configuration, using default rate mode");
    }
        
    TK8710EnterCritical();
        
    uint8_t txUserIndex = 0;
    uint8_t futureFrameCount = 0;  /* 跟踪未来帧号的用户数量 */
    uint16_t processedCount = 0;    /* 跟踪已处理的用户数量 */
    
    while (g_txQueue.count > 0 && sentCount < maxUserCount && processedCount < g_txQueue.count) {
        TxItem* item = &g_txQueue.items[g_txQueue.head];
        processedCount++;
        
        if (item->valid) {
            uint8_t shouldSend = 0;
            
            if (isMultiRate) {
                /* 多速率模式：只检查目标速率模式是否匹配下一帧速率模式 */
                if (item->targetRateMode == nextRateMode) {
                    /* 速率模式匹配下一帧，可以发送 */
                    shouldSend = 1;
                    TRM_LOG_DEBUG("TRM: Multi-rate send match - userRate=%d, nextRate=%d, user=%u", 
                                 item->targetRateMode, nextRateMode, item->userId);
                } else {
                    /* 速率模式不匹配，跳过 */
                    TRM_LOG_DEBUG("TRM: Multi-rate send skip - userRate=%d, nextRate=%d, user=%u", 
                                 item->targetRateMode, nextRateMode, item->userId);
                }
            } else {
                /* 单速率模式：只检查帧号匹配 */
                if (item->targetRateMode == 0) {
                    /* 使用帧号匹配（原有逻辑） */
                    if (item->frameNo == g_trmCurrentFrame) {
                        shouldSend = 1;
                    } else if (item->frameNo < g_trmCurrentFrame) {
                        /* 过期的帧号，直接丢弃 */
                        TRM_LOG_WARN("TRM: Discarding expired data - user=%u, target_frame=%u, current_frame=%u", 
                                   item->userId, item->frameNo, g_trmCurrentFrame);
                        item->valid = 0;
                        g_txQueue.head = (g_txQueue.head + 1) % TX_QUEUE_SIZE;
                        g_txQueue.count--;
                        continue;
                    } else {
                        /* 未来帧号，跳过当前用户，继续处理下一个 */
                        TRM_LOG_DEBUG("TRM: Future frame - user=%u, target_frame=%u, current_frame=%u, skipping", 
                                     item->userId, item->frameNo, g_trmCurrentFrame);
                        futureFrameCount++;
                        continue;
                    }
                } else {
                    /* 单速率模式下指定了速率模式，不支持，直接丢弃 */
                    TRM_LOG_WARN("TRM: Single-rate mode does not support rate mode specification, discarding data - user=%u, targetRate=%d", 
                               item->userId, item->targetRateMode);
                    item->valid = 0;
                    g_txQueue.head = (g_txQueue.head + 1) % TX_QUEUE_SIZE;
                    g_txQueue.count--;
                    continue;
                }
            }
            
            if (shouldSend) {
                /* 查询波束 */
                TRM_BeamInfo beam;
                TRM_LOG_DEBUG("TRM: Querying beam info for user ID=0x%08X", item->userId);
                
                /* 添加安全检查，防止崩溃 */
                if (item->userId == 0) {
                    TRM_LOG_WARN("TRM: Invalid user ID=0, skipping send");
                    /* 移除无效的队列项 */
                    item->valid = 0;
                    g_txQueue.head = (g_txQueue.head + 1) % TX_QUEUE_SIZE;
                    g_txQueue.count--;
                    continue;
                }
                
                /* 恢复波束检查 */
                int beamRet = TRM_GetBeamInfo(item->userId, &beam);
                if (beamRet == TRM_OK) {
                    /* 设置发送下行2数据 - 参考test_Driver_TRM_main_3506.c实现 */
                    int ret = TK8710SetDownlink2DataWithPower(txUserIndex, item->data, item->len, item->power, TK8710_USER_DATA_TYPE_NORMAL);
                    if (ret != TK8710_OK) {
                        TRM_LOG_ERROR("TRM: Failed to set TX user data for user[%u]: %d", item->userId, ret);
                    } else {
                        /* 设置发送用户信息 - 使用波束信息 */
                        ret = TK8710SetTxUserInfo(txUserIndex, beam.freq, beam.ahData, beam.pilotPower);
                        if (ret != TK8710_OK) {
                            TRM_LOG_ERROR("TRM: Failed to set TX user info for user[%u]: %d", item->userId, ret);
                        } else {
                            TRM_LOG_DEBUG("TRM: Driver send setup successful for user[%u] with txIndex=%u, targetRate=%d", 
                                        item->userId, txUserIndex, item->targetRateMode);
                            sentCount++;
                            
                            /* 使用完波束后，延时释放波束RAM - 减少延迟以应对高负载 */
                            TRM_ScheduleBeamRamRelease(item->userId, 4);//16 -> 4
                        }
                    }
                    
                    /* 递增发送用户索引，为下一个用户准备 */
                    txUserIndex++;
                    if (txUserIndex >= 128) {  /* 防止超出最大用户数 */
                        txUserIndex = 0;
                    }
                } else {
                    /* 波束信息不存在或过期，跳过发送但记录日志 */
                    TRM_LOG_WARN("TRM: No valid beam info for user ID=0x%08X, error=%d, skipping send", item->userId, beamRet);
                }
                
                /* 移除已发送的队列项 */
                item->valid = 0;
                g_txQueue.head = (g_txQueue.head + 1) % TX_QUEUE_SIZE;
                g_txQueue.count--;
            }
        } else {
            /* 无效项，直接跳过 */
            g_txQueue.head = (g_txQueue.head + 1) % TX_QUEUE_SIZE;
            g_txQueue.count--;
        }
    }
    
    TK8710ExitCritical();
    
    /* 记录未来帧号的情况 */
    if (futureFrameCount > 0) {
        TRM_LOG_DEBUG("TRM: ProcessTxSlot found %u users with future frames, current_frame=%u", 
                     futureFrameCount, g_trmCurrentFrame);
    }
    
    TRM_LOG_DEBUG("TRM: ProcessTxSlot completed - sentCount=%d, queueCount=%u, multiRate=%s, processed=%u", 
                 sentCount, g_txQueue.count, isMultiRate ? "true" : "false", processedCount);
    
    return sentCount;
}

/* 内部初始化函数 */
void TRM_DataInit(void)
{
    memset(&g_txQueue, 0, sizeof(g_txQueue));
    memset(&g_beamReleaseQueue, 0, sizeof(g_beamReleaseQueue));
    g_trmCurrentFrame = 0;
}

/* 内部反初始化函数 */
void TRM_DataDeinit(void)
{
    TRM_ClearTxData(0xFFFFFFFF);
}
