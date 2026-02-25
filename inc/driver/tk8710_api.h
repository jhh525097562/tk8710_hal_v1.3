/**
 * @file tk8710_api.h
 * @brief TK8710 驱动外部API头文件
 * @note 此文件包含应用层可调用的所有公开API
 */

#ifndef TK8710_API_H
#define TK8710_API_H

#include "tk8710_types.h"
#include "../common/tk8710_log.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 芯片初始化与控制API
 * ============================================================================
 */

/**
 * @brief 初始化TK8710芯片
 * @param initConfig 初始化配置参数，为NULL时使用默认配置
 * @param irqCallback 中断回调函数指针
 * @return 0-成功, 1-失败, 2-超时
 */
int TK8710Init(const ChipConfig* initConfig, const TK8710IrqCallback* irqCallback);

/**
 * @brief 芯片进入收发状态
 * @param workType 工作类型: 1=Master, 2=Slave
 * @param workMode 工作模式: 1=连续, 2=单次
 * @return 0-成功, 1-失败, 2-超时
 */
int TK8710Startwork(uint8_t workType, uint8_t workMode);

/**
 * @brief 初始化芯片连接射频
 * @param initrfConfig 射频初始化配置参数
 * @return 0-成功, 1-失败, 2-超时
 */
int TK8710RfInit(const ChiprfConfig* initrfConfig);

/**
 * @brief 芯片复位
 * @param rstType 复位类型: 1=仅复位状态机, 2=复位状态机+寄存器
 * @return 0-成功, 1-失败, 2-超时
 */
int TK8710ResetChip(uint8_t rstType);

/**
 * @brief 设置芯片配置
 * @param type 配置类型
 * @param params 配置参数指针
 * @return 0-成功, 1-失败, 2-超时
 */
int TK8710SetConfig(TK8710ConfigType type, const void* params);

/**
 * @brief 获取芯片配置
 * @param type 配置类型
 * @param params 配置参数输出指针
 * @return 0-成功, 1-失败, 2-超时
 */
int TK8710GetConfig(TK8710ConfigType type, void* params);

/**
 * @brief 芯片控制命令
 * @param type 控制类型
 * @param params 控制参数指针
 * @return 0-成功, 非0-失败(ACM时表示异常天线位)
 */
int TK8710Ctrl(TK8710CtrlType type, const void* params);

/* ============================================================================
 * 寄存器读写API
 * ============================================================================
 */

/**
 * @brief 写寄存器
 * @param regType 寄存器类型: 0=全局, 非0=RF寄存器
 * @param addr 寄存器地址
 * @param data 写入数据
 * @return 0-成功, 1-失败, 2-超时
 */
int TK8710WriteReg(uint8_t regType, uint16_t addr, uint32_t data);

/**
 * @brief 读寄存器
 * @param regType 寄存器类型: 0=全局, 非0=RF寄存器
 * @param addr 寄存器地址
 * @param data 读取数据输出指针
 * @return 0-成功, 1-失败, 2-超时
 */
int TK8710ReadReg(uint8_t regType, uint16_t addr, uint32_t* data);

/* ============================================================================
 * Buffer读写API
 * ============================================================================
 */

/**
 * @brief 写发送数据缓冲区
 * @param start_index 数据开始index (0-127数据用户, 128-143广播用户)
 * @param data 用户数据指针
 * @param len 数据长度
 * @return 0-成功, 1-失败, 2-超时
 */
int TK8710WriteBuffer(uint8_t start_index, const uint8_t* data, size_t len);

/**
 * @brief 读接收数据缓冲区
 * @param start_index 数据开始index (0-127数据用户, 128-143广播用户)
 * @param data_out 数据输出指针
 * @param len 数据长度
 * @return 0-成功, 1-失败, 2-超时
 */
int TK8710ReadBuffer(uint8_t start_index, uint8_t* data_out, size_t len);

/* ============================================================================
 * Efuse读写API
 * ============================================================================
 */

/**
 * @brief 写Efuse
 * @param start_bit 起始bit
 * @param data_bits 写入数据
 * @param len_bits 写入长度
 * @return 0-成功, 1-失败, 2-超时
 */
int TK8710EfuseWrite(uint16_t start_bit, const uint8_t* data_bits, size_t len_bits);

