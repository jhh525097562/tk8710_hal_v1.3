# TK8710 HAL API接口文档

## 概述

本文档描述了TK8710 HAL (Hardware Abstraction Layer) API接口，提供统一的硬件抽象层接口，简化应用层开发。

TK8710 HAL API是对底层Driver API和TRM API的封装和组合，提供更高级、更易用的接口：

- **8710 Driver API**: 提供底层芯片控制、数据传输等基础功能
- **8710 TRM API**: 在Driver API之上构建，提供传输资源管理、波束管理、队列管理等高级功能
- **8710 HAL API**: 统一的硬件抽象层接口，封装Driver和TRM的复杂性

本文档提供了完整的API说明和使用示例。

---

## HAL API接口汇总

| API函数                | 功能描述  | 参数说明         | 返回值        |
| ---------------------- | --------- | ---------------- | ------------- |
| **初始化和控制** |           |                  |               |
| `hal_init`           | HAL初始化 | 配置结构体指针   | 成功/失败状态 |
| `hal_config`         | HAL配置   | 时隙配置指针     | 成功/失败状态 |
| `hal_start`          | 启动HAL   | 无               | 成功/失败状态 |
| `hal_reset`          | 复位HAL   | 无               | 成功/失败状态 |
| **数据传输**     |           |                  |               |
| `hal_sendData`       | 发送数据  | 完整的发送参数   | 成功/失败状态 |
| **状态查询**     |           |                  |               |
| `hal_getStatus`      | 获取状态  | 统计信息输出指针 | 成功/失败状态 |

---

## 错误码定义

### HAL错误码

| 错误码                      | 数值 | 描述         |
| --------------------------- | ---- | ------------ |
| `TK8710_HAL_OK`           | 0    | 操作成功     |
| `TK8710_HAL_ERROR_PARAM`  | -1   | 参数错误     |
| `TK8710_HAL_ERROR_INIT`   | -2   | 初始化失败   |
| `TK8710_HAL_ERROR_CONFIG` | -3   | 配置失败     |
| `TK8710_HAL_ERROR_START`  | -4   | 启动失败     |
| `TK8710_HAL_ERROR_SEND`   | -5   | 发送失败     |
| `TK8710_HAL_ERROR_STATUS` | -6   | 状态查询失败 |
| `TK8710_HAL_ERROR_RESET` | -7   | 复位失败     |

---

## 1. 初始化和控制

### 1.1 `hal_init`

```c
TK8710_HalError hal_init(const TK8710_HalInitConfig* config);
```

**功能**: 初始化HAL层，分配资源，准备硬件接口

**参数**:

- `config`: 初始化配置指针，为NULL时使用默认配置

**返回值**:

- `TK8710_HAL_OK`: 初始化成功
- `TK8710_HAL_ERROR_PARAM`: 参数错误
- `TK8710_HAL_ERROR_INIT`: 初始化失败

**说明**:

- 按顺序执行：TK8710Init、TK8710RfConfig、TK8710LogConfig、TRM_Init、TRM_LogConfig
- 如果config为NULL，使用默认配置
- TRM_Init内部会自动注册Driver回调函数
- 完成后HAL层可以正常工作

**配置结构体**:

