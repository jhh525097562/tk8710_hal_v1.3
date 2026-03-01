/**
 * @file trm_data.c
 * @brief TRM数据管理功能实现
 */

#include "../inc/trm/trm.h"
#include "../inc/trm/trm_log.h"
#include "../inc/trm/trm_beam.h"
#include "../inc/driver/tk8710_api.h"
#include "../port/tk8710_hal.h"
#include <string.h>
#include <stdio.h>

/* 外部函数声明 */
extern void TK8710EnterCritical(void);
extern void TK8710ExitCritical(void);
extern uint64_t TK8710GetTimeUs(void);
extern TrmContext* TRM_GetContext(void);

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
    uint8_t  dataType;        /**< 数据类型 (TK8710_USER_DATA_TYPE_NORMAL 或 TK8710_USER_DATA_TYPE_SLOT3) */
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


/**
 * @brief 统一发送数据接口（支持用户数据和广播数据）
 * @param downlinkType 下行类型 (TK8710_DOWNLINK_1=广播, TK8710_DOWNLINK_2=用户数据)
 * @param userIdOrIndex 用户ID或广播索引
 * @param data 数据指针
 * @param len 数据长度
 * @param txPower 发送功率
 * @param frameNo 帧号 (仅用户数据使用，广播时忽略)
 * @param targetRateMode 目标速率模式 (仅用户数据使用，广播时忽略)
 * @param BeamType 波束类型 (0=广播波束, 1=指定波束)
 * @return TRM_OK成功，其他失败
 */
int TRM_SetTxUserData(TK8710DownlinkType downlinkType, uint32_t userIdOrIndex, const uint8_t* data, uint16_t len, uint8_t txPower, uint32_t frameNo, uint8_t targetRateMode, uint8_t BeamType)
{
    if (data == NULL || len == 0) {
        TRM_LOG_ERROR("TRM_SetTxUserData失败: 参数错误 - data=%p, len=%d", data, len);
        return TRM_ERR_PARAM;
    }
    
    if (downlinkType == TK8710_DOWNLINK_1) {
        /* 广播数据模式 - 直接调用Driver发送 */
        TRM_LOG_DEBUG("TRM_SetTxUserData发送广播 - 索引=%d, 长度=%d, 功率=%d, 波束类型=%d", 
                      (uint8_t)userIdOrIndex, len, txPower, BeamType);
        
        int ret = TK8710SetTxUserData(TK8710_DOWNLINK_1, (uint8_t)userIdOrIndex, data, len, txPower, BeamType);
        if (ret == TK8710_OK) {
            TRM_LOG_DEBUG("TRM广播发送成功");
        } else {
            TRM_LOG_ERROR("TRM广播发送失败 - 错误码=%d", ret);
        }
        
        return (ret == TK8710_OK) ? TRM_OK : TRM_ERR_DRIVER;
        
    } else if (downlinkType == TK8710_DOWNLINK_2) {
        /* 用户数据模式 - 缓存到发送队列 */
        TRM_LOG_DEBUG("TRM_SetTxUserData发送用户数据 - 用户ID=0x%08X, 长度=%d, 功率=%d, 帧号=%u, 速率模式=%d", 
                      userIdOrIndex, len, txPower, frameNo, targetRateMode);
        
        return TRM_SendData(userIdOrIndex, data, len, txPower, frameNo, targetRateMode, BeamType);
        
    } else {
        TRM_LOG_ERROR("TRM_SetTxUserData失败: 无效的下行类型 - downlinkType=%d", downlinkType);
        return TRM_ERR_PARAM;
    }
}