/**
 * @brief 读Efuse
 * @param start_bit 起始bit
 * @param data_bits 读取数据输出
 * @param len_bits 读取长度
 * @return 0-成功, 1-失败, 2-超时
 */
int TK8710EfuseRead(uint16_t start_bit, uint8_t* data_bits, size_t len_bits);

/* ============================================================================
 * 数据发送API
 * ============================================================================
 */

/**
 * @brief 设置下行发送数据和功率
 * @param userIndex 用户索引 (0-127)
 * @param data 用户数据指针
 * @param dataLen 数据长度
 * @param txPower 发送功率
 * @param dataType 数据类型: 0=正常发送(指定信息模式波束), 1=与Slot1共用波束信息
 * @return 0-成功, 1-失败
 */
int TK8710SetTxUserDataWithPower(uint8_t userIndex, const uint8_t* data, uint16_t dataLen, uint8_t txPower, uint8_t dataType);

/**
 * @brief 设置发送用用户信息 (指定信息发送模式)
 * @param userIndex 用户索引 (0-127)
 * @param freq 频率
 * @param ahData AH数据数组 (16个32位数据)
 * @param pilotPower Pilot功率
 * @return 0-成功, 1-失败
 */
int TK8710SetTxUserInfo(uint8_t userIndex, uint32_t freq, const uint32_t* ahData, uint64_t pilotPower);

/**
 * @brief 获取发送用用户信息
 * @param userIndex 用户索引 (0-127)
 * @param freq 输出频率指针
 * @param ahData 输出AH数据数组 (16个32位数据)
 * @param pilotPower 输出Pilot功率指针
 * @return 0-成功, 1-失败
 */
int TK8710GetTxUserInfo(uint8_t userIndex, uint32_t* freq, uint32_t* ahData, uint64_t* pilotPower);

/**
 * @brief 清除发送用用户信息
 * @param userIndex 用户索引 (0-127), 255表示清除所有
 * @return 0-成功, 1-失败
 */
int TK8710ClearTxUserInfo(uint8_t userIndex);

/**
 * @brief 清除自动发送用户数据
 * @param userIndex 用户索引 (0-127), 255表示清除所有
 * @return 0-成功, 1-失败
 */
int TK8710ClearTxUserData(uint8_t userIndex);

/**
 * @brief 设置广播发送数据
 * @param brdIndex 广播用户索引 (0-15)
 * @param data 广播数据指针
 * @param dataLen 数据长度
 * @return 0-成功, 1-失败
 */
int TK8710SetTxBrdData(uint8_t brdIndex, const uint8_t* data, uint16_t dataLen);

/**
 * @brief 设置广播发送数据和功率
 * @param brdIndex 广播用户索引 (0-15)
 * @param data 广播数据指针
 * @param dataLen 数据长度
 * @param txPower 发送功率
 * @param dataType 数据类型: 0=正常广播(Driver自动生成波束), 1=与Slot3共用波束信息
 * @return 0-成功, 1-失败
 */
int TK8710SetTxBrdDataWithPower(uint8_t brdIndex, const uint8_t* data, uint16_t dataLen, uint8_t txPower, uint8_t dataType);

/* ============================================================================
 * 数据接收API
 * ============================================================================
 */

/**
 * @brief 获取接收数据
 * @param userIndex 用户索引 (0-127)
 * @param data 数据指针输出
 * @param dataLen 数据长度输出
 * @return 0-成功, 1-失败
 */
int TK8710GetRxData(uint8_t userIndex, uint8_t** data, uint16_t* dataLen);

/**
 * @brief 释放接收数据
 * @param userIndex 用户索引 (0-127)
 * @return 0-成功, 1-失败
 */
int TK8710ReleaseRxData(uint8_t userIndex);

/**
 * @brief 获取发送数据
 * @param userIndex 用户索引 (0-127)
 * @param data 数据指针输出
 * @param dataLen 数据长度输出
 * @param txPower 发送功率输出
 * @return 0-成功, 1-失败
 */
int TK8710GetTxData(uint8_t userIndex, uint8_t** data, uint16_t* dataLen, uint8_t* txPower);

/**
 * @brief 获取广播数据
 * @param brdIndex 广播用户索引 (0-15)
 * @param data 数据指针输出
 * @param dataLen 数据长度输出
 * @param txPower 发送功率输出
 * @return 0-成功, 1-失败
 */
