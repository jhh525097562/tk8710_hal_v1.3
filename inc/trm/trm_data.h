/**
 * @file trm_data.h
 * @brief TRM数据管理接口
 */

#ifndef TRM_DATA_H
#define TRM_DATA_H

#include <stdint.h>
#include "trm.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 数据管理接口
 *============================================================================*/

/**
 * @brief 发送广播数据
 * @param brdIndex 广播索引
 * @param data 数据指针
 * @param len 数据长度
 * @param txPower 发送功率
 * @param dataType 数据类型 (TK8710_BRD_DATA_TYPE_NORMAL 或 TK8710_BRD_DATA_TYPE_SLOT3)
 * @return 0-成功, 非0-失败
 */
int TRM_SendBroadcast(uint8_t brdIndex, const uint8_t* data, uint16_t len, uint8_t txPower, uint8_t dataType);

/**
 * @brief 清除发送数据
 * @param userId 用户ID (0xFFFFFFFF表示清除所有)
 * @return 0-成功, 非0-失败
 */
int TRM_ClearTxData(uint32_t userId);

/*==============================================================================
 * 内部函数（供TRM核心模块使用）
 *============================================================================*/

/**
 * @brief 数据管理初始化
 */
void TRM_DataInit(void);

/**
 * @brief 数据管理反初始化
 */
void TRM_DataDeinit(void);

/**
 * @brief 处理发送时隙（内部函数）
 * @param slotIndex 时隙索引
 * @param maxUserCount 最大用户数
 * @param irqResult Driver中断结果，用于获取当前速率信息
 * @return 实际发送的用户数
 */
int TRM_ProcessTxSlot(uint8_t slotIndex, uint8_t maxUserCount, TK8710IrqResult* irqResult);

/**
 * @brief 批量处理接收的用户数据（内部函数）
 * @param userIndices 用户索引数组
 * @param userCount 用户数量
 * @param crcResults CRC结果数组
 * @param irqResult Driver中断结果
 * @return 0-成功, 其他-失败
 */
int TRM_ProcessRxUserDataBatch(uint8_t* userIndices, uint8_t userCount, TK8710CrcResult* crcResults, TK8710IrqResult* irqResult);

#ifdef __cplusplus
}
#endif

#endif /* TRM_DATA_H */
