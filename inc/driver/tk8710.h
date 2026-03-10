/**
 * @file tk8710.h
 * @brief TK8710 驱动主头文件
 * @note 此文件包含所有头文件，保持向后兼容
 * 
 * 头文件结构:
 *   - tk8710_driver_api.h : 外部API (应用层使用)
 *   - tk8710_internal.h : 内部函数和调试接口 (驱动内部/调试使用)
 * 
 * 建议:
 *   - 应用层只需包含 tk8710_driver_api.h
 *   - 需要内部功能和调试时包含 tk8710_internal.h
 *   - 包含 tk8710.h 可获得所有功能 (向后兼容)
 */

#ifndef TK8710_H
#define TK8710_H

#include "tk8710_types.h"
#include "tk8710_log.h"
#include "tk8710_regs.h"

/* 包含API和内部功能头文件 */
#include "tk8710_driver_api.h"
#include "tk8710_internal.h"

#endif /* TK8710_H */
