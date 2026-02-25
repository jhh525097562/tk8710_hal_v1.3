/**
 * @file tk8710_debug.h
 * @brief TK8710 驱动调试接口头文件
 * @note 此文件包含调试和测试用的函数，生产环境可选择不包含
 */

#ifndef TK8710_DEBUG_H
#define TK8710_DEBUG_H

#include "tk8710_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 测试控制接口
 * ============================================================================
 */

/**
 * @brief 设置是否强制处理所有用户数据（测试接口）
 * @param enable 1-强制处理所有用户数据，0-按CRC结果正常处理
 * @note 此函数仅用于测试中断处理能力
 */
void TK8710SetForceProcessAllUsers(uint8_t enable);

/**
 * @brief 获取当前是否强制处理所有用户数据
 * @return 1-强制处理，0-正常处理
 */
uint8_t TK8710GetForceProcessAllUsers(void);

/**
 * @brief 设置是否强制按最大用户数发送（测试接口）
 * @param enable 1-强制按最大用户数发送，0-按实际输入用户数发送
 * @note 此函数仅用于测试中断处理能力
 */
void TK8710SetForceMaxUsersTx(uint8_t enable);

/**
 * @brief 获取当前是否强制按最大用户数发送
 * @return 1-强制发送，0-正常发送
 */
uint8_t TK8710GetForceMaxUsersTx(void);

/* ============================================================================
 * 中断时间统计接口
 * ============================================================================
 */

/**
 * @brief 获取中断处理时间统计
 * @param irqType 中断类型 (0-9)
 * @param totalTime 总处理时间 (输出参数)
 * @param maxTime 最大处理时间 (输出参数)
 * @param minTime 最小处理时间 (输出参数)
 * @param count 中断次数 (输出参数)
 * @return 0-成功, 1-失败
 */
int TK8710GetIrqTimeStats(uint8_t irqType, uint32_t* totalTime, 
                          uint32_t* maxTime, uint32_t* minTime, uint32_t* count);

/**
 * @brief 重置中断处理时间统计
 * @param irqType 中断类型 (0-9)，255表示重置所有
 * @return 0-成功, 1-失败
 */
int TK8710ResetIrqTimeStats(uint8_t irqType);

/**
 * @brief 打印中断处理时间统计报告
 */
void TK8710PrintIrqTimeStats(void);

/* ============================================================================
 * 调试控制接口
 * ============================================================================
 */

/**
 * @brief 调试控制接口
 * @param ctrlType 控制类型
 * @param optType 操作类型: 0=Set, 1=Get, 2=Exe
 * @param inputParams 输入参数指针
 * @param outputParams 输出参数指针
 * @return 0-成功, 1-失败, 2-超时
 */
int TK8710DebugCtrl(TK8710DebugCtrlType ctrlType, CtrlOptType optType,
                    const void* inputParams, void* outputParams);

#ifdef __cplusplus
}
#endif

#endif /* TK8710_DEBUG_H */
