/**
 * @file trm.h
 * @brief TRM (Transmission Resource Management) 主头文件
 * @note 此文件包含所有TRM头文件，保持向后兼容
 * 
 * 头文件结构:
 *   - trm_api.h      : 外部API (应用层使用)
 *   - trm_internal.h : 内部函数 (TRM内部使用)
 * 
 * 建议:
 *   - 应用层只需包含 trm_api.h
 *   - 需要内部功能和调试时包含 trm_internal.h
 *   - 包含 trm.h 可获得所有功能 (向后兼容)
 */

#ifndef TRM_H
#define TRM_H

#include <stdint.h>
#include <stddef.h>

/* 包含核心TRM头文件 */
#include "trm_api.h"
#include "trm_internal.h"

/* 包含TRM子模块头文件 (避免重复定义) */
#include "trm_beam.h"
#include "trm_config.h"
#include "trm_data.h"
#include "trm_log.h"

#endif /* TRM_H */
