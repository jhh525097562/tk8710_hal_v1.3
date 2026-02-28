/**
 * @file tk8710_internal.h
 * @brief TK8710 驱动内部函数头文件
 * @note 此文件包含驱动内部使用的函数，不建议应用层直接调用
 */

#ifndef TK8710_INTERNAL_H
#define TK8710_INTERNAL_H

#include "tk8710_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 内部控制函数
 * ============================================================================
 */

/**
 * @brief 控制发送BCN的天线和bcnbits
 * @param bcnantennasel 发送BCN天线配置
 * @param bcnbits 发送BCN信号中的bcnbits
 * @return 0-成功, 1-失败, 2-超时
 * @note 内部函数，不建议应用层直接调用
 */
int TK8710Txbcnctl(uint8_t bcnantennasel, uint8_t bcnbits);

#ifdef __cplusplus
}
#endif

#endif /* TK8710_INTERNAL_H */
