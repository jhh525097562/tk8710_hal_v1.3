/**
 * @file trm_api.h
 * @brief TRM (Transmission Resource Management) 公共API头文件
 * @note 此文件包含应用层可调用的TRM API接口函数
 */

#ifndef TRM_API_H
#define TRM_API_H

#include <stdint.h>
#include <stddef.h>
#include "driver/tk8710_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =============================================================================
 * 错误码定义
 * ============================================================================= */
#define TRM_OK                  0
#define TRM_ERR_PARAM           (-1)
#define TRM_ERR_STATE           (-2)
#define TRM_ERR_TIMEOUT         (-3)
#define TRM_ERR_NO_MEM          (-4)
#define TRM_ERR_NOT_INIT        (-5)
#define TRM_ERR_QUEUE_FULL      (-6)
#define TRM_ERR_NO_BEAM         (-7)

/* =============================================================================
 * 系统初始化与控制API
 * ============================================================================= */

/**
 * @brief 初始化TRM系统
 * @param config 初始化配置参数
 * @return TRM_OK成功，其他失败
 */
int TRM_Init(const TRM_InitConfig* config);

/**
 * @brief 启动TRM系统
 * @return TRM_OK成功，其他失败
 */
int TRM_Start(void);

/**
 * @brief 停止TRM系统
 * @return TRM_OK成功，其他失败
 */
int TRM_Stop(void);

/**
 * @brief 清理TRM系统，释放所有资源
 */
void TRM_Deinit(void);

/* =============================================================================
 * 数据发送API
 * ============================================================================= */

/**
 * @brief 发送用户数据（缓存到发送队列）
 * @param userId 用户ID
 * @param data 发送数据指针
 * @param len 数据长度 (最大512字节)
 * @param txPower 发送功率 (0-31)
 * @param frameNo 目标帧号
 * @param targetRateMode 目标速率模式
 * @param dataType 数据类型 (TK8710_USER_DATA_TYPE_NORMAL 或 TK8710_USER_DATA_TYPE_SLOT3)
 * @return TRM_OK成功，其他失败
 */
int TRM_SendData(uint32_t userId, const uint8_t* data, uint16_t len, uint8_t txPower, uint32_t frameNo, uint8_t targetRateMode, uint8_t dataType);

/**
 * @brief 发送广播数据
 * @param brdIndex 广播索引 (0-15)
 * @param data 发送数据指针
 * @param len 数据长度 (最大260字节)
 * @param txPower 发送功率 (0-31)
 * @param dataType 数据类型 (TK8710_BRD_DATA_TYPE_NORMAL 或 TK8710_BRD_DATA_TYPE_SLOT3)
 * @return TRM_OK成功，其他失败
 */
int TRM_SendBroadcast(uint8_t brdIndex, const uint8_t* data, uint16_t len, uint8_t txPower, uint8_t dataType);

/**
 * @brief 清除发送数据
 * @param userId 用户ID，0xFFFFFFFF表示清除所有
 * @return TRM_OK成功，其他失败
 */
int TRM_ClearTxData(uint32_t userId);

/* =============================================================================
 * 波束获取API
 * ============================================================================= */

/**
 * @brief 获取用户波束信息
 * @param userId 用户ID
 * @param beamInfo 波束信息输出指针
 * @return TRM_OK成功，其他失败
 */
int TRM_GetBeamInfo(uint32_t userId, TRM_BeamInfo* beamInfo);

/* =============================================================================
 * TRM上层回调接口
 * ============================================================================= */

/**
 * @brief 接收数据回调函数类型
 * @param rxDataList 接收数据列表
 */
typedef void (*TRM_OnRxData)(const TRM_RxDataList* rxDataList);

/**
 * @brief 发送完成回调函数类型
 * @param userId 用户ID
 * @param result 发送结果
 */
typedef void (*TRM_OnTxComplete)(uint32_t userId, TRM_TxResult result);

/* =============================================================================
 * 状态查询API
 * ============================================================================= */

/**
 * @brief 获取TRM运行状态
 * @return 1表示运行中，0表示未运行
 */
int TRM_IsRunning(void);

/**
 * @brief 获取TRM统计信息
 * @param stats 统计信息输出
 * @return TRM_OK成功，其他失败
 */
int TRM_GetStats(TRM_Stats* stats);

/**
 * @brief 获取当前系统帧号
 * @return 当前帧号
 */
uint32_t TRM_GetCurrentFrame(void);

/**
 * @brief 设置当前系统帧号
 * @param frameNo 帧号
 */
void TRM_SetCurrentFrame(uint32_t frameNo);

/**
 * @brief 设置最大帧数
 * @param maxCount 最大帧数
 */
void TRM_SetMaxFrameCount(uint32_t maxCount);

/* =============================================================================
 * 回调函数管理API
 * ============================================================================= */

/**
 * @brief 注册TRM到Driver的回调函数
 * @return TRM_OK成功，其他失败
 */
int TRM_RegisterDriverCallbacks(void);

/* =============================================================================
 * TRM日志系统API
 * ============================================================================= */

/**
 * @brief 初始化TRM日志系统
 * @param level 日志级别
 */
void TRM_LogInit(TRMLogLevel level);

/**
 * @brief 设置TRM日志级别
 * @param level 日志级别
 */
void TRM_LogSetLevel(TRMLogLevel level);

/* =============================================================================
 * TRM日志宏
 * ============================================================================= */
#define TRM_LOG_ERROR(fmt, ...)   TRM_LogOutput(TRM_LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define TRM_LOG_WARN(fmt, ...)    TRM_LogOutput(TRM_LOG_LEVEL_WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define TRM_LOG_INFO(fmt, ...)    TRM_LogOutput(TRM_LOG_LEVEL_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define TRM_LOG_DEBUG(fmt, ...)   TRM_LogOutput(TRM_LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

/**
 * @brief TRM日志输出函数
 * @param level 日志级别
 * @param file 文件名
 * @param line 行号
 * @param format 格式字符串
 * @param ... 可变参数
 */
void TRM_LogOutput(int level, const char* file, int line, const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif /* TRM_API_H */