```c
/* HAL初始化主配置结构体 */
typedef struct {
    /* TK8710初始化配置 */
    ChipConfig* tk8710Init;           /**< TK8710芯片初始化配置，为NULL时使用默认配置 */
  
    /* RF配置 */
    ChiprfConfig* rfConfig;           /**< RF配置参数，为NULL时使用默认配置 */
  
    /* Driver日志配置 */
    struct {
        TK8710LogLevel logLevel;      /**< 日志级别 */
        uint32_t moduleMask;          /**< 模块掩码 */
    } driverLogConfig;
  
    /* TRM配置 */
    TRM_InitConfig* trmInitConfig;    /**< TRM初始化配置参数，为NULL时使用默认配置 */
  
    /* TRM日志配置 */
    struct {
        uint8_t logLevel;             /**< TRM日志级别 */
        bool enableStats;             /**< 是否启用统计 */
    } trmLogConfig;
} TK8710_HalInitConfig;

/* TK8710芯片初始化配置结构体 */
typedef struct {
    uint32_t bcn_agc;       /* BCN AGC长度, 默认32 */
    uint32_t interval;      /* Intval长度, 默认32 */
    uint8_t  tx_dly;        /* 下行发送时使用前几个上行ram中的信息, 默认1 */
    uint8_t  rx_dly;        /* 上行接收时使用前几个下行ram中的信息, 默认1 */
    uint8_t  ant_num;       /* 天线数量, 默认8 */
    uint8_t  user_num;      /* 用户数量, 默认128 */
    uint8_t  brd_num;       /* 广播用户数量, 默认16 */
    uint8_t  ant_sel;       /* 天线选择, 默认0xFF(全选) */
    uint8_t  work_mode;     /* 工作模式, 默认1(TDD) */
    uint8_t  ms_mode;       /* 主从模式, 默认0(主模式) */
} ChipConfig;

/* RF配置结构体 */
typedef struct {
    rfType_e rftype;        /* 射频类型 */
    uint32_t Freq;          /* 射频中心频率 */
    uint8_t  rxgain;        /* 射频接收增益 */
    uint8_t  txgain;        /* 射频发送增益 */
    uint8_t  lnaGain;       /* LNA增益 */
    uint8_t  antSel;        /* 天线选择 */
    uint8_t  tx_dac[16];    /* 发送直流校准数据, 8天线×2路(I/Q) */
} ChiprfConfig;

/* Driver日志级别枚举 */
typedef enum {
    TK8710_LOG_ERROR = 0,   /* 错误级别 */
    TK8710_LOG_WARN  = 1,   /* 警告级别 */
    TK8710_LOG_INFO  = 2,   /* 信息级别 */
    TK8710_LOG_DEBUG = 3,   /* 调试级别 */
    TK8710_LOG_TRACE = 4    /* 跟踪级别 */
} TK8710LogLevel;

/* TRM初始化配置结构体 */
typedef struct {
    /* 波束配置 */
    TRM_BeamMode beamMode;          /* 波束存储模式 */
    uint32_t     beamMaxUsers;      /* 最大用户数(使用默认?) */
    uint32_t     beamTimeoutMs;     /* 波束超时时间戳使用默认?) */
    
    /* 帧管理配置 */
    uint32_t     maxFrameCount;     /* 最大帧数 */
    
    /* 回调函数 */
    struct {
        TRM_OnRxData      onRxData;
        TRM_OnTxComplete  onTxComplete;
    } callbacks;
    
    /* 平台配置(预留) */
    void* platformConfig;
} TRM_InitConfig;

/* TRM波束存储模式枚举 */
typedef enum {
    TRM_BEAM_MODE_FULL_STORE = 0,   /* 完整波束存储模式(CPU RAM) */
    TRM_BEAM_MODE_MAPPING    = 1,   /* 波束映射模式(8710 RAM) */
} TRM_BeamMode;

/* TRM回调函数类型定义 */
typedef void (*TRM_OnRxData)(uint32_t userId, const uint8_t* data, uint16_t len, void* context);
typedef void (*TRM_OnTxComplete)(uint32_t userId, int result, void* context);
```

**使用示例**:

```c
// 使用默认配置初始化
TK8710_HalError ret = hal_init(NULL);
if (ret != TK8710_HAL_OK) {
    printf("HAL初始化失败: %d\n", ret);
    return -1;
}

// 使用自定义配置初始化
TK8710_HalInitConfig config = {
    .tk8710Init = NULL,              // 使用Driver默认配置
    .rfConfig = NULL,                // 使用Driver默认配置
    .driverLogConfig = {
        .logLevel = TK8710_LOG_INFO,
        .moduleMask = 0xFFFFFFFF
    },
    .trmInitConfig = NULL,           // 使用TRM默认配置
    .trmLogConfig = {
        .logLevel = 3,               // TRM_LOG_INFO
        .enableStats = true
    }
};

ret = hal_init(&config);
if (ret != TK8710_HAL_OK) {
    printf("HAL初始化失败: %d\n", ret);
    return -1;
}
```

