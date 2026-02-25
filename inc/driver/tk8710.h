/**
 * @file tk8710.h
 * @brief TK8710 驱动主头文件
 * @note 此文件包含所有头文件，保持向后兼容
 * 
 * 头文件结构:
 *   - tk8710_api.h      : 外部API (应用层使用)
 *   - tk8710_internal.h : 内部函数 (驱动内部使用)
 *   - tk8710_debug.h    : 调试接口 (测试/调试使用)
 * 
 * 建议:
 *   - 应用层只需包含 tk8710_api.h
 *   - 需要调试功能时包含 tk8710_debug.h
 *   - 包含 tk8710.h 可获得所有功能 (向后兼容)
 */

#ifndef TK8710_H
#define TK8710_H

#include "tk8710_types.h"
#include "tk8710_log.h"
#include "tk8710_regs.h"

/* 包含所有分离的头文件 */
#include "tk8710_api.h"
#include "tk8710_internal.h"
#include "tk8710_debug.h"

#endif /* TK8710_H */
