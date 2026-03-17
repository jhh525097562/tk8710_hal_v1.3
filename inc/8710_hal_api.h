/**
 * @file hal_api.h
 * @brief TK8710 HAL API接口定义
 * @version 1.0
 * @date 2026-03-02
 * 
 * TK8710硬件抽象层API，提供芯片控制的核心接口
 */

#ifndef TK8710_8710_HAL_API_H
#define TK8710_8710_HAL_API_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/tk8710_driver_api.h"
#include "trm/trm_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* HAL错误码定义 */
typedef enum {
    TK8710_HAL_OK = 0,                /**< 成功 */
    TK8710_HAL_ERROR_PARAM = -1,       /**< 参数错误 */
    TK8710_HAL_ERROR_INIT = -2,        /**< 初始化失败 */
    TK8710_HAL_ERROR_CONFIG = -3,      /**< 配置失败 */
    TK8710_HAL_ERROR_START = -4,       /**< 启动失败 */
    TK8710_HAL_ERROR_SEND = -5,        /**< 发送失败 */
    TK8710_HAL_ERROR_STATUS = -6,      /**< 状态查询失败 */
    TK8710_HAL_ERROR_RESET = -7,      /**< 复位失败 */    
} TK8710_HalError;

/* 前向声明 */
typedef struct TK8710_Hal TK8710_Hal;

/**
 * @brief HAL初始化配置结构体
 */
typedef struct {
    /* TK8710初始化配置 */
    ChipConfig* tk8710Init;           /**< TK8710芯片初始化配置，为NULL时使用默认配置 */
    
    /* Driver日志配置 */
    struct {
        TK8710LogLevel logLevel;      /**< 日志级别 */
        uint32_t moduleMask;          /**< 模块掩码 */
        uint8_t enable_file_logging;  /**< 是否启用文件日志 */
    } driverLogConfig;
    
    /* TRM配置 */
    TRM_InitConfig* trmInitConfig;    /**< TRM初始化配置参数，为NULL时使用默认配置 */
    
    /* TRM日志配置 */
    struct {
        uint8_t logLevel;             /**< TRM日志级别 */
        uint8_t enable_file_logging;  /**< 是否启用文件日志 */
    } trmLogConfig;
} TK8710_HalInitConfig;

/*============================================================================
                                HAL API接口
============================================================================*/

/**
 * @brief HAL初始化
 * @param config 初始化配置指针，为NULL时使用默认配置
 * @return TK8710_HAL_OK成功，其他值失败
 * 
 * 初始化HAL层，分配资源，准备硬件接口
 */
TK8710_HalError hal_init(const TK8710_HalInitConfig* config);

/**
 * @brief HAL配置
 * @param slotConfig 时隙配置指针，为NULL时使用当前配置
 * @return TK8710_HAL_OK成功，其他值失败
 * 
 * 配置HAL参数，设置工作模式和硬件参数
 */
TK8710_HalError hal_config(const slotCfg_t* slotConfig);

/**
 * @brief HAL启动
 * @return TK8710_HAL_OK成功，其他值失败
 * 
 * 启动HAL，开始硬件工作，可以收发数据
 */
TK8710_HalError hal_start(void);

/**
 * @brief HAL复位
 * @return TK8710_HAL_OK成功，其他值失败
 * 
 * 复位HAL，重新初始化硬件状态
 */
TK8710_HalError hal_reset(void);

/**
 * @brief HAL发送数据
 * @param downlinkType 下行位置 (TK8710_DOWNLINK_A=slot1发送, TK8710_DOWNLINK_B=slot3发送)
 * @param userId_brdIndex 用户ID或广播索引
 * @param data 数据指针
 * @param len 数据长度
 * @param txPower 发送功率
 * @param frameNo 帧号 (仅用户数据使用，广播时忽略)
 * @param targetRateMode 目标速率模式 (仅用户数据使用，广播时忽略)
 * @param dataType 数据类型/波束类型
 * @return TK8710_HAL_OK成功，其他值失败
 * 
 * 发送数据到目标设备
 */
TK8710_HalError hal_sendData(TK8710DownlinkType downlinkType, uint32_t userId_brdIndex, 
                           const uint8_t* data, uint16_t len, uint8_t txPower, 
                           uint32_t frameNo, uint8_t targetRateMode, uint8_t dataType);

/**
 * @brief HAL获取状态
 * @param stats 状态信息输出指针
 * @return TK8710_HAL_OK成功，其他值失败
 * 
 * 获取HAL当前状态信息
 */
TK8710_HalError hal_getStatus(TRM_Stats* stats);

#ifdef __cplusplus
}
#endif

#endif /* TK8710_8710_HAL_API_H */