### 1.2 `hal_config`

```c
TK8710_HalError hal_config(const slotCfg_t* slotConfig);
```

**功能**: 配置HAL参数，设置工作模式和硬件参数

**参数**:

- `slotConfig`: 时隙配置指针，为NULL时使用当前配置

**返回值**:

- `TK8710_HAL_OK`: 配置成功
- `TK8710_HAL_ERROR_CONFIG`: 配置失败

**说明**:

- 调用TK8710SetConfig(TK8710_CFG_TYPE_SLOT_CFG, slotConfig)设置时隙配置
- 如果slotConfig为NULL，自动获取当前配置并重新设置
- 确保时隙配置在硬件中生效

**时隙配置结构体**:

```c
/* 时隙配置子结构体 */
typedef struct {
    uint16_t byteLen;       /* 时隙字节数 */
    uint32_t timeLen;       /* 时隙时间长度, 单位us (仅查询有效) */
    uint32_t centerFreq;    /* 时隙中心频点 */
    uint32_t FreqOffset;    /* 相较于中心频点的偏移 */
    uint32_t da_m;          /* 内部DAC参数，用于配置时隙末尾的空闲长度 */
} SlotConfig;

/* 完整时隙配置结构体 */
typedef struct {
    msMode_e   msMode;          /* 主从模式 */
    uint8_t    plCrcEn;         /* Payload CRC使能 */
    uint8_t    rateCount;       /* 速率个数，范围1~4，支持最多4个速率模式 */
    rateMode_e rateModes[4];    /* 各时隙速率模式数组，对应s0Cfg-s3Cfg */
    uint8_t    brdUserNum;      /* 广播时隙用户数, 范围1~16 */
    float      brdFreq[TK8710_MAX_BRD_USERS]; /* 广播时隙发送用户的频率 */
    uint8_t    antEn;           /* 天线使能, 默认0xFF(8天线) */
    uint8_t    rfSel;           /* RF天线选择, 默认0xFF(8天线) */
    uint8_t    txAutoMode;      /* 下行发送模式: 0=自动发送, 1=指定信息发送 */
    uint8_t    txBcnEn;         /* BCN发送使能 */
    uint8_t    bcnRotation[TK8710_MAX_ANTENNAS];  /* BCN发送使能为0xff时，从bcnRotation中轮流获取当前发送bcn天线 */
    uint32_t   rx_delay;        /* RX delay, 默认0 */
    uint32_t   md_agc;          /* DATA AGC长度, 默认1024 */
    uint8_t    local_sync;      /* 本地同步: 1=产生本地同步信号, 0=接收外部同步脉冲 */
    
    SlotConfig s0Cfg[4];           /* 时隙0(BCN)配置 */
    SlotConfig s1Cfg[4];           /* 时隙1配置 */
    SlotConfig s2Cfg[4];           /* 时隙2配置 */
    SlotConfig s3Cfg[4];           /* 时隙3配置 */
    uint32_t   frameTimeLen;    /* TDD周期总时间长度, 单位us */
} slotCfg_t;

/* 主从模式枚举 */
typedef enum {
    TK8710_MS_MASTER = 0,     /* 主模式 */
    TK8710_MS_SLAVE  = 1      /* 从模式 */
} msMode_e;

/* 速率模式枚举 */
typedef enum {
    TK8710_RATE_MODE_5  = 5,
    TK8710_RATE_MODE_6  = 6,
    TK8710_RATE_MODE_7  = 7,
    TK8710_RATE_MODE_8  = 8,
    TK8710_RATE_MODE_9  = 9,
    TK8710_RATE_MODE_10 = 10,
    TK8710_RATE_MODE_11 = 11,
    TK8710_RATE_MODE_18 = 18,
} rateMode_e;
```

