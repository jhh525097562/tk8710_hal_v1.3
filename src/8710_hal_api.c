/**
 * @file 8710_hal_api.c
 * @brief TK8710 HAL API接口实现
 * @version 1.0
 * @date 2026-03-02
 * 
 * TK8710硬件抽象层API实现，提供芯片控制的核心接口
 */

#include "../inc/8710_hal_api.h"
#include "../inc/driver/tk8710_internal.h"
#include "../port/tk8710_hal.h"

/* RK3506平台特定函数声明 */
#ifdef PLATFORM_RK3506
void TK8710Rk3506Cleanup(void);
#endif

/*============================================================================
                                HAL API接口实现
============================================================================*/

/**
 * @brief HAL初始化
 * @param config 初始化配置指针，为NULL时使用默认配置
 * @return TK8710_HAL_OK成功，其他值失败
 * 
 * 初始化HAL层，分配资源，准备硬件接口
 * 按顺序执行：TK8710Init、TK8710RfConfig、TK8710LogConfig、TRM_Init、TRM_LogConfig
 */
TK8710_HalError hal_init(const TK8710_HalInitConfig* config)
{
    int ret;
    
    // 如果配置为NULL，使用默认配置
    TK8710_HalInitConfig defaultConfig = {
        .tk8710Init = NULL,              // 使用Driver层默认配置
        .driverLogConfig = {
            .logLevel = TK8710_LOG_INFO, // 信息级别
            .moduleMask = 0xFFFFFFFF     // 所有模块
        },
        .trmInitConfig = NULL,           // 使用TRM层默认配置
        .trmLogConfig = {
            .logLevel = 2,
            .enableStats = true
        }
    };
    
    // 使用传入的配置或默认配置
    const TK8710_HalInitConfig* cfg = config ? config : &defaultConfig;
    
    // 1. 初始化TK8710芯片
    ret = TK8710Init(cfg->tk8710Init);
    if (ret != TK8710_OK) {
        return TK8710_HAL_ERROR_INIT;
    }
    
    // 3. 配置日志
    ret = TK8710LogConfig(cfg->driverLogConfig.logLevel, cfg->driverLogConfig.moduleMask);
    if (ret != TK8710_OK) {
        return TK8710_HAL_ERROR_INIT;
    }
    
    // 4. 初始化TRM（TRM_Init内部会自动注册Driver回调）
    ret = TRM_Init(cfg->trmInitConfig);
    if (ret != TRM_OK) {
        return TK8710_HAL_ERROR_INIT;
    }
    
    // 5. 配置TRM日志
    ret = TRM_LogConfig(cfg->trmLogConfig.logLevel);
    if (ret != TRM_OK) {
        return TK8710_HAL_ERROR_INIT;
    }
    
    return TK8710_HAL_OK;
}

/**
 * @brief HAL配置
 * @param slotConfig 时隙配置指针，为NULL时使用当前配置
 * @return TK8710_HAL_OK成功，其他值失败
 * 
 * 配置HAL参数，设置工作模式和硬件参数
 * 调用TK8710SetConfig(TK8710_CFG_TYPE_SLOT_CFG, slotConfig)设置时隙配置
 */
TK8710_HalError hal_config(const slotCfg_t* slotConfig)
{
    int ret;
    const slotCfg_t* configToUse;
    
    // 如果传入的配置为NULL，使用当前配置
    if (slotConfig == NULL) {
        configToUse = TK8710GetSlotConfig();
        if (configToUse == NULL) {
            return TK8710_HAL_ERROR_CONFIG;
        }
    } else {
        configToUse = slotConfig;
    }
    
    // 设置时隙配置
    ret = TK8710SetConfig(TK8710_CFG_TYPE_SLOT_CFG, configToUse);
    if (ret != TK8710_OK) {
        return TK8710_HAL_ERROR_CONFIG;
    }
    
    return TK8710_HAL_OK;
}

/**
 * @brief HAL启动
 * @return TK8710_HAL_OK成功，其他值失败
 * 
 * 启动HAL，开始硬件工作，可以收发数据
 * 调用TK8710Start(TK8710_MODE_MASTER, TK8710_WORK_MODE_CONTINUOUS)启动芯片
 */
TK8710_HalError hal_start(void)
{
    int ret;
    
    // 启动TK8710芯片，使用Master模式和连续工作模式
    ret = TK8710Start(TK8710_MODE_MASTER, TK8710_WORK_MODE_CONTINUOUS);
    if (ret != TK8710_OK) {
        return TK8710_HAL_ERROR_START;
    }
    
    return TK8710_HAL_OK;
}

/**
 * @brief HAL复位
 * @return TK8710_HAL_OK成功，其他值失败
 * 
 * 复位HAL，重新初始化硬件状态
 * 按顺序执行：TK8710Reset(TK8710_RST_ALL)、TRM_Deinit()
 */
TK8710_HalError hal_reset(void)
{
    int ret;
    int trmRet;
    
    // 1. 清理TRM系统资源
    trmRet = TRM_Deinit();
    if (trmRet != TRM_OK) {
        return TK8710_HAL_ERROR_RESET;
    }

    // 2. 复位TK8710芯片（复位状态机+寄存器）
    ret = TK8710Reset(TK8710_RST_STATE_MACHINE);//TK8710_RST_STATE_MACHINE  TK8710_RST_ALL
    ret = TK8710Reset(TK8710_RST_ALL);//TK8710_RST_STATE_MACHINE  TK8710_RST_ALL
    if (ret != TK8710_OK) {
        return TK8710_HAL_ERROR_RESET;
    }

    TK8710GpioIrqEnable(0, 0);
    
    TK8710Rk3506Cleanup();

    return TK8710_HAL_OK;
}

/**
 * @brief HAL发送数据
 * @param downlinkType 下行类型 (TK8710_DOWNLINK_A=广播, TK8710_DOWNLINK_B=用户数据)
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
 * 调用TRM_SetTxData()发送数据
 */
TK8710_HalError hal_sendData(TK8710DownlinkType downlinkType, uint32_t userId_brdIndex, const uint8_t* data, uint16_t len, uint8_t txPower, uint32_t frameNo, uint8_t targetRateMode, uint8_t dataType)
{
    int ret;
    
    // 调用TRM层发送数据接口
    ret = TRM_SetTxData(downlinkType, userId_brdIndex, data, len, txPower, frameNo, targetRateMode, dataType);
    if (ret != TRM_OK) {
        return TK8710_HAL_ERROR_SEND;
    }
    
    return TK8710_HAL_OK;
}

/**
 * @brief HAL获取状态
 * @param stats 状态信息输出指针
 * @return TK8710_HAL_OK成功，其他值失败
 * 
 * 获取HAL当前状态信息
 * 调用TRM_GetStats()获取TRM统计信息
 */
TK8710_HalError hal_getStatus(TRM_Stats* stats)
{
    int ret;
    
    if (stats == NULL) {
        return TK8710_HAL_ERROR_PARAM;
    }
    
    // 调用TRM层获取统计信息
    ret = TRM_GetStats(stats);
    if (ret != TRM_OK) {
        return TK8710_HAL_ERROR_STATUS;
    }
    
    return TK8710_HAL_OK;
}