int TK8710GetBrdData(uint8_t brdIndex, uint8_t** data, uint16_t* dataLen, uint8_t* txPower);

/**
 * @brief 获取接收用户信息 (从MD_UD中断获取的数据)
 * @param userIndex 用户索引 (0-127)
 * @param freq 输出频率指针
 * @param ahData 输出AH数据数组 (16个32位数据)
 * @param pilotPower 输出Pilot功率指针
 * @return 0-成功, 1-失败
 */
int TK8710GetRxUserInfo(uint8_t userIndex, uint32_t* freq, uint32_t* ahData, uint64_t* pilotPower);

/**
 * @brief 获取信号质量信息
 * @param userIndex 用户索引 (0-127)
 * @param rssi RSSI值输出
 * @param snr SNR值输出
 * @param freq 频率值输出
 * @return 0-成功, 1-失败
 */
int TK8710GetSignalInfo(uint8_t userIndex, uint32_t* rssi, uint8_t* snr, uint32_t* freq);

/* ============================================================================
 * 中断管理API
 * ============================================================================
 */

/**
 * @brief 初始化中断模块
 * @param irqCallback 中断回调函数指针
 */
void TK8710IrqInit(const TK8710IrqCallback* irqCallback);

/**
 * @brief 获取当前中断状态
 * @return 中断状态位图
 */
uint32_t TK8710GetIrqStatus(void);

/**
 * @brief 清除中断状态
 * @param mask 要清除的中断位
 */
void TK8710ClearIrqStatus(uint32_t mask);

/**
 * @brief 使能指定中断
 * @param mask 要使能的中断位
 */
void TK8710EnableIrq(uint32_t mask);

/**
 * @brief 禁用指定中断
 * @param mask 要禁用的中断位
 */
void TK8710DisableIrq(uint32_t mask);

/**
 * @brief 获取中断结果
 * @param irqResult 中断结果输出指针
 * @return 0-成功, 1-失败
 */
int TK8710GetIrqResult(TK8710IrqResult* irqResult);

/**
 * @brief 获取中断计数器
 * @param irqType 中断类型 (0-9)
 * @return 中断发生次数
 */
uint32_t TK8710GetIrqCounter(uint8_t irqType);

/**
 * @brief 获取所有中断计数器
 * @param counters 输出数组指针，至少10个元素
 */
void TK8710GetAllIrqCounters(uint32_t* counters);

/**
 * @brief 重置中断计数器
 */
void TK8710ResetIrqCounters(void);

/* ============================================================================
 * 日志管理API
 * ============================================================================
 */

/**
 * @brief 简化的日志初始化函数
 * @param level 日志级别
 * @param module_mask 模块掩码
 * @return 0-成功, 1-失败
 */
int TK8710LogSimpleInit(TK8710LogLevel level, uint32_t module_mask);

/**
 * @brief 设置日志级别
 * @param level 日志级别
 */
void TK8710LogSetLevel(TK8710LogLevel level);

/**
 * @brief 设置模块掩码
 * @param module_mask 模块掩码
 */
void TK8710LogSetModuleMask(uint32_t module_mask);

/**
 * @brief 设置日志回调函数
 * @param callback 回调函数指针
 */
void TK8710LogSetCallback(TK8710LogCallback callback);

/* ============================================================================
 * 状态查询API
 * ============================================================================
 */

/**
 * @brief 获取当前速率模式
 * @return 当前速率模式
 */
uint8_t TK8710GetRateMode(void);

/**
 * @brief 获取广播用户数
 * @return 当前广播用户数
 */
uint8_t TK8710GetBrdUserNum(void);

/**
 * @brief 获取速率模式参数
 * @param rateMode 速率模式
 * @param params 输出参数结构体指针
 * @return 0-成功, 1-失败
 */
int TK8710GetRateModeParams(uint8_t rateMode, RateModeParams* params);

/**
 * @brief 获取当前工作类型
 * @return 当前工作类型
 */
uint8_t TK8710GetWorkType(void);

/**
 * @brief 获取时隙配置
 * @return 时隙配置指针
 */
const slotCfg_t* TK8710GetSlotCfg(void);

/**
 * @brief 获取下行发送模式
 * @return 0=自动发送, 1=指定信息发送
 */
uint8_t TK8710GetTxAutoMode(void);

#ifdef __cplusplus
}
#endif

#endif /* TK8710_API_H */