**使用示例**:

```c
// 使用当前配置
TK8710_HalError ret = hal_config(NULL);

// 使用自定义配置
slotCfg_t customConfig = {
    // 配置时隙参数
    .msMode = TK8710_MS_MASTER,
    .workMode = TK8710_WORK_MODE_TDD,
    // ... 其他配置
};

ret = hal_config(&customConfig);
if (ret != TK8710_HAL_OK) {
    printf("HAL配置失败: %d\n", ret);
}
```

### 1.3 `hal_start`

```c
TK8710_HalError hal_start(void);
```

**功能**: 启动HAL，开始硬件工作，可以收发数据

**返回值**:

- `TK8710_HAL_OK`: 启动成功
- `TK8710_HAL_ERROR_START`: 启动失败

**说明**:

- 调用TK8710Start(TK8710_WORK_TYPE_MASTER, TK8710_WORK_MODE_CONTINUOUS)
- 使用主模式和连续工作模式
- 启动成功后可以进行数据收发操作

**使用示例**:

```c
TK8710_HalError ret = hal_start();
if (ret != TK8710_HAL_OK) {
    printf("HAL启动失败: %d\n", ret);
    return -1;
}
printf("HAL启动成功，可以开始收发数据\n");
```

### 1.4 `hal_reset`

```c
TK8710_HalError hal_reset(void);
```

**功能**: 复位HAL，重新初始化硬件状态

**返回值**:

- `TK8710_HAL_OK`: 复位成功
- `TK8710_HAL_ERROR_RESET`: 复位失败

**说明**:

- 按顺序执行：TK8710Reset(TK8710_RST_ALL)、TRM_Deinit()
- 完全复位状态机和寄存器
- 清理TRM系统资源
- 复位后需要重新初始化才能使用

**使用示例**:

```c
TK8710_HalError ret = hal_reset();
if (ret != TK8710_HAL_OK) {
    printf("HAL复位失败: %d\n", ret);
    return -1;
}
printf("HAL复位完成\n");
```

---

## 2. 数据传输

### 2.1 `hal_sendData`

```c
TK8710_HalError hal_sendData(TK8710DownlinkType downlinkType, uint32_t userIdOrIndex, 
                           const uint8_t* data, uint16_t len, uint8_t txPower, 
                           uint32_t frameNo, uint8_t targetRateMode, uint8_t dataType);
```

**功能**: 发送数据到目标设备

**参数**:

- `downlinkType`: 下行类型 (TK8710_DOWNLINK_A=广播, TK8710_DOWNLINK_B=用户数据)
- `userIdOrIndex`: 用户ID或广播索引
- `data`: 数据指针
- `len`: 数据长度
- `txPower`: 发送功率
- `frameNo`: 帧号 (仅用户数据使用，广播时忽略)
- `targetRateMode`: 目标速率模式 (仅用户数据使用，广播时忽略)
- `dataType`: 数据类型/波束类型

**下行类型枚举**:

```c
/* 下行类型枚举 */
typedef enum {
    TK8710_DOWNLINK_A = 0,  /* 下行A (广播数据) */
    TK8710_DOWNLINK_B = 1,  /* 下行B (专用数据) */
} TK8710DownlinkType;

/* 数据类型定义 */
#define TK8710_DATA_TYPE_BRD     0   /* 广播数据: 使用Driver自动生成的波束信息或与Slot3共用波束信息 */
#define TK8710_DATA_TYPE_DED     1   /* 专用数据: 使用指定信息模式的波束信息或与Slot1共用波束信息 */

/* 速率模式定义 (可选值) */
#define TK8710_RATE_MODE_5       5
#define TK8710_RATE_MODE_6       6
#define TK8710_RATE_MODE_7       7
#define TK8710_RATE_MODE_8       8
#define TK8710_RATE_MODE_9       9
#define TK8710_RATE_MODE_10      10
#define TK8710_RATE_MODE_11      11
#define TK8710_RATE_MODE_18      18
```