int TRM_SendData(uint32_t userId, const uint8_t* data, uint16_t len, uint8_t txPower, uint32_t frameNo, uint8_t targetRateMode, uint8_t dataType)
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
    item->dataType = dataType;              /* 设置数据类型 */
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
    uint32_t successCount = 0;    /* 成功发送的用户数 */
    uint32_t failedCount = 0;     /* 发送失败的用户数 */
    TRM_TxUserResult txResults[TX_QUEUE_SIZE]; /* 存储发送结果 */
    uint32_t resultCount = 0;     /* 结果统计数量 */
    
    /* 首先处理波束RAM延时释放 */
    TRM_ProcessBeamRamReleases();
        
    /* 获取当前时隙配置以判断是否为多速率模式 */
    const slotCfg_t* slotCfg = TK8710GetSlotConfig();
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
                        /* 过期的帧号，记录超时失败并丢弃 */
                        TRM_LOG_WARN("TRM: Discarding expired data - user=%u, target_frame=%u, current_frame=%u", 
                                   item->userId, item->frameNo, g_trmCurrentFrame);
                        
                        /* 记录发送超时结果 */
                        if (resultCount < TX_QUEUE_SIZE) {
                            txResults[resultCount].userId = item->userId;
                            txResults[resultCount].result = TRM_TX_TIMEOUT;
                            resultCount++;
                        }
                        
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
                    /* 单速率模式下指定了速率模式，不支持，记录错误并丢弃 */
                    TRM_LOG_WARN("TRM: Single-rate mode does not support rate mode specification, discarding data - user=%u, targetRate=%d", 
                               item->userId, item->targetRateMode);
                    
                    /* 记录发送错误结果 - 参数错误 */
                    if (resultCount < TX_QUEUE_SIZE) {
                        txResults[resultCount].userId = item->userId;
                        txResults[resultCount].result = TRM_TX_ERROR;
                        resultCount++;
                    }
                    
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
                    int ret = TK8710SetTxUserData(TK8710_DOWNLINK_2, txUserIndex, item->data, item->len, item->power, item->dataType);
                    if (ret != TK8710_OK) {
                        TRM_LOG_ERROR("TRM: Failed to set TX user data for user[%u]: %d", item->userId, ret);
                        
                        /* 记录发送失败结果 - Driver错误 */
                        if (resultCount < TX_QUEUE_SIZE) {
                            txResults[resultCount].userId = item->userId;
                            txResults[resultCount].result = TRM_TX_ERROR;
                            resultCount++;
                        }
                    } else {
                        /* 设置发送用户信息 - 使用波束信息 */
                        ret = TK8710SetTxUserInfo(txUserIndex, beam.freq, beam.ahData, beam.pilotPower);
                        if (ret != TK8710_OK) {
                            TRM_LOG_ERROR("TRM: Failed to set TX user info for user[%u]: %d", item->userId, ret);
                            
                            /* 记录发送失败结果 - Driver错误 */
                            if (resultCount < TX_QUEUE_SIZE) {
                                txResults[resultCount].userId = item->userId;
                                txResults[resultCount].result = TRM_TX_ERROR;
                                resultCount++;
                            }
                        } else {
                            TRM_LOG_DEBUG("TRM: Driver send setup successful for user[%u] with txIndex=%u, targetRate=%d", 
                                        item->userId, txUserIndex, item->targetRateMode);
                            sentCount++;
                            successCount++;
                            
                            /* 记录发送成功结果 */
                            if (resultCount < TX_QUEUE_SIZE) {
                                txResults[resultCount].userId = item->userId;
                                txResults[resultCount].result = TRM_TX_OK;
                                resultCount++;
                            }
                            
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
                    failedCount++;
                    
                    /* 记录发送失败结果 */
                    if (resultCount < TX_QUEUE_SIZE) {
                        txResults[resultCount].userId = item->userId;
                        txResults[resultCount].result = TRM_TX_NO_BEAM;
                        resultCount++;
                    }
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
    
    /* 调用发送完成回调，通知上层发送结果及剩余队列数 */
    if (resultCount > 0) {
        TrmContext* ctx = TRM_GetContext();
        if (ctx && ctx->config.callbacks.onTxComplete) {
            /* 构建发送结果结构体 */
            TRM_TxCompleteResult txResult;
            txResult.totalUsers = resultCount;
            txResult.remainingQueue = g_txQueue.count;
            txResult.userCount = resultCount;
            txResult.users = txResults;  /* 直接使用结果数组 */
            
            /* 一次性调用回调，传递所有结果 */
            ctx->config.callbacks.onTxComplete(&txResult);
            TRM_LOG_DEBUG("TRM: Called TxComplete callback with %u users, remaining queue count: %u", 
                         resultCount, g_txQueue.count);
        }
    }
    
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
    memset(&g_txQueue, 0, sizeof(g_txQueue));
    memset(&g_beamReleaseQueue, 0, sizeof(g_beamReleaseQueue));
    g_trmCurrentFrame = 0;
}

/**
 * @brief 获取发送队列当前数量
 * @return 发送队列当前数量
 */
uint32_t TRM_GetTxQueueCount(void)
{
    return g_txQueue.count;
}

/**
 * @brief 获取发送队列最大容量
 * @return 发送队列最大容量
 */
uint32_t TRM_GetTxQueueCapacity(void)
{
    return TX_QUEUE_SIZE;
}

/*==============================================================================
 * 接收数据处理实现
 *============================================================================*/

/**
 * @brief 批量处理接收的用户数据
 * @param userIndices 用户索引数组
 * @param userCount 用户数量
 * @param crcResults CRC结果数组
 * @param irqResult Driver中断结果
 * @return 0-成功, 其他-失败
 */
int TRM_ProcessRxUserDataBatch(uint8_t* userIndices, uint8_t userCount, TK8710CrcResult* crcResults, TK8710IrqResult* irqResult)
{
    /* 创建用户数据存储数组 */
    static TRM_RxUserData userStorage[128];  /* 静态存储用户数据数组 */
    TRM_RxDataList rxDataList;
    rxDataList.slotIndex = 0;  /* TODO: 获取实际时隙索引 */
    rxDataList.frameNo = TRM_GetCurrentFrame();  /* 获取当前系统帧号 */
    rxDataList.userCount = userCount;
    rxDataList.users = userStorage;  /* 指向用户数据数组 */
    
    TRM_LOG_DEBUG("TRM: Processing %d valid users in batch", userCount);
    
    /* 获取当前速率模式 - 直接从Driver中断结果中获取 */
    uint8_t currentRateMode = 0;
    const slotCfg_t* slotCfg = TK8710GetSlotConfig();
    if (slotCfg && slotCfg->rateCount > 0 && irqResult) {
        /* 使用Driver提供的当前速率索引获取速率模式 */
        uint8_t currentRateIndex = irqResult->currentRateIndex;
        if (currentRateIndex < slotCfg->rateCount) {
            currentRateMode = slotCfg->rateModes[currentRateIndex];
            if (slotCfg->rateCount > 1) {
                TRM_LOG_DEBUG("TRM: Multi-rate mode - rateIndex=%u, rateMode=%d", 
                             currentRateIndex, currentRateMode);
            } else {
                TRM_LOG_DEBUG("TRM: Single-rate mode - rateIndex=%u, rateMode=%d", 
                             currentRateIndex, currentRateMode);
            }
        } else {
            TRM_LOG_WARN("TRM: Invalid rate index %u, using default rate mode", currentRateIndex);
            currentRateMode = slotCfg->rateModes[0];  /* 使用第一个速率模式 */
        }
    } else if (slotCfg && slotCfg->rateCount > 0) {
        /* 信号信息无效但配置有效，使用第一个速率模式 */
        TRM_LOG_DEBUG("TRM: Signal info invalid, using first configured rate mode");
        currentRateMode = slotCfg->rateModes[0];
    } else {
        /* 配置无效，使用默认速率模式 */
        TRM_LOG_WARN("TRM: Failed to get rate info from Driver, using default rate mode");
        currentRateMode = TK8710_RATE_MODE_8;  /* 默认速率模式 */
    }
    
    /* 批量处理用户数据 */
    for (uint8_t i = 0; i < userCount; i++) {
        uint8_t userIndex = userIndices[i];
        TRM_RxUserData* currentUser = &userStorage[i];
        
        TRM_LOG_DEBUG("TRM: Processing user[%d] with CRC result", userIndex);
        
        /* 初始化用户数据结构 */
        memset(currentUser, 0, sizeof(TRM_RxUserData));
        currentUser->userId = userIndex;  /* 默认用户ID */
        currentUser->slotIndex = rxDataList.slotIndex;
        currentUser->rateMode = currentRateMode;  /* 设置接收速率模式 */  // TODO: 后面id需要从获取的数据中提取
        
        /* 从接收数据中提取用户信息 - 参考test_Driver_TRM_main_3506.c:TK8710GetRxUserInfo实现 */
        uint32_t freq;
        uint32_t ahData[16];
        uint64_t pilotPower;
        TRM_BeamInfo beam;
        memset(&beam, 0, sizeof(beam));
        
        /* 调用TK8710GetRxUserInfo获取实际的用户信息 */
        int ret = TK8710GetRxUserInfo(userIndex, &freq, ahData, &pilotPower);
        if (ret != TK8710_OK) {
            /* 如果获取失败，使用默认值 */
            TRM_LOG_WARN("TRM: Failed to get RX user info for user[%d]: %d, using defaults", userIndex, ret);
            freq = 20000;  /* 默认频率 */
            for (int j = 0; j < 16; j++) {
                ahData[j] = 8192 + j;  /* 默认AH值 */
            }
            pilotPower = 1000000;  /* 默认Pilot功率 */
        }
        
        /* 使用获取到的用户信息 */
        beam.freq = freq;  /* 实际频率 */
        memcpy(beam.ahData, ahData, sizeof(beam.ahData));  /* 实际AH数据 */
        beam.pilotPower = pilotPower;  /* 实际Pilot功率 */
        
        /* 获取用户数据 */
        uint8_t* userData;
        uint16_t dataLen;
        if (TK8710GetRxUserData(userIndex, &userData, &dataLen) == TK8710_OK) {
            // TRM_LOG_DEBUG("TRM: User[%d] received %d bytes\n", userIndex, dataLen);
            
            /* 从接收数据的前4个字节提取用户ID */
            if (dataLen >= 4) {
                beam.userId = (userData[0] << 24) | (userData[1] << 16) | (userData[2] << 8) | userData[3];
                TRM_LOG_DEBUG("TRM: Extracted user ID from data: 0x%08X", beam.userId);
            } else {
                /* 数据长度不足4字节，保持原有ID */
                TRM_LOG_WARN("TRM: Data length %d < 4, using default user ID %d", dataLen, beam.userId);
            }
            
            beam.valid = 1;
            beam.timestamp = TK8710GetTickMs();
            
            /* 存储波束信息 */
            int ret = TRM_SetBeamInfo(beam.userId, &beam);
            if (ret == TRM_OK) {
                /* 创建波束后，延时30个帧周期释放波束RAM */
                TRM_ScheduleBeamRamRelease(beam.userId, 30);
                TRM_LOG_DEBUG("TRM: Beam info stored successfully for user ID=0x%08X", beam.userId);
            } else {
                TRM_LOG_WARN("TRM: Failed to store beam info for user ID=0x%08X, error=%d", beam.userId, ret);
            }
            
            /* 填充用户数据 */
            currentUser->userId = beam.userId;
            currentUser->data = userData;
            currentUser->dataLen = dataLen;
            
            /* 获取信号质量信息 - 参考test_Driver_TRM_main_3506.c:TK8710GetRxUserSignalQuality实现 */
            uint32_t rssi, freqSignal;
            uint8_t snr;
            if (TK8710GetRxUserSignalQuality(userIndex, &rssi, &snr, &freqSignal) == TK8710_OK) {
                /* SNR转换：uint8_t最大255，直接除以4 */
                uint8_t snrValue = snr / 4;
                
                /* RSSI转换：11位有符号数，需要转换为有符号值 */
                uint32_t rssiRaw = rssi;
                int16_t rssiValue = (int16_t)(rssiRaw - 2048) / 4;
                
                /* 频率转换：26-bit格式转换为实际频率Hz */
                uint32_t freq26 = freqSignal & 0x03FFFFFF;  /* 取26位 */
                int32_t freqValue = freq26 > (1<<25) ? (int)(freq26 - (1<<26)) : freq26;
                
                /* 设置信号质量信息到currentUser */
                currentUser->rssi = rssiValue;               /* 设置实际RSSI值 (int16) */
                currentUser->snr = snrValue;                 /* 设置SNR值 (uint8) */
                currentUser->freq = freqValue;               /* 设置频率值 (int32) */
                
                // TRM_LOG_DEBUG("TRM: User[%d] Signal: SNR=%d, RSSI=%d, Freq=%d Hz", 
                //              userIndex, snrValue, rssiValue, freqValue/128);
            } else {
                currentUser->rssi = 0;    /* 获取失败时使用默认值 */
                currentUser->snr = 0;      /* SNR默认值 */
                currentUser->freq = 0;     /* 频率默认值 */
                TRM_LOG_WARN("TRM: Failed to get signal info for user[%d]", userIndex);
            }
            
            currentUser->beam = beam;  /* 设置波束信息，包含timestamp */
        } else {
            /* 获取数据失败，设置默认值 */
            currentUser->userId = beam.userId;
            currentUser->data = NULL;
            currentUser->dataLen = 0;
            currentUser->rssi = 0;
            currentUser->snr = 0;
            currentUser->freq = 0;
            currentUser->beam = beam;
            TRM_LOG_WARN("TRM: Failed to get RX data for user[%d]", userIndex);
        }
    }
    
    /* 一次性调用接收回调，处理所有用户 */
    TrmContext* ctx = TRM_GetContext();
    if (ctx && ctx->config.callbacks.onRxData != NULL) {
        TRM_LOG_DEBUG("TRM: Calling onRxData callback for %d users", userCount);
        ctx->config.callbacks.onRxData(&rxDataList);
    }
    
    /* 批量释放接收数据Buffer */
    for (uint8_t idx = 0; idx < userCount; idx++) {
        uint8_t userIndex = userIndices[idx];
        TK8710ReleaseRxData(userIndex);
    }
    
    return TRM_OK;
}
