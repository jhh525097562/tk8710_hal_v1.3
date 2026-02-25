/**
 * @file trm_config.h
 * @brief TRM配置定义
 * @note TRM模块的配置参数和常量定义
 */

#ifndef TRM_CONFIG_H
#define TRM_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * TRM配置常量
 *============================================================================*/

/** TRM最大用户数 */
#define TRM_MAX_USERS                128

/** TRM最大波束数 */
#define TRM_MAX_BEAMS                256

/** TRM最大时隙数 */
#define TRM_MAX_SLOTS                32

/** TRM最大频率数 */
#define TRM_MAX_FREQUENCIES          16

/** TRM缓存大小 */
#define TRM_BUFFER_SIZE              4096

/** TRM回调队列大小 */
#define TRM_CALLBACK_QUEUE_SIZE      64

/** TRM最大功率等级数 */
#define TRM_MAX_POWER_LEVELS         32

/*============================================================================
 * TRM工作模式
 *============================================================================*/

/** TRM工作模式枚举 */
typedef enum {
    TRM_MODE_MASTER = 0,             /**< 主模式 */
    TRM_MODE_SLAVE = 1,              /**< 从模式 */
    TRM_MODE_AUTO = 2                 /**< 自动模式 */
} trm_mode_t;

/** TRM波束存储模式 */
typedef enum {
    TRM_BEAM_STORE_DISABLE = 0,      /**< 禁用波束存储 */
    TRM_BEAM_STORE_ENABLE = 1,       /**< 启用波束存储 */
    TRM_BEAM_STORE_AUTO = 2          /**< 自动波束存储 */
} trm_beam_store_mode_t;

/** TRM数据模式 */
typedef enum {
    TRM_DATA_MODE_BROADCAST = 0,     /**< 广播模式 */
    TRM_DATA_MODE_UNICAST = 1,       /**< 单播模式 */
    TRM_DATA_MODE_MULTICAST = 2      /**< 组播模式 */
} trm_data_mode_t;

/*============================================================================
 * TRM配置结构体
 *============================================================================*/

/** TRM初始化配置 */
typedef struct {
    trm_mode_t mode;                 /**< 工作模式 */
    trm_beam_store_mode_t beam_mode; /**< 波束存储模式 */
    trm_data_mode_t data_mode;       /**< 数据模式 */
    uint8_t max_users;               /**< 最大用户数 */
    uint8_t max_beams;               /**< 最大波束数 */
    uint8_t max_slots;               /**< 最大时隙数 */
    uint32_t buffer_size;            /**< 缓存大小 */
    bool enable_irq;                 /**< 使能中断 */
    bool enable_log;                 /**< 使能日志 */
} trm_config_t;

/** TRM运行时配置 */
typedef struct {
    uint32_t frame_duration_us;      /**< 帧时长(微秒) */
    uint32_t slot_duration_us;       /**< 时隙时长(微秒) */
    uint16_t beacon_interval;        /**< 信标间隔 */
    uint8_t retry_count;             /**< 重试次数 */
    uint32_t timeout_ms;             /**< 超时时间(毫秒) */
} trm_runtime_config_t;

/** TRM统计信息 */
typedef struct {
    uint32_t tx_frames;              /**< 发送帧数 */
    uint32_t rx_frames;              /**< 接收帧数 */
    uint32_t tx_bytes;               /**< 发送字节数 */
    uint32_t rx_bytes;               /**< 接收字节数 */
    uint32_t error_count;            /**< 错误计数 */
    uint32_t timeout_count;          /**< 超时计数 */
    uint32_t beam_count;             /**< 波束计数 */
    uint32_t irq_count;              /**< 中断计数 */
} trm_stats_t;

/** 时隙信息结构体 */
typedef struct {
    uint8_t slot_index;              /**< 时隙索引 */
    uint8_t slot_type;               /**< 时隙类型 */
    uint32_t start_time_us;          /**< 开始时间(微秒) */
    uint32_t duration_us;            /**< 持续时间(微秒) */
    uint8_t assigned_user;           /**< 分配用户 */
    uint16_t beam_id;                /**< 波束ID */
    bool is_tx_slot;                 /**< 是否为发送时隙 */
    bool is_rx_slot;                 /**< 是否为接收时隙 */
} trm_slot_info_t;

/** 频率信息结构体 */
typedef struct {
    uint8_t freq_index;              /**< 频率索引 */
    uint32_t frequency_hz;           /**< 频率(Hz) */
    uint16_t bandwidth_khz;          /**< 带宽(kHz) */
    uint8_t assigned_user;           /**< 分配用户 */
    uint8_t power_level;             /**< 功率等级 */
    bool is_tx_freq;                 /**< 是否为发送频率 */
    bool is_rx_freq;                 /**< 是否为接收频率 */
} trm_freq_info_t;

/** 功率信息结构体 */
typedef struct {
    uint8_t power_index;             /**< 功率索引 */
    int16_t power_level_dBm;         /**< 功率等级(dBm) */
    uint8_t antenna_id;              /**< 天线ID */
    uint8_t assigned_user;           /**< 分配用户 */
    uint32_t frequency_hz;           /**< 频率(Hz) */
    bool is_tx_power;                /**< 是否为发送功率 */
    bool is_rx_power;                /**< 是否为接收功率 */
    uint16_t control_word;           /**< 功率控制字 */
} trm_power_info_t;

/*============================================================================
 * TRM默认配置
 *============================================================================*/

/** 默认TRM配置 */
extern const trm_config_t g_default_trm_config;

/** 默认运行时配置 */
extern const trm_runtime_config_t g_default_runtime_config;

/*============================================================================
 * TRM配置函数
 *============================================================================*/

/**
 * @brief 初始化TRM配置
 * @param config 配置结构体指针
 * @return 0-成功, 非0-失败
 */
int trm_config_init(trm_config_t* config);

/**
 * @brief 验证TRM配置
 * @param config 配置结构体指针
 * @return 0-有效, 非0-无效
 */
int trm_config_validate(const trm_config_t* config);

/**
 * @brief 重置TRM统计信息
 * @param stats 统计结构体指针
 */
void trm_stats_reset(trm_stats_t* stats);

/**
 * @brief 获取TRM配置字符串
 * @param config 配置结构体指针
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 * @return 实际写入长度
 */
int trm_config_to_string(const trm_config_t* config, char* buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* TRM_CONFIG_H */