**返回值**:

- `TK8710_HAL_OK`: 发送成功
- `TK8710_HAL_ERROR_SEND`: 发送失败

**说明**:

- 调用TRM_SetTxUserData发送数据
- 支持广播和单播数据发送
- 参数完全兼容TRM层接口
- 发送是异步的，实际发送结果通过回调通知

**使用示例**:

```c
// 发送广播数据
uint8_t broadcastData[] = {0x01, 0x02, 0x03, 0x04};
TK8710_HalError ret = hal_sendData(
    TK8710_DOWNLINK_A,    // 广播类型
    0,                    // 广播索引
    broadcastData,        // 数据
    sizeof(broadcastData),// 数据长度
    35,                   // 发送功率
    0,                    // 帧号（广播忽略）
    0,                    // 速率模式（广播忽略）
    1                     // 数据类型
);

// 发送用户数据
uint8_t userData[] = {0x11, 0x12, 0x13, 0x14, 0x15};
ret = hal_sendData(
    TK8710_DOWNLINK_B,    // 用户数据类型
    0x30000001,           // 用户ID
    userData,             // 数据
    sizeof(userData),     // 数据长度
    30,                   // 发送功率
    100,                  // 帧号
    2,                    // 速率模式
    3                     // 数据类型
);

if (ret != TK8710_HAL_OK) {
    printf("数据发送失败: %d\n", ret);
}
```

---

## 3. 状态查询

### 3.1 `hal_getStatus`

```c
TK8710_HalError hal_getStatus(TRM_Stats* stats);
```

**功能**: 获取HAL当前状态信息

**参数**:

- `stats`: 状态信息输出指针

**返回值**:

- `TK8710_HAL_OK`: 获取成功
- `TK8710_HAL_ERROR_PARAM`: 参数错误
- `TK8710_HAL_ERROR_STATUS`: 状态查询失败

**说明**:

- 调用TRM_GetStats获取TRM统计信息
- 提供完整的系统状态信息
- 包括发送、接收、内存、队列等统计

**统计信息结构体**:

```c
/* TRM运行状态枚举 */
typedef enum {
    TRM_STATE_UNINIT = 0,    /* 未初始化 */
    TRM_STATE_INITED,        /* 已初始化 */
    TRM_STATE_RUNNING,       /* 运行中 */
    TRM_STATE_STOPPED,       /* 已停止 */
    TRM_STATE_ERROR          /* 错误状态 */
} TrmState;

/* TRM统计信息结构体 */
typedef struct {
    /* 运行状态 */
    TrmState    state;             /* TRM运行状态 */
    
    /* 发送统计 */
    uint32_t    txCount;           /* 总发送次数 */
    uint32_t    txSuccessCount;    /* 发送成功次数 */
    uint32_t    txFailureCount;    /* 发送失败次数 */
    uint32_t    txRetryCount;      /* 发送重试次数 */
  
    /* 接收统计 */
    uint32_t    rxCount;           /* 总接收次数 */
    uint32_t    rxDataCount;       /* 接收数据次数 */
  
    /* 波束统计 */
    uint32_t    beamCount;         /* 当前波束数量 */
  
    /* 内存统计 */
    uint32_t    memAllocCount;     /* 内存分配次数 */
    uint32_t    memFreeCount;      /* 内存释放次数 */
  
    /* 队列状态 */
    uint32_t    txQueueRemaining;  /* 剩余发送队列数量 */
} TRM_Stats;
```

**使用示例**:

```c
TRM_Stats stats;
TK8710_HalError ret = hal_getStatus(&stats);
if (ret == TK8710_HAL_OK) {
    printf("=== HAL状态信息 ===\n");
    printf("发送成功: %u\n", stats.txSuccessCount);
    printf("发送失败: %u\n", stats.txFailureCount);
    printf("发送重试: %u\n", stats.txRetryCount);
    printf("接收数据: %u\n", stats.rxDataCount);
    printf("内存分配: %u\n", stats.memAllocCount);
    printf("内存释放: %u\n", stats.memFreeCount);
    printf("剩余队列: %u\n", stats.txQueueRemaining);
} else {
    printf("获取状态失败: %d\n", ret);
}
```

