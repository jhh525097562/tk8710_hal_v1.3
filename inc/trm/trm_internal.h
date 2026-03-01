/**
 * @file trm_internal.h
 * @brief TRM (Transmission Resource Management) 内部函数头文件
 * @note 此文件包含TRM内部使用的函数，不建议应用层直接调用
 * 
 * 内容分类:
 *   - 波束管理内部函数
 *   - 系统控制内部函数
 *   - 调试和测试接口
 *   - 内部工具函数
 * 
 * 建议:
 *   - 应用层只需包含 trm_api.h
 *   - 需要内部功能和调试时包含此文件
 *   - 包含 trm.h 可获得所有功能 (向后兼容)
 */

#ifndef TRM_INTERNAL_H
#define TRM_INTERNAL_H

#include <stdint.h>
#include <stddef.h>
#include "driver/tk8710_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =============================================================================
 * 内部错误码定义
 * ============================================================================= */
#define TRM_ERR_INTERNAL        (-8)
#define TRM_ERR_INVALID_STATE   (-9)

/* =============================================================================
 * 内部类型定义
 * ============================================================================= */

/* 回调函数集合 */
typedef struct {
    TRM_OnRxData      onRxData;
    TRM_OnTxComplete  onTxComplete;
} TRM_Callbacks;

/* 时隙配置 */
typedef struct {
    uint8_t bcnSlotCount;       /* BCN时隙数 */
    uint8_t brdSlotCount;       /* 广播时隙数 */
    uint8_t ulSlotCount;        /* 上行时隙数 */
    uint8_t dlSlotCount;        /* 下行时隙数 */
} TRM_SlotConfig;

/* 广播接收数据 */
typedef struct {
    uint8_t  brdIndex;          /* 广播索引 */
    uint8_t  dataLen;           /* 数据长度 */
    uint16_t reserved;
    uint8_t* data;              /* 数据指针 */
} TRM_RxBrdData;

/* 时隙计算输入 */
typedef struct {
    uint32_t totalUsers;        /* 总用户数 */
    uint8_t rateMode;           /* 速率模式 */
    uint8_t slotConfig;         /* 时隙配置 */
} TRM_SlotCalcInput;

/* 时隙计算输出 */
typedef struct {
    uint8_t optimalSlots;       /* 最优时隙数 */
    uint32_t maxThroughput;     /* 最大吞吐量 */
    uint8_t recommendedConfig;  /* 推荐配置 */
} TRM_SlotCalcOutput;

/* TRM上下文 */
typedef struct {
    TrmState state;
    TRM_InitConfig config;
    TRM_Stats stats;
} TrmContext;

/* =============================================================================
 * 全局变量声明
 * ============================================================================= */

/* 当前系统帧号和最大帧数 - 由trm_core.c管理 */
extern uint32_t g_trmCurrentFrame;
extern uint32_t g_trmMaxFrameCount;

/* =============================================================================
 * 波束管理内部函数
 * ============================================================================= */

/**
 * @brief 设置用户波束信息 (内部函数)
 * @param userId 用户ID
 * @param beamInfo 波束信息指针
 * @return TRM_OK成功，其他失败
 */
int TRM_SetBeamInfo(uint32_t userId, const TRM_BeamInfo* beamInfo);

/**
 * @brief 清除用户波束信息 (内部函数)
 * @param userId 用户ID，0xFFFFFFFF表示清除所有
 * @return TRM_OK成功，其他失败
 */
int TRM_ClearBeamInfo(uint32_t userId);

/**
 * @brief 设置波束超时时间 (内部函数)
 * @param timeoutMs 超时时间(毫秒)
 * @return 0-成功, 非0-失败
 */
int TRM_SetBeamTimeout(uint32_t timeoutMs);

/**
 * @brief 调度波束RAM释放 (内部函数)
 * @param userId 用户ID
 * @param delayFrames 延迟帧数
 */
void TRM_ScheduleBeamRamRelease(uint32_t userId, uint32_t delayFrames);

/**
 * @brief 处理波束RAM释放 (内部函数)
 */
void TRM_ProcessBeamRamReleases(void);

/* =============================================================================
 * 系统控制内部函数
 * ============================================================================= */

/**
 * @brief 重置TRM系统 (内部函数)
 * @return TRM_OK成功，其他失败
 */
int TRM_Reset(void);

/**
 * @brief 获取TRM上下文 (内部函数)
 * @return TRM上下文指针
 */
TrmContext* TRM_GetContext(void);

/* =============================================================================
 * 调试和测试接口
 * ============================================================================= */

/**
 * @brief TRM发送验证器接收数据处理 (测试接口)
 * @param rxDataList 接收数据列表
 * @return TRM_OK成功，其他失败
 */
int TRM_TxValidatorOnRxData(const TRM_RxDataList* rxDataList);

/* =============================================================================
 * 数据管理内部函数
 * ============================================================================= */

/**
 * @brief 发送用户数据(支持单速率和多速率模式) - 内部函数
 * @param userId 用户ID
 * @param data 数据指针
 * @param len 数据长度
 * @param txPower 发送功率
 * @param frameNo 帧号
 * @param targetRateMode 目标速率模式
 * @param dataType 数据类型
 * @return TRM_OK成功，其他失败
 */
int TRM_SendData(uint32_t userId, const uint8_t* data, uint16_t len, uint8_t txPower, uint32_t frameNo, uint8_t targetRateMode, uint8_t dataType);

/**
 * @brief 发送广播数据 - 内部函数
 * @param brdIndex 广播索引
 * @param data 数据指针
 * @param len 数据长度
 * @param txPower 发送功率
 * @param dataType 数据类型
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
 * 内部工具函数
 * ============================================================================= */

/**
 * @brief 计算时隙配置 (内部函数)
 * @param input 输入参数
 * @param output 输出参数
 * @return TRM_OK成功，其他失败
 */
int trm_calc_slot_config(const TRM_SlotCalcInput* input, TRM_SlotCalcOutput* output);

/**
 * @brief 打印时隙计算结果 (内部函数)
 * @param output 计算结果
 */
void trm_print_slot_calc_result(const TRM_SlotCalcOutput* output);

/* =============================================================================
 * 调试和测试接口
 * ============================================================================= */

/**
 * @brief TRM发送验证器接收数据处理 (测试接口)
 * @param rxDataList 接收数据列表
 * @return TRM_OK成功，其他失败
 */
int TRM_TxValidatorOnRxData(const TRM_RxDataList* rxDataList);

#ifdef __cplusplus
}
#endif

#endif /* TRM_INTERNAL_H */
