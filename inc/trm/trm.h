/**

 * @file trm.h

 * @brief TRM模块对外接口（供上层应用调用? * @version 1.0

 * @date 2026-02-09

 * 

 * TRM(Transmission Resource Management)是HAL层的唯一对外接口 * 负责管理时域、波束、频域、功率等无线资源? */



#ifndef TRM_H

#define TRM_H



#include <stdint.h>

#include <stddef.h>

#include "driver/tk8710_types.h"



#ifdef __cplusplus

extern "C" {

#endif



/*==============================================================================

 * 错误码定? *============================================================================*/

#define TRM_OK                  0

#define TRM_ERR_PARAM           (-1)

#define TRM_ERR_STATE           (-2)

#define TRM_ERR_TIMEOUT         (-3)

#define TRM_ERR_NO_MEM          (-4)

#define TRM_ERR_NO_BEAM         (-5)

#define TRM_ERR_NOT_INIT        (-6)

#define TRM_ERR_QUEUE_FULL      (-7)



/*==============================================================================

 * 常量定义

 *============================================================================*/

#define TRM_BEAM_MAX_USERS_DEFAULT  3000    /* 默认最大用户数 */

#define TRM_BEAM_TIMEOUT_DEFAULT    3000    /* 默认波束超时时间(ms) */



/*==============================================================================

 * 类型定义

 *============================================================================*/



/* 波束存储模式 */

typedef enum {

    TRM_BEAM_MODE_FULL_STORE = 0,   /* 完整波束存储模式(CPU RAM) */

    TRM_BEAM_MODE_MAPPING    = 1,   /* 波束映射模式(8710 RAM) */

} TRM_BeamMode;



/* 发送结?*/

typedef enum {

    TRM_TX_OK = 0,

    TRM_TX_NO_BEAM,

    TRM_TX_TIMEOUT,

    TRM_TX_ERROR,

} TRM_TxResult;



/* TRM状态 */

typedef enum {

    TRM_STATE_UNINIT = 0,

    TRM_STATE_INIT,

    TRM_STATE_RUNNING,

    TRM_STATE_STOPPED,

} TrmState;



/* 速率模式 */

typedef enum {

    TRM_RATE_2M = 0,

    TRM_RATE_4M = 1,

    TRM_RATE_8M = 2,

} TRM_RateMode;



/* 时隙配置 */

typedef struct {

    uint8_t bcnSlotCount;       /* BCN时隙数 */

    uint8_t brdSlotCount;       /* 广播时隙数 */

    uint8_t ulSlotCount;        /* 上行时隙数 */

    uint8_t dlSlotCount;        /* 下行时隙数 */

} TRM_SlotConfig;



/* 波束信息 */

typedef struct {

    uint32_t userId;            /* 用户ID */

    uint32_t freq;              /* 频率 (26位格式) */

    uint32_t ahData[16];        /* AH数据: 8天线×2(I/Q) */

    uint64_t pilotPower;        /* Pilot功率 */

    uint32_t timestamp;         /* 更新时间戳*/

    uint8_t  valid;             /* 有效标志 */

} TRM_BeamInfo;



/* 接收用户数据 */

typedef struct {

    uint32_t userId;            /* 用户ID */

    uint8_t  slotIndex;         /* 时隙索引 */

    uint8_t  dataLen;           /* 数据长度 */

    uint8_t  rateMode;          /* 接收速率模式 */

    int16_t  rssi;              /* 信号强度 */

    uint8_t  snr;               /* 信噪比 */

    uint8_t  reserved;

    uint8_t* data;              /* 数据指针 */

    int32_t  freq;              /* 频率 */

    TRM_BeamInfo beam;          /* 波束信息 */

} TRM_RxUserData;



/* 接收数据列表 */

typedef struct {

    uint8_t  slotIndex;         /* 时隙索引 */

    uint8_t  userCount;         /* 用户数量 */

    uint16_t reserved;

    uint32_t frameNo;           /* 帧号 */

    TRM_RxUserData* users;      /* 用户数据数组 */

} TRM_RxDataList;



/* 广播接收数据 */

typedef struct {

    uint8_t  brdIndex;          /* 广播索引 */

    uint8_t  dataLen;           /* 数据长度 */

    uint16_t reserved;

    uint8_t* data;              /* 数据指针 */

} TRM_RxBrdData;



/* 统计信息 */

typedef struct {

    uint32_t txCount;           /* 发送次?*/

    uint32_t txSuccessCount;    /* 发送成功次?*/

    uint32_t rxCount;           /* 接收次数 */

    uint32_t beamCount;         /* 当前波束数量 */

    uint32_t memAllocCount;     /* 内存分配次数 */

    uint32_t memFreeCount;      /* 内存释放次数 */

} TRM_Stats;



/*==============================================================================

 * 回调函数类型（TRM -> 上层应用? *============================================================================*/



/* 接收数据回调 - 上层必须同步处理数据 */

typedef void (*TRM_OnRxData)(const TRM_RxDataList* rxDataList);



/* 发送完成回?*/

typedef void (*TRM_OnTxComplete)(uint32_t userId, TRM_TxResult result);



/* 回调函数集合 */

typedef struct {

    TRM_OnRxData      onRxData;

    TRM_OnTxComplete  onTxComplete;

} TRM_Callbacks;



/* TRM初始化配置 */

typedef struct {

    /* 波束配置 */

    TRM_BeamMode beamMode;          /* 波束存储模式 */

    uint32_t     beamMaxUsers;      /* 最大用户数?使用默认?*/

    uint32_t     beamTimeoutMs;     /* 波束超时时间戳使用默认?*/

    

    /* 回调函数 */

    TRM_Callbacks callbacks;

    

    /* 平台配置(预留) */

    void* platformConfig;

} TRM_InitConfig;



/* TRM上下文 */

typedef struct {

    TrmState state;

    TRM_InitConfig config;

    TRM_Stats stats;

} TrmContext;



/*==============================================================================

 * TRM对外API

 *============================================================================*/



/**

 * @brief 初始化TRM

 * @param config 初始化配? * @return TRM_OK成功，其他失? */

int TRM_Init(const TRM_InitConfig* config);



/**

 * @brief 反初始化TRM

 */

void TRM_Deinit(void);



/**

 * @brief 启动TRM

 * @return TRM_OK成功，其他失? */

int TRM_Start(void);



/**

 * @brief 停止TRM

 * @return TRM_OK成功，其他失? */

int TRM_Stop(void);



/**

 * @brief 复位TRM

 * @return TRM_OK成功，其他失? */

int TRM_Reset(void);

/**
 * @brief 发送用户数据（缓存到发送队列）
 * @param userId 用户ID
 * @param data 数据
 * @param len 数据长度
 * @param txPower 发射功率
 * @param frameNo 目标发送帧号
 * @param targetRateMode 目标速率模式 (0=使用帧号, 5-11,18=使用速率模式)
 * @param dataType 数据类型 (TK8710_USER_DATA_TYPE_NORMAL 或 TK8710_USER_DATA_TYPE_SLOT3)
 * @return TRM_OK成功，其他失败
 */
int TRM_SendData(uint32_t userId, const uint8_t* data, uint16_t len, uint8_t txPower, uint32_t frameNo, uint8_t targetRateMode, uint8_t dataType);

/**
 * @brief 发送广播数据
 * @param brdIndex 广播索引
 * @param data 广播数据
 * @param len 数据长度
 * @param txPower 发射功率
 * @param dataType 数据类型 (TK8710_BRD_DATA_TYPE_NORMAL 或 TK8710_BRD_DATA_TYPE_SLOT3)
 * @return TRM_OK成功，其他失败
 */
int TRM_SendBroadcast(uint8_t brdIndex, const uint8_t* data, uint16_t len, uint8_t txPower, uint8_t dataType);

/**
 * @brief 清除发送数据
 * @param userId 用户ID，0xFFFFFFFF表示清除所有
 * @return TRM_OK成功，其他失败
 */
int TRM_ClearTxData(uint32_t userId);



/**

 * @brief 设置波束信息

 * @param userId 用户ID

 * @param beamInfo 波束信息

 * @return TRM_OK成功，其他失? */

int TRM_SetBeamInfo(uint32_t userId, const TRM_BeamInfo* beamInfo);



/**

 * @brief 获取波束信息

 * @param userId 用户ID

 * @param beamInfo 波束信息输出

 * @return TRM_OK成功，其他失? */

int TRM_GetBeamInfo(uint32_t userId, TRM_BeamInfo* beamInfo);



/**
 * @brief 清除波束信息
 * @param userId 用户ID，0xFFFFFFFF表示清除所有
 * @return TRM_OK成功，其他失败
 */

int TRM_ClearBeamInfo(uint32_t userId);

/**
 * @brief 设置波束超时时间
 * @param timeoutMs 超时时间(毫秒)
 * @return 0-成功, 非0-失败
 */
int TRM_SetBeamTimeout(uint32_t timeoutMs);

/**
 * @brief 设置当前系统帧号
 * @param frameNo 帧号
 */

void TRM_SetCurrentFrame(uint32_t frameNo);



/**

 * @brief 获取当前系统帧号

 * @return 当前帧号

 */

uint32_t TRM_GetCurrentFrame(void);



/**

 * @brief 设置最大帧数（超帧大小）

 * @param maxCount 最大帧数

 */

void TRM_SetMaxFrameCount(uint32_t maxCount);




/**

 * @brief 获取TRM中断回调函数

 * @note 供Driver注册使用

 * @return TRM中断回调函数指针

 */

/**
 * @brief 获取TRM运行状态
 * @return 1表示运行中，0表示未运行
 */
int TRM_IsRunning(void);

/**
 * @brief 获取TRM统计信息
 * @param stats 统计信息输出
 * @return TRM_OK成功，其他失败
 */
int TRM_GetStats(TRM_Stats* stats);

/**
 * @brief 获取TRM上下文（内部函数）
 * @return TRM上下文指针
 */
TrmContext* TRM_GetContext(void);

/**
 * @brief 注册TRM到Driver的回调函数
 * @return TRM_OK成功，其他失败
 */
int TRM_RegisterDriverCallbacks(void);



/*==============================================================================

 * 全局变量声明（供内部模块使用）

 *============================================================================*/



/* 当前系统帧号和最大帧数 - 由trm_core.c管理 */

extern uint32_t g_trmCurrentFrame;

extern uint32_t g_trmMaxFrameCount;



/**

 * @brief 调度波束RAM延时释放

 * @param userId 用户ID

 * @param delayFrames 延时帧数

 * @note 内部函数，用于TRM模块内部调用

 */

void TRM_ScheduleBeamRamRelease(uint32_t userId, uint32_t delayFrames);



/**

 * @brief 处理波束RAM延时释放

 * @note 内部函数，用于TRM模块内部调用

 */

void TRM_ProcessBeamRamReleases(void);



/*==============================================================================

 * TRM时隙计算器 - 基于8710_HAL用户指南v1.0 7.2.4章节

 *============================================================================*/



/** 时隙计算器输入参数 */

typedef struct {

    uint8_t  rateMode;       /**< 速率模式: 5-11, 18 */

    uint8_t  ulBlockNum;     /**< 上行包块数 */

    uint8_t  dlBlockNum;     /**< 下行包块数 */

    uint8_t  superFrameNum;  /**< 超帧数 */

} TRM_SlotCalcInput;



/** 时隙计算器输出结果 */

typedef struct {

    uint32_t bcnSlotLen;     /**< BCN时隙长度(us) */

    uint32_t brdSlotLen;     /**< 广播时隙长度(us) */

    uint32_t ulSlotLen;      /**< 上行时隙长度(us) */

    uint32_t dlSlotLen;      /**< 下行时隙长度(us) */

    uint32_t bcnGap;         /**< BCN间隔(us) */

    uint32_t brdGap;         /**< 广播间隔(us) */

    uint32_t ulGap;          /**< 上行间隔(us) */

    uint32_t dlGap;          /**< 下行间隔(us)，用于调整帧周期 */

    uint32_t framePeriod;    /**< 调整后帧周期(us) */

    uint32_t frameCount;     /**< 帧数(framePeriod * frameCount = 1s的倍数) */

} TRM_SlotCalcOutput;



/**

 * @brief TRM时隙计算器

 * @param input 输入参数

 * @param output 输出结果

 * @return TRM_OK成功，其他失败

 * @note 基于8710_HAL用户指南v1.0 7.2.4章节实现

 */

int trm_calc_slot_config(const TRM_SlotCalcInput* input, TRM_SlotCalcOutput* output);



/**

 * @brief 打印时隙计算结果

 * @param output 计算结果

 */

void trm_print_slot_calc_result(const TRM_SlotCalcOutput* output);



#ifdef __cplusplus

}

#endif



#endif /* TRM_H */