---

## 4. 完整使用示例

### 4.1 基本使用流程

```c
#include "8710_hal_api.h"

int main(void)
{
    TK8710_HalError ret;
  
    // 1. 初始化HAL
    printf("初始化HAL...\n");
    ret = hal_init(NULL);  // 使用默认配置
    if (ret != TK8710_HAL_OK) {
        printf("HAL初始化失败: %d\n", ret);
        return -1;
    }
    printf("HAL初始化成功\n");
  
    // 2. 配置HAL
    printf("配置HAL...\n");
    ret = hal_config(NULL);  // 使用当前配置
    if (ret != TK8710_HAL_OK) {
        printf("HAL配置失败: %d\n", ret);
        return -1;
    }
    printf("HAL配置成功\n");
  
    // 3. 启动HAL
    printf("启动HAL...\n");
    ret = hal_start();
    if (ret != TK8710_HAL_OK) {
        printf("HAL启动失败: %d\n", ret);
        return -1;
    }
    printf("HAL启动成功\n");
  
    // 4. 发送数据
    printf("发送数据...\n");
    uint8_t testData[] = {0x01, 0x02, 0x03, 0x04};
    ret = hal_sendData(
        TK8710_DOWNLINK_A,    // 广播
        0,                    // 索引
        testData,             // 数据
        sizeof(testData),     // 长度
        35,                   // 功率
        0, 0, 1               // 其他参数
    );
    if (ret != TK8710_HAL_OK) {
        printf("数据发送失败: %d\n", ret);
    } else {
        printf("数据发送成功\n");
    }
  
    // 5. 获取状态
    printf("获取状态...\n");
    TRM_Stats stats;
    ret = hal_getStatus(&stats);
    if (ret == TK8710_HAL_OK) {
        printf("发送成功次数: %u\n", stats.txSuccessCount);
        printf("接收数据次数: %u\n", stats.rxDataCount);
    }
  
    // 6. 复位（可选）
    printf("复位HAL...\n");
    ret = hal_reset();
    if (ret != TK8710_HAL_OK) {
        printf("HAL复位失败: %d\n", ret);
    }
  
    printf("程序结束\n");
    return 0;
}
```

### 4.2 自定义配置示例

```c
int custom_hal_example(void)
{
    // 自定义初始化配置
    TK8710_HalInitConfig initConfig = {
        .tk8710Init = NULL,              // 使用Driver默认配置
        .rfConfig = NULL,                // 使用Driver默认配置
        .driverLogConfig = {
            .logLevel = TK8710_LOG_DEBUG,  // 调试级别
            .moduleMask = 0xFFFFFFF        // 所有模块
        },
        .trmInitConfig = NULL,           // 使用TRM默认配置
        .trmLogConfig = {
            .logLevel = 4,               // TRM_LOG_DEBUG
            .enableStats = true
        }
    };
  
    // 初始化
    TK8710_HalError ret = hal_init(&initConfig);
    if (ret != TK8710_HAL_OK) {
        printf("HAL初始化失败: %d\n", ret);
        return -1;
    }
  
    // 自定义时隙配置
    slotCfg_t slotConfig = {
        .msMode = TK8710_MS_MASTER,
        .workMode = TK8710_WORK_MODE_TDD,
        .frameTimeLen = 1000000,  // 1秒
        // ... 其他时隙配置
    };
  
    // 配置
    ret = hal_config(&slotConfig);
    if (ret != TK8710_HAL_OK) {
        printf("HAL配置失败: %d\n", ret);
        return -1;
    }
  
    // 启动
    ret = hal_start();
    if (ret != TK8710_HAL_OK) {
        printf("HAL启动失败: %d\n", ret);
        return -1;
    }
  
    // 发送用户数据
    uint8_t userData[] = "Hello TK8710";
    ret = hal_sendData(
        TK8710_DOWNLINK_B,    // 用户数据
        0x30000001,           // 用户ID
        userData,             // 数据
        sizeof(userData)-1,   // 长度（不包括\0）
        30,                   // 功率
        100,                  // 帧号
        2,                    // 速率模式
        3                     // 数据类型
    );
  
    return 0;
}
```

---

## 5. 最佳实践

### 5.1 错误处理

```c
// 推荐的错误处理方式
TK8710_HalError ret = hal_init(NULL);
switch (ret) {
    case TK8710_HAL_OK:
        printf("HAL初始化成功\n");
        break;
    case TK8710_HAL_ERROR_PARAM:
        printf("参数错误\n");
        return -1;
    case TK8710_HAL_ERROR_INIT:
        printf("初始化失败\n");
        return -1;
    default:
        printf("未知错误: %d\n", ret);
        return -1;
}
```

### 5.2 资源管理

```c
// 程序结束时清理资源
void cleanup_hal(void)
{
    TK8710_HalError ret = hal_reset();
    if (ret != TK8710_HAL_OK) {
        printf("HAL清理失败: %d\n", ret);
    }
}
```

### 5.3 状态监控

```c
// 定期监控状态
void monitor_hal_status(void)
{
    static uint32_t lastTxCount = 0;
  
    TRM_Stats stats;
    TK8710_HalError ret = hal_getStatus(&stats);
    if (ret == TK8710_HAL_OK) {
        if (stats.txSuccessCount > lastTxCount) {
            printf("新的发送成功: %u\n", stats.txSuccessCount);
            lastTxCount = stats.txSuccessCount;
        }
      
        // 检查队列状态
        if (stats.txQueueRemaining < 10) {
            printf("警告: 发送队列接近满载\n");
        }
    }
}
```

---

## 6. 注意事项

### 6.1 线程安全

- HAL API目前不是线程安全的
- 多线程环境下需要外部同步
- 建议在单线程中使用

### 6.2 内存管理

- HAL内部会自动管理内存
- 应用层不需要手动释放HAL分配的内存
- 复位操作会清理所有内部资源

### 6.3 性能考虑

- 发送操作是异步的，不会阻塞调用线程
- 状态查询操作很快，可以频繁调用
- 初始化和复位操作较耗时，应避免频繁调用

### 6.4 兼容性

- HAL API封装了Driver和TRM的版本差异
- 应用层代码不需要关心底层实现细节
- 升级底层库时HAL接口保持稳定

---

## 7. 常见问题

### Q1: hal_init失败怎么办？

**A**: 检查以下几点：

1. 确保硬件连接正常
2. 检查配置参数是否正确
3. 查看日志输出了解具体错误原因
4. 尝试使用默认配置（传入NULL）

### Q2: 数据发送失败但返回成功？

**A**: HAL的发送操作是异步的：

1. 返回成功只表示数据已加入发送队列
2. 实际发送结果通过回调函数通知
3. 可以通过状态查询了解发送统计

### Q3: 如何知道发送是否真正成功？

**A**: 有几种方式：

1. 注册回调函数接收发送完成通知
2. 定期查询状态统计信息
3. 查看接收方的确认信息

### Q4: 复位后需要重新初始化吗？

**A**: 是的，复位会清理所有资源：

1. 复位后HAL状态回到未初始化
2. 需要重新调用hal_init
3. 需要重新配置和启动

---

## 8. 版本历史

| 版本 | 日期       | 更新内容                      |
| ---- | ---------- | ----------------------------- |
| 1.0  | 2026-03-02 | 初始版本，包含基本HAL API接口 |

---

## 9. 参考资料

- [TK8710 TRM和Driver API接口文档](TK8710_TRM_Driver_API.md)
- [TK8710 Driver API文档](driver/tk8710_api.h)
- [TK8710 TRM API文档](trm/trm_api.h)

---

*本文档最后更新时间: 2026年3月2日*
