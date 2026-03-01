# TK8710 TRM和Driver API接口文档

## 概述

本文档整理了TK8710 HAL层中TRM(Transmission Resource Management)模块和Driver模块的主要API接口，包括接口参数说明和使用方法。

---

## Driver API接口

### Driver API接口汇总

| API函数                              | 功能描述                | 目标用户层 | 分类       |
| ------------------------------------ | ----------------------- | ---------- | ---------- |
| **芯片初始化与控制**           |                         |            |            |
| `TK8710Init`                       | 初始化TK8710芯片        | 应用层     | 初始化控制 |
| `TK8710Start`                      | 启动TK8710工作          | 应用层     | 初始化控制 |
| `TK8710RfConfig`                   | 初始化RF配置            | 应用层     | 初始化控制 |
| `TK8710Reset`                      | 复位芯片                | 应用层     | 初始化控制 |
| **配置管理**                   |                         |            |            |
| `TK8710SetConfig`                  | 设置芯片配置参数        | 应用层     | 配置管理   |
| `TK8710GetConfig`                  | 获取芯片配置参数        | 应用层     | 配置管理   |
| **数据传输**                   |                         |            |            |
| `TK8710SetTxUserData` | 设置下行数据(广播/用户) | TRM层      | 数据传输   |
| `TK8710SetTxUserInfo`              | 设置发送用户信息        | TRM层      | 数据传输   |
| **数据接收**                   |                         |            |            |
| `TK8710GetRxUserData`              | 获取接收数据            | TRM层      | 数据接收   |
| `TK8710GetRxUserInfo`              | 获取接收用户信息        | TRM层      | 数据接收   |
| `TK8710GetRxUserSignalQuality`     | 获取接收用户信号质量    | TRM层      | 数据接收   |
| `TK8710ReleaseRxData`              | 释放接收数据资源        | TRM层      | 数据接收   |
| **状态查询**                   |                         |            |            |
| `TK8710GetSlotConfig`（GetConfig） | 获取时隙配置            | 应用层     | 状态查询   |
| **回调管理**                   |                         |            |            |
| `TK8710RegisterCallbacks`          | 注册Driver回调函数      | TRM层      | 回调管理   |
| **中断处理**                   |                         |            |            |
| `TK8710GpioInit`                   | 初始化GPIO中断          | 应用层     | 中断处理   |
| `TK8710GpioIrqEnable`              | 使能/禁用GPIO中断       | 应用层     | 中断处理   |
| **日志系统**                   |                         |            |            |
| `TK8710LogSimpleConfig`              | 初始化日志系统          | 应用层     | 日志系统   |

**说明:**

- **应用层**: 直接供应用层开发者使用的API
- **TRM层**: 主要供TRM模块内部使用的API，应用层一般不直接调用
- **初始化控制**: 芯片初始化和基本控制功能
- **配置管理**: 芯片配置参数设置
- **数据传输**: 数据发送相关功能
- **数据接收**: 数据接收相关功能
- **状态查询**: 芯片状态查询功能
- **回调管理**: 中断回调函数管理
- **中断处理**: GPIO中断相关功能
- **日志系统**: 日志输出和管理功能

### 1. 芯片初始化与控制

#### `TK8710Init`

```c
int TK8710Init(const ChipConfig* initConfig);
```

**功能**: 初始化TK8710芯片
**参数**:

- `initConfig`: 初始化配置参数，为NULL时使用默认配置

  ```c
  typedef struct {
      uint32_t bcn_agc;       /* BCN AGC长度, 默认32 */
      uint32_t interval;      /* Intval长度, 默认32 */
      uint8_t  tx_dly;        /* 下行发送时使用前几个上行ram中的信息, 默认1 */
      uint8_t  tx_fix_info;   /* TX是否固定频点, 默认0 */
      int32_t  offset_adj;    /* BCN sync窗口微调, 默认0, 单位us */
      int32_t  tx_pre;        /* 发送数据窗口调整, 默认0, 单位us */
      uint8_t  conti_mode;    /* 连续工作模式使能, 默认1 */
      uint8_t  bcn_scan;      /* BCN SCAN模式使能, 默认0 */
      uint8_t  ant_en;        /* 天线使能, 默认0xFF */
      uint8_t  rf_sel;        /* 射频使能, 默认0xFF */
      uint8_t  tx_bcn_en;     /* 发送BCN使能, 默认1 */
      uint8_t  ts_sync;       /* 本地同步, 默认0 */
      uint8_t  rf_model;      /* 射频芯片型号: 1=SX1255, 2=SX1257 */
      uint8_t  bcnbits;       /* 信标标识位, 共5bit */
      uint32_t anoiseThe1;    /* 用户检测anoiseTh1门限, 默认0 */
      uint32_t power2rssi;    /* RSSI换算 */
      uint32_t irq_ctrl0;     /* 中断使能 */
      uint32_t irq_ctrl1;     /* 中断清理 */
  } ChipConfig;
  ```
- **返回值**: 0-成功, 1-失败, 2-超时

#### `TK8710Start`

```c
int TK8710Start(uint8_t workType, uint8_t workMode);
```

**功能**: 芯片进入收发状态
**参数**:

- `workType`: 工作类型

  ```c
  typedef enum {
      TK8710_WORK_TYPE_MASTER = 1,  /* 主模式 */
      TK8710_WORK_TYPE_SLAVE  = 2,  /* 从模式 */
  } workType_e;
  ```
- `workMode`: 工作模式

  ```c
  typedef enum {
      TK8710_WORK_MODE_CONTINUOUS = 1,  /* 连续工作模式 */
      TK8710_WORK_MODE_SINGLE     = 2,  /* 单次工作模式 */
  } workMode_e;
  ```

  **返回值**: 0-成功, 1-失败, 2-超时

#### `TK8710RfConfig`

```c
int TK8710RfConfig(const ChiprfConfig* initrfConfig);
```

**功能**: 初始化芯片连接射频
**参数**:

- `initrfConfig`: 射频初始化配置参数

  ```c
  typedef struct {
      rfType_e rftype;        /* 射频类型 */
      uint32_t Freq;          /* 射频中心频率 */
      uint8_t  rxgain;        /* 射频接收增益 */
      uint8_t  txgain;        /* 射频发送增益 */
      TxAdcConfig txadc[TK8710_MAX_ANTENNAS]; /* 射频发送直流, 8天线×(i,q)×16bit */
  } ChiprfConfig;

  typedef enum {
      TK8710_RF_TYPE_1255_1M  = 0,  /* SX1255 1MHz */
      TK8710_RF_TYPE_1255_32M = 1,  /* SX1255 32MHz */
      TK8710_RF_TYPE_1257_32M = 2,  /* SX1257 32MHz */
      TK8710_RF_TYPE_OTHER    = 3,  /* 其他类型 */
  } rfType_e;

  typedef struct {
      int16_t i;              /* I路直流, 16bit */
      int16_t q;              /* Q路直流, 16bit */
  } TxAdcConfig;
  ```

  **返回值**: 0-成功, 1-失败, 2-超时

#### `TK8710Reset`

```c
int TK8710Reset(uint8_t rstType);
```

**功能**: 芯片复位
**参数**:

- `rstType`: 复位类型

  ```c
  typedef enum {
      TK8710_RST_STATE_MACHINE = 1,  /* 仅复位状态机 */
      TK8710_RST_ALL           = 2,  /* 复位状态机+寄存器 */
  } rstType_e;
  ```

  **返回值**: 0-成功, 1-失败, 2-超时

### 2. 配置管理

#### `TK8710SetConfig`

```c
int TK8710SetConfig(TK8710ConfigType type, const void* params);
```

**功能**: 设置芯片配置
**参数**:

- `type`: 配置类型

  ```c
  typedef enum {
    TK8710_CFG_TYPE_CHIP_INFO,        /* 芯片信息配置 */
    TK8710_CFG_TYPE_SLOT_CFG,         /* 时隙配置 */
    TK8710_CFG_TYPE_ADDTL,            /* 附加位配置 */
  } TK8710ConfigType;

  ```

#### `TK8710GetConfig`

```c
int TK8710GetConfig(TK8710ConfigType type, void* params);
```

**功能**: 获取芯片配置
**参数**:

- `type`: 配置类型 (同TK8710SetConfig)
- `params`: 配置参数输出指针
  **返回值**: 0-成功, 1-失败, 2-超时

**说明**:

- 获取指定类型的芯片配置参数
- 支持的配置类型与 `TK8710SetConfig`相同
- `params`指针类型需要根据配置类型进行相应的类型转换
- 主要用于读取当前芯片的配置状态
- 常用于配置验证和状态监控

**示例**:

```c
// 获取时隙配置
slotConfig_t slotConfig;
ret = TK8710GetConfig(TK8710_CFG_TYPE_SLOT_CFG, &slotConfig);
if (ret == TK8710_OK) {
    printf("当前时隙配置: antEn=0x%02X, rfSel=0x%02X\n", 
           slotConfig.antEn, slotConfig.rfSel);
}
```

### 3. 数据传输

#### `TK8710SetTxUserData`

```c
int TK8710SetTxUserData(TK8710DownlinkType downlinkType, uint8_t index, const uint8_t* data, uint16_t dataLen, uint8_t txPower, uint8_t beamType);
```

**功能**: 设置下行发送数据和功率
**参数**:

- `downlinkType`: 下行类型 (0=下行1数据, 1=下行2数据)
- `index`: 索引 (下行1时范围0-15, 下行2时范围0-127)
- `data`: 数据指针
- `dataLen`: 数据长度
- `txPower`: 发送功率
- `beamType`: 波束类型 (0=广播波束, 1=指定波束)
  **返回值**: 0-成功, 1-失败, 2-超时

**说明**:

- 统一的下行数据发送接口，支持广播和专用数据
- 下行1用于广播数据，最大支持16个用户
- 下行2用于专用数据，最大支持128个用户
- 自动管理内存分配和数据复制
- 支持不同的波束类型配置

#### `TK8710SetTxUserInfo`

```c
int TK8710SetTxUserInfo(uint8_t userBufferIdx, uint32_t freqInfo, const uint32_t* ahInfo, uint64_t pilotPowerInfo);
```

**功能**: 设置发送用户信息
**参数**:

- `userBufferIdx`: 用户索引 (0-127)
- `freqInfo`: 频率信息，需要按照一定规则转换才能解读
- `ahInfo`: AH信息
- `pilotPowerInfo`: PilotPower信息
  **返回值**: 0-成功, 1-失败, 2-超时

### 4. 数据接收

#### `TK8710GetRxUserData`

```c
int TK8710GetRxUserData(uint8_t userIndex, uint8_t** data, uint16_t* dataLen);
```

**功能**: 获取接收数据
**参数**:

- `userIndex`: 用户索引 (0-127)
- `data`: 数据指针输出
- `dataLen`: 数据长度输出
  **返回值**: 0-成功, 1-失败, 2-超时

#### `TK8710GetRxUserInfo`

```c
int TK8710GetRxUserInfo(uint8_t userBufferIdx, uint32_t* freqInfo, uint32_t* ahInfo, uint64_t* pilotPowerInfo);
```

**功能**: 获取接收用户信息 (从MD_UD中断获取的数据)
**参数**

- `userBufferIdx`: 用户索引 (0-127)
- `freqInfo`: 频率信息，需要按照一定规则转换才能解读
- `ahInfo`: AH信息
- `pilotPowerInfo`: PilotPower信息

**
    返回值**: 0-成功, 1-失败, 2-超时

**说明**:

- 从MD_UD中断获取的用户波束信息中提取详细参数
- `ahInfo`数组包含16个32位的AH配置数据
- `pilotPowerInfo`为导频功率值，用于波束管理
- TRM层使用此接口获取用户信息进行波束跟踪和管理
- 如果获取失败，TRM层会使用默认值进行处理

#### `TK8710GetRxUserSignalQuality`

```c
int TK8710GetRxUserSignalQuality(uint8_t userIndex, uint32_t* rssi, uint8_t* snr, uint32_t* freq);
```

**功能**: 获取接收用户信号质量信息
**参数**:

- `userIndex`: 用户索引 (0-127)
- `rssi`: RSSI值输出 (dBm)
- `snr`: SNR值输出
- `freq`: 频率值输出 (Hz)
  **返回值**: 0-成功, 1-失败, 2-超时

#### `TK8710ReleaseRxData`

```c
void TK8710ReleaseRxData(uint8_t userIndex);
```

**功能**: 释放接收数据缓冲区
**参数**:

- `userIndex`: 用户索引 (0-127)

### 5. 状态查询

#### `TK8710GetSlotCfg`

```c
const slotCfg_t* TK8710GetSlotCfg(void);
```

**功能**: 获取时隙配置
**返回值**: 时隙配置指针

```c
  typedef struct {
      msMode_e   msMode;          /* 主从模式 */
      uint8_t    plCrcEn;         /* Payload CRC使能 */
      uint8_t    rateCount;       /* 速率个数，范围1~4，支持最多4个速率模式 */
      rateMode_e rateModes[4];    /* 各时隙速率模式数组，对应s0Cfg-s3Cfg */
      uint8_t    brdUserNum;      /* 广播时隙用户数, 范围1~16 */
      float      brdFreq[TK8710_MAX_BRD_USERS]; /*广播时隙发送用户的频率*/
      uint8_t    antEn;           /* 天线使能, 默认0xFF(8天线) */
      uint8_t    rfSel;           /* RF天线选择, 默认0xFF(8天线) */
      uint8_t    txAutoMode;      /* 下行发送模式: 0=自动发送, 1=指定信息发送 */
      uint8_t    txBcnEn;         /* BCN发送使能 */
      uint8_t    bcnRotation[TK8710_MAX_ANTENNAS];  /* BCN发送使能为0xff时，从bcnRotation中轮流获取当前发送bcn天线*/
      uint32_t   rx_delay;        /* RX delay, 默认0 */
      uint32_t   md_agc;          /* DATA AGC长度, 默认1024 */
      uint8_t    local_sync;      /* 本地同步: 1=产生本地同步信号, 0=接收外部同步脉冲 */
  
      SlotConfig s0Cfg[4];           /* 时隙0(BCN)配置 */
      SlotConfig s1Cfg[4];           /* 时隙1配置 */
      SlotConfig s2Cfg[4];           /* 时隙2配置 */
      SlotConfig s3Cfg[4];           /* 时隙3配置 */
      uint32_t   frameTimeLen;    /* TDD周期总时间长度, 单位us */
  } slotCfg_t;
  
  typedef struct {
      uint16_t byteLen;       /* 时隙字节数 */
      uint32_t timeLen;       /* 时隙时间长度, 单位us (仅查询有效) */
      uint32_t centerFreq;    /* 时隙中心频点 */
      uint32_t FreqOffset;    /*相较于中心频点的偏移*/
      uint32_t da_m;          /* 内部DAC参数，用于配置时隙末尾的空闲长度 */
  } SlotConfig;
  
  typedef enum {
      TK8710_MODE_SLAVE  = 0,  /* 从模式 */
      TK8710_MODE_MASTER = 1,  /* 主模式 */
  } msMode_e;
  
  typedef enum {
      TK8710_RATE_MODE_5  = 5,   /* 速率模式5: 2KHz, 62.5KHz系统带宽, 128用户 */
      TK8710_RATE_MODE_6  = 6,   /* 速率模式6: 4KHz, 125KHz系统带宽, 128用户 */
      TK8710_RATE_MODE_7  = 7,   /* 速率模式7: 8KHz, 250KHz系统带宽, 128用户 */
      TK8710_RATE_MODE_8  = 8,   /* 速率模式8: 16KHz, 500KHz系统带宽, 128用户 */
      TK8710_RATE_MODE_9  = 9,   /* 速率模式9: 32KHz, 500KHz系统带宽, 64用户 */
      TK8710_RATE_MODE_10 = 10,  /* 速率模式10: 64KHz, 500KHz系统带宽, 32用户 */
      TK8710_RATE_MODE_11 = 11,  /* 速率模式11: 128KHz, 500KHz系统带宽, 16用户 */
      TK8710_RATE_MODE_18 = 18,  /* 速率模式18: 128KHz, 500KHz系统带宽, 16用户 */
  } rateMode_e;
```

**说明**: TRM和其他业务模块通过此接口获取完整的slot配置信息，包括速率模式配置等

### 6. Driver上层回调接口

Driver层提供完整的多回调架构，支持为不同中断类型注册专用回调函数，提供灵活的事件处理机制。

#### `TK8710RegisterCallbacks`

```c
void TK8710RegisterCallbacks(const TK8710DriverCallbacks* callbacks);
```

**功能**: 注册Driver回调函数
**参数**:

- `callbacks`: 回调函数结构体指针

```c
typedef void (*TK8710RxDataCallback)(TK8710IrqResult* irqResult);
typedef void (*TK8710TxSlotCallback)(TK8710IrqResult* irqResult);
typedef void (*TK8710SlotEndCallback)(TK8710IrqResult* irqResult);
typedef void (*TK8710ErrorCallback)(TK8710IrqResult* irqResult);

typedef struct {
    TK8710RxDataCallback onRxData;      /* 接收数据回调 */
    TK8710TxSlotCallback onTxSlot;      /* 发送时隙回调 */
    TK8710SlotEndCallback onSlotEnd;    /* 时隙结束回调 */
    TK8710ErrorCallback onError;        /* 错误回调 */
} TK8710DriverCallbacks;
```

**回调函数说明**:

- `onRxData`: 当接收到MD_DATA中断时调用，对应数据接收事件
- `onTxSlot`: 当S1时隙发送完成时调用，对应数据发送结束事件
- `onSlotEnd`: 当时隙结束时调用，对应时隙结束事件
- `onError`: 当发生错误中断时调用

**说明**:

- 替代了原有的单一回调接口 `TK8710IrqInit`
- 支持为不同中断类型注册专用回调函数
- 所有回调都使用统一的 `TK8710IrqResult*` 参数
- 提供更灵活的中断处理机制
- `TK8710_IRQ_MD_UD` 中断类型已定义但暂未实现具体回调处理

### 7. 中断处理

#### `TK8710GpioInit`

```c
int TK8710GpioInit(int pin, TK8710GpioEdge edge, TK8710GpioIrqCallback cb, void* user);
```

**功能**: 初始化GPIO中断
**参数**:

- `pin`: GPIO引脚号
- `edge`: 中断触发边沿
  ```c
  typedef enum {
      TK8710_GPIO_EDGE_RISING = 0,   /* 上升沿触发 */
      TK8710_GPIO_EDGE_FALLING = 1,  /* 下降沿触发 */
      TK8710_GPIO_EDGE_BOTH = 2,      /* 双边沿触发 */
  } TK8710GpioEdge;
  ```
- `cb`: GPIO中断回调函数
- `user`: 用户数据指针
- `userData`: 用户数据指针
  **返回值**: 0-成功, 1-失败, 2-超时

```c
typedef void (*TK8710GpioIrqCallback)(void* user);
```

**说明**: GPIO中断使用独立的回调类型，与Driver回调分离

#### `TK8710GpioIrqEnable`

```c
int TK8710GpioIrqEnable(uint8_t gpioPin, uint8_t enable);
```

**功能**: 使能GPIO中断
**参数**:

- `gpioPin`: GPIO引脚号
- `enable`: 使能标志 (1=使能, 0=禁用)
  **返回值**: 0-成功, 其他-失败

### 8. 日志系统

#### `TK8710LogSimpleInit`（config）

```c
int TK8710LogSimpleInit(TK8710LogLevel level, uint32_t module_mask);
```

**功能**: 简化初始化日志系统
**参数**:

- `level`: 日志级别

  ```c
  typedef enum {
      TK8710_LOG_ERROR = 0,  /* 错误级别 */
      TK8710_LOG_WARN  = 1,  /* 警告级别 */
      TK8710_LOG_INFO  = 2,  /* 信息级别 */
      TK8710_LOG_DEBUG = 3,  /* 调试级别 */
      TK8710_LOG_TRACE = 4,  /* 跟踪级别 */
      TK8710_LOG_ALL   = 5   /* 所有级别 */
  } TK8710LogLevel;
  ```
- `module_mask`: 模块掩码

  ```c
  typedef enum {
      TK8710_LOG_MODULE_CORE     = 0x01,  /* 核心模块 */
      TK8710_LOG_MODULE_CONFIG   = 0x02,  /* 配置模块 */
      TK8710_LOG_MODULE_IRQ      = 0x04,  /* 中断模块 */
      TK8710_LOG_MODULE_HAL      = 0x08,  /* HAL层模块 */
      TK8710_LOG_MODULE_ALL      = 0xFF   /* 所有模块 */
  } TK8710LogModule;
  ```

  **返回值**: 0-成功, 1-失败

### 9. 调试接口

#### `TK8710GetIrqTimeStats`

```c
int TK8710GetIrqTimeStats(uint8_t irqType, uint32_t* totalTime, 
                          uint32_t* maxTime, uint32_t* minTime, uint32_t* count);
```

**功能**: 获取中断处理时间统计
**参数**:

- `irqType`: 中断类型 (0-9)
- `totalTime`: 总处理时间 (输出参数)
- `maxTime`: 最大处理时间 (输出参数)
- `minTime`: 最小处理时间 (输出参数)
- `count`: 中断次数 (输出参数)
  **返回值**: 0-成功, 1-失败

#### `TK8710ResetIrqTimeStats`

```c
int TK8710ResetIrqTimeStats(uint8_t irqType);
```

**功能**: 重置中断处理时间统计
**参数**:

- `irqType`: 中断类型 (0-9)，255表示重置所有
  **返回值**: 0-成功, 1-失败

#### `TK8710PrintIrqTimeStats`

```c
void TK8710PrintIrqTimeStats(void);
```

**功能**: 打印中断处理时间统计报告

#### `TK8710SetForceProcessAllUsers`

```c
void TK8710SetForceProcessAllUsers(uint8_t enable);
```

**功能**: 设置是否强制处理所有用户数据（测试接口）
**参数**:

- `enable`: 1-强制处理所有用户数据，0-按CRC结果正常处理
  **说明**: 此函数仅用于测试中断处理能力

#### `TK8710GetForceProcessAllUsers`

```c
uint8_t TK8710GetForceProcessAllUsers(void);
```

**功能**: 获取当前是否强制处理所有用户数据
**返回值**: 1-强制处理，0-正常处理

#### `TK8710SetForceMaxUsersTx`

```c
void TK8710SetForceMaxUsersTx(uint8_t enable);
```

**功能**: 设置是否强制按最大用户数发送（测试接口）
**参数**:

- `enable`: 1-强制按最大用户数发送，0-按实际输入用户数发送
  **说明**: 此函数仅用于测试中断处理能力

#### `TK8710GetForceMaxUsersTx`

```c
uint8_t TK8710GetForceMaxUsersTx(void);
```

**功能**: 获取当前是否强制按最大用户数发送
**返回值**: 1-强制发送，0-正常发送

#### `TK8710DebugCtrl`

```c
int TK8710DebugCtrl(TK8710DebugCtrlType ctrlType, CtrlOptType optType,
                    const void* inputParams, void* outputParams);
```

**功能**: 调试控制接口
**参数**:

- `ctrlType`: 控制类型
  ```c
  typedef enum {
      TK8710_DBG_TYPE_FFT_OUT,
      TK8710_DBG_TYPE_CAPTURE_DATA,
      TK8710_DBG_TYPE_ACM_CAL_FACTOR,
      TK8710_DBG_TYPE_ACM_SNR,
      TK8710_DBG_TYPE_TX_TONE,
  } TK8710DebugCtrlType;
  ```
- `optType`: 操作类型: 0=Set, 1=Get, 2=Exe
- `inputParams`: 输入参数指针
- `outputParams`: 输出参数指针
  **返回值**: 0-成功, 1-失败, 2-超时

---

### 10. Driver API典型工作流程

Driver API的使用遵循标准的硬件操作流程。以下是精简的典型工作流程，展示了所有Driver API接口的使用顺序。

#### 10.1 GPIO中断初始化配置

**第一步：配置GPIO中断系统**

```c
// GPIO中断处理函数
void GpioIrqHandler(void* user) {
    printf("GPIO interrupt triggered\n");
    // GPIO中断处理
}

// 初始化GPIO中断
TK8710GpioInit(0, TK8710_GPIO_EDGE_RISING, GpioIrqHandler, NULL);
TK8710GpioIrqEnable(0, 1);
```

#### 10.2 TK8710初始化与日志系统

**第二步：芯片初始化和日志配置**

```c
// 1. 芯片基础初始化
ChipConfig chipConfig = {0};
chipConfig.interval = 32;           // 时隙间隔
chipConfig.ant_en = 0xFF;           // 天线使能
chipConfig.conti_mode = 1;          // 连续工作模式
chipConfig.tx_bcn_en = 1;           // 发送BCN使能

int ret = TK8710Init(&chipConfig);
if (ret != TK8710_OK) {
    printf("Chip initialization failed: %d\n", ret);
    return ret;
}

// 2. 射频子系统初始化
ChiprfConfig rfConfig = {0};
rfConfig.rftype = RF_TYPE_SX1257;   // 射频芯片类型
rfConfig.Freq = 2400000000;         // 中心频率 2.4GHz
rfConfig.rxgain = 1;                // 接收增益
rfConfig.txgain = 1;                // 发送增益

ret = TK8710RfConfig(&rfConfig);
if (ret != TK8710_OK) {
    printf("RF initialization failed: %d\n", ret);
    return ret;
}

// 3. 日志系统初始化
TK8710LogSimpleInit(TK8710_LOG_INFO, TK8710_LOG_MODULE_ALL);
```

#### 10.3 注册Driver回调

**第三步：注册Driver回调函数**

```c
// 1. 定义回调函数
void OnRxData(TK8710IrqResult* irqResult) {
    printf("Received MD_DATA interrupt\n");
    // 处理接收数据...
}

void OnTxSlot(TK8710IrqResult* irqResult) {
    printf("S1 slot transmission completed\n");
    // 处理发送完成...
}

void OnError(TK8710IrqResult* irqResult) {
    printf("Driver error occurred\n");
    // 错误处理...
}

// 2. 注册回调
TK8710DriverCallbacks callbacks = {
    .onRxData = OnRxData,
    .onTxSlot = OnTxSlot,
    .onSlotEnd = NULL,  // 可选
    .onError = OnError
};

TK8710RegisterCallbacks(&callbacks);
```

#### 10.4 TK8710配置与启动工作

**第四步：时隙配置和启动工作模式**

```c
// 1. 获取当前时隙配置（可选）
const slotCfg_t* slotCfg = TK8710GetSlotCfg();
printf("Current slot configuration: rate=%d, users=%d\n", 
       slotCfg->rateMode, slotCfg->maxUsers);

// 2. 配置时隙参数
// 注意：具体的时隙配置需要根据实际需求定义slotConfig结构体
// 这里仅作为示例，实际使用时需要提供正确的时隙配置参数
ret = TK8710SetConfig(TK8710_CFG_TYPE_SLOT_CFG, &slotConfig);

// 3. 启动芯片进入收发状态
ret = TK8710Start(1, 1);  // Master模式，连续工作
if (ret != TK8710_OK) {
    printf("Start work failed: %d\n", ret);
    return ret;
}
```

#### 10.5 数据接收操作

**第五步：在中断回调中处理接收数据**

```c
void OnRxData(TK8710IrqResult* irqResult) {
    // 1. 遍历所有有效用户
    for (uint8_t i = 0; i < 128; i++) {
        if (irqResult->crcResults[i].crcValid) {
            // 2. 获取用户数据
            uint8_t* userData;
            uint16_t dataLen;
            ret = TK8710GetRxUserData(i, &userData, &dataLen);
            if (ret == TK8710_OK) {
                // 3. 处理数据
                printf("User[%d] received %d bytes\n", i, dataLen);
  
                // 4. 获取信号质量信息
                uint32_t rssi, freq;
                uint8_t snr;
                TK8710GetRxUserSignalQuality(i, &rssi, &snr, &freq);
  
                // 5. 获取用户波束信息（TRM层使用）
                uint32_t userFreq;
                uint32_t ahInfo[16];
                uint64_t pilotPowerInfo;
                TK8710GetRxUserInfo(i, &userFreq, ahInfo, &pilotPowerInfo);
  
                // 6. 释放数据缓冲区
                TK8710ReleaseRxData(i);
            }
        }
    }
}
```

#### 10.6 数据发送操作

**第六步：配置发送数据**

```c
// 1. 设置下行用户数据
uint8_t userData[32] = {0x01, 0x02, 0x03, ...};
ret = TK8710SetDownlink2Data(userIndex, userData, sizeof(userData), 35, TK8710_USER_DATA_TYPE_NORMAL);
if (ret != TK8710_OK) {
    printf("Set downlink data failed: %d\n", ret);
}

// 2. 设置发送用户信息
uint32_t freqInfo = 2400000000;
uint32_t ahInfo[16] = {0};  // AH配置数据
uint64_t pilotPowerInfo = 20;
ret = TK8710SetTxUserInfo(userIndex, freqInfo, ahInfo, pilotPowerInfo);
if (ret != TK8710_OK) {
    printf("Set TX user info failed: %d\n", ret);
}

// 3. 设置广播数据（可选）
uint8_t brdData[26] = {0xFF, 0xFE, 0xFD, ...};
ret = TK8710SetBrdData(0, brdData, sizeof(brdData), 35, TK8710_BRD_DATA_TYPE_NORMAL);
if (ret != TK8710_OK) {
    printf("Set broadcast data failed: %d\n", ret);
}
```

#### 10.7 系统控制与维护

**第七步：系统状态监控和维护**

```c
// 1. 获取芯片信息
TK8710ChipInfo chipInfo;
ret = TK8710GetChipInfo(&chipInfo);
if (ret == TK8710_OK) {
    printf("Chip ID: 0x%08X, Version: %d.%d\n", 
           chipInfo.chipId, chipInfo.versionMajor, chipInfo.versionMinor);
}

// 2. 获取工作状态
uint8_t workState;
ret = TK8710GetWorkState(&workState);
if (ret == TK8710_OK) {
    printf("Work state: %d\n", workState);
}

// 3. 错误恢复与重置（必要时）
ret = TK8710ResetChip(TK8710_RST_ALL);
if (ret != TK8710_OK) {
    printf("Reset failed: %d\n", ret);
}

// 4. 重新初始化（必要时）
ret = TK8710Init(&chipConfig);
ret = TK8710RfConfig(&rfConfig);
```

#### 10.8 工作流程总结

**7个主要步骤**：

1. **GPIO中断初始化配置** - `TK8710GpioInit()`, `TK8710GpioIrqEnable()`
2. **TK8710初始化与日志系统** - `TK8710Init()`, `TK8710RfConfig()`, `TK8710LogSimpleInit()`
3. **注册Driver回调** - `TK8710RegisterCallbacks()`
4. **TK8710配置与启动工作** - `TK8710GetSlotCfg()`, `TK8710SetConfig()`, `TK8710GetConfig()`, `TK8710Start()`
5. **数据接收操作** - `TK8710GetRxUserData()`, `TK8710GetRxUserSignalQuality()`, `TK8710GetRxUserInfo()`, `TK8710ReleaseRxData()`
6. **数据发送操作** - `TK8710SetDownlink2Data()`, `TK8710SetTxUserInfo()`, `TK8710SetBrdData()`
7. **系统控制与维护** - `TK8710GetChipInfo()`, `TK8710GetWorkState()`, `TK8710ResetChip()`

**最佳实践**：

- **初始化顺序**: GPIO中断 → 芯片初始化 → 回调注册 → 配置启动
- **错误处理**: 每个API调用都应检查返回值
- **资源管理**: 及时释放接收数据缓冲区
- **系统监控**: 定期检查芯片状态和工作状态

---

## TRM API接口

### TRM API接口汇总

| API函数                         | 功能描述               | 目标用户层 | 分类       |
| ------------------------------- | ---------------------- | ---------- | ---------- |
| **系统初始化与控制**      |                        |            |            |
| `TRM_Init`                    | 初始化TRM系统          | 应用层     | 初始化控制 |
| `TRM_Start`                   | 启动TRM系统            | 应用层     | 初始化控制 |
| `TRM_Stop`                    | 停止TRM系统            | 应用层     | 初始化控制 |
| `TRM_Deinit`                  | 清理TRM系统资源        | 应用层     | 初始化控制 |
| **数据发送**              |                        |            |            |
| `TRM_SendData`                | 发送用户数据           | 应用层     | 数据发送   |
| `TRM_SendBroadcast`           | 发送广播数据           | 应用层     | 数据发送   |
| **波束获取**              |                        |            |            |
| `TRM_GetBeamInfo`             | 获取用户波束信息       | 应用层     | 波束获取   |
| **回调接口**              |                        |            |            |
| `TRM_OnRxData`                | 接收数据回调函数类型   | 应用层     | 回调接口   |
| `TRM_OnTxComplete`            | 发送完成回调函数类型   | 应用层     | 回调接口   |
| **状态查询**              |                        |            |            |
| `TRM_IsRunning`               | 获取TRM运行状态        | 应用层     | 状态查询   |
| `TRM_GetStats`                | 获取TRM统计信息        | 应用层     | 状态查询   |
| `TRM_GetCurrentFrame`         | 获取当前帧号           | 应用层     | 状态查询   |
| `TRM_SetCurrentFrame`         | 设置当前帧号           | 应用层     | 状态查询   |
| `TRM_SetMaxFrameCount`        | 设置最大帧数           | 应用层     | 状态查询   |
| **回调函数管理**          |                        |            |            |
| `TRM_RegisterDriverCallbacks` | 注册Driver回调函数     | Driver层   | 回调管理   |
| **日志系统**              |                        |            |            |
| `TRM_LogInit`                 | 初始化TRM日志系统      | 应用层     | 日志系统   |
| `TRM_LogSetLevel`             | 设置TRM日志级别        | 应用层     | 日志系统   |
| `TRM_LOG_*` 宏                | TRM日志输出宏          | 应用层     | 日志系统   |
| **调试接口**              |                        |            |            |
| `TRM_TxValidatorOnRxData`     | 发送验证器接收数据处理 | Driver层   | 调试接口   |

**说明:**

- **应用层**: 直接供应用层开发者使用的API
- **Driver层**: 主要供Driver层内部使用的API，应用层一般不直接调用
- **初始化控制**: TRM系统初始化和基本控制功能
- **数据发送**: 数据发送相关功能
- **波束获取**: 波束信息获取功能
- **回调接口**: 应用层回调函数类型定义
- **状态查询**: TRM状态查询功能
- **回调管理**: Driver回调函数管理
- **日志系统**: TRM日志输出和管理功能
- **调试接口**: TRM调试和测试功能

---

### 1. 系统初始化与控制

#### `TRM_Init`

```c
int TRM_Init(const TRM_InitConfig* config);
```

**功能**: 初始化TRM系统
**参数**:

- `config`: TRM初始化配置指针，包含波束配置、回调函数等设置

**TRM_InitConfig结构体定义**:

```c
typedef struct {
    /* 波束配置 */
    TRM_BeamMode beamMode;          /* 波束存储模式 */
    uint32_t     beamMaxUsers;      /* 最大用户数，0表示使用默认值(3000) */
    uint32_t     beamTimeoutMs;     /* 波束超时时间(毫秒)，0表示使用默认值(3000ms) */
  
    /* 回调函数 */
    TRM_Callbacks callbacks;
  
    /* 平台配置(预留) */
    void* platformConfig;
} TRM_InitConfig;
```

**结构体成员详细说明**:

- `beamMode`: 波束存储模式
  - `TRM_BEAM_MODE_FULL_STORE`: 完整波束存储模式(CPU RAM)
  - `TRM_BEAM_MODE_MAPPING`: 波束映射模式(8710 RAM)
- `beamMaxUsers`: 最大用户数，0表示使用默认值(3000)
- `beamTimeoutMs`: 波束超时时间(毫秒)，0表示使用默认值(3000ms)
- `callbacks`: 回调函数集合，详见TRM_Callbacks结构体
- `platformConfig`: 平台配置指针(预留)

**TRM_Callbacks结构体定义**:

```c
typedef struct {
    TRM_OnRxData      onRxData;      /* 接收数据回调函数指针 */
    TRM_OnTxComplete  onTxComplete;  /* 发送完成回调函数指针 */
} TRM_Callbacks;
```

**回调函数类型定义**:

```c
/* 接收数据回调 - 上层必须同步处理数据 */
typedef void (*TRM_OnRxData)(const TRM_RxDataList* rxDataList);

/* 发送完成回调 */
typedef void (*TRM_OnTxComplete)(uint32_t userId, TRM_TxResult result);
```

**返回值**:

- `TRM_OK`: 初始化成功
- `TRM_ERR_PARAM`: 参数错误
- `TRM_ERR_NO_MEM`: 内存不足
- 其他: 初始化失败

#### `TRM_Start`

```c
int TRM_Start(void);
```

**功能**: 启动TRM系统，开始处理数据发送和接收
**返回值**:

- `TRM_OK`: 启动成功
- `TRM_ERR_STATE`: 状态错误(未初始化或已启动)
- 其他: 启动失败

#### `TRM_Stop`

```c
int TRM_Stop(void);
```

**功能**: 停止TRM系统，停止数据处理
**返回值**:

- `TRM_OK`: 停止成功
- `TRM_ERR_STATE`: 状态错误(未启动)
- 其他: 停止失败

#### `TRM_Deinit`

```c
void TRM_Deinit(void);
```

**功能**: 清理TRM系统，释放所有资源

### 2. 数据发送

#### `TRM_SendData`

```c
int TRM_SendData(uint32_t userId, const uint8_t* data, uint16_t len, uint8_t txPower, uint32_t frameNo, uint8_t targetRateMode, uint8_t dataType);
```

**功能**: 发送用户数据(支持单速率和多速率模式)
**参数**:

- `userId`: 用户ID，32位唯一标识符
  - 范围: 0x00000000 - 0xFFFFFFFF
  - 0xFFFFFFFF: 保留值，用于特殊操作
- `data`: 数据指针，指向要发送的数据缓冲区
  - 不能为NULL
  - 数据长度不超过512字节
- `len`: 数据长度，字节数
  - 范围: 1 - 512
  - 必须小于等于实际数据缓冲区大小
- `txPower`: 发射功率
  - 范围: 0 - 31
  - 0: 最小功率
  - 31: 最大功率
- `frameNo`: 目标发送帧号
  - 范围: 0 - g_trmMaxFrameCount-1
  - 当targetRateMode=0时使用此参数进行帧号匹配
- `targetRateMode`: 目标速率模式
  - 0: 使用帧号匹配模式
  - 5-11: 使用速率模式匹配(对应不同速率)
  - 18: 使用速率模式匹配(特殊速率)
- `dataType`: 数据类型
  - `TK8710_USER_DATA_TYPE_NORMAL`: 正常用户数据
  - `TK8710_USER_DATA_TYPE_SLOT3`: 与Slot3共用数据
    **返回值**:
- `TRM_OK`: 发送成功(数据已入队)
- `TRM_ERR_PARAM`: 参数错误
- `TRM_ERR_QUEUE_FULL`: 发送队列已满
- `TRM_ERR_NOT_INIT`: TRM未初始化
- 其他: 发送失败

#### `TRM_SendBroadcast`

```c
int TRM_SendBroadcast(uint8_t brdIndex, const uint8_t* data, uint16_t len, uint8_t txPower, uint8_t dataType);
```

**功能**: 发送广播数据
**参数**:

- `brdIndex`: 广播索引
  - 范围: 0 - 15
  - 对应不同的广播用户
- `data`: 数据指针，指向要发送的广播数据缓冲区
  - 不能为NULL
  - 数据长度不超过512字节
- `len`: 数据长度，字节数
  - 范围: 1 - 512
  - 必须小于等于实际数据缓冲区大小
- `txPower`: 发射功率
  - 范围: 0 - 31
  - 0: 最小功率
  - 31: 最大功率
- `dataType`: 数据类型
  - `TK8710_BRD_DATA_TYPE_NORMAL`: 正常广播数据
  - `TK8710_BRD_DATA_TYPE_SLOT3`: 与Slot3共用波束信息
    **返回值**:
- `TRM_OK`: 发送成功
- `TRM_ERR_PARAM`: 参数错误
- `TRM_ERR_NOT_INIT`: TRM未初始化
- 其他: 发送失败

#### `TRM_ClearTxData`

```c
int TRM_ClearTxData(uint32_t userId);
```

**功能**: 清除发送数据
**参数**:

- `userId`: 用户ID
  - 0xFFFFFFFF: 清除所有用户的发送数据
  - 其他值: 清除指定用户的发送数据
    **返回值**:
- `TRM_OK`: 清除成功
- `TRM_ERR_PARAM`: 参数错误
- 其他: 清除失败

### 3. 波束获取

#### `TRM_GetBeamInfo`

```c
int TRM_GetBeamInfo(uint32_t userId, TRM_BeamInfo* beamInfo);
```

**功能**: 获取用户波束信息
**参数**:

- `userId`: 用户ID
  - 范围: 0x00000000 - 0xFFFFFFFF
- `beamInfo`: 波束信息输出指针
  - 函数会填充此结构体
    **返回值**:
- `TRM_OK`: 获取成功
- `TRM_ERR_PARAM`: 参数错误
- `TRM_ERR_NO_BEAM`: 未找到波束信息
- 其他: 获取失败

### 5. TRM上层回调接口

TRM提供完整的上层回调接口，用于通知应用层各种事件，包括数据接收和发送完成。

#### `TRM_OnRxData`

```c
typedef void (*TRM_OnRxData)(const TRM_RxDataList* rxDataList);
```

**功能**: 接收数据回调函数
**参数**:

- `rxDataList`: 接收数据列表指针

```c
typedef struct {
    uint8_t  slotIndex;         /* 时隙索引 */
    uint8_t  userCount;         /* 用户数量 */
    uint16_t reserved;
    uint32_t frameNo;           /* 帧号 */
    TRM_RxUserData* users;      /* 用户数据数组 */
} TRM_RxDataList;
```

**说明**:

- 当接收到用户数据时调用
- 应用层需要同步处理数据，避免阻塞中断处理
- `rxDataList->users` 包含所有接收到的用户数据
- 处理完成后需要及时释放相关资源

#### `TRM_OnTxComplete`(返回队列剩余资源)

```c
typedef void (*TRM_OnTxComplete)(uint32_t userId, TRM_TxResult result);
```

**功能**: 发送完成回调函数
**参数**:

- `userId`: 用户ID
- `result`: 发送结果

```c
typedef enum {
    TRM_TX_OK = 0,              /* 发送成功 */
    TRM_TX_ERR_TIMEOUT,         /* 发送超时 */
    TRM_TX_ERR_RETRY,           /* 发送重试 */
    TRM_TX_ERR_PARAM,           /* 参数错误 */
    TRM_TX_ERR_BUSY,            /* 发送忙 */
    TRM_TX_ERR_FAIL,            /* 发送失败 */
} TRM_TxResult;
```

**说明**:

- 当数据发送完成时调用，通知发送结果
- `userId` 标识发送的用户
- `result` 指示发送是否成功及失败原因

### 6. 状态查询

#### `TRM_IsRunning`

```c
int TRM_IsRunning(void);
```

**功能**: 检查TRM是否运行中
**返回值**:

- 1: 运行中
- 0: 未运行

#### `TRM_GetStats`

```c
int TRM_GetStats(TRM_Stats* stats);
```

**功能**: 获取统计信息
**参数**:

- `stats`: 统计信息输出指针

**TRM_Stats结构体定义**:

```c
typedef struct {
    uint32_t txCount;           /* 发送次数 */
    uint32_t txSuccessCount;    /* 发送成功次数 */
    uint32_t rxCount;           /* 接收次数 */
    uint32_t beamCount;         /* 当前波束数量 */
    uint32_t memAllocCount;     /* 内存分配次数 */
    uint32_t memFreeCount;      /* 内存释放次数 */
} TRM_Stats;
```

**结构体成员详细说明**:

- `txCount`: 发送次数
  - 累计的数据发送尝试次数
- `txSuccessCount`: 发送成功次数
  - 累计的数据发送成功次数
- `rxCount`: 接收次数
  - 累计的数据接收次数
- `beamCount`: 当前波束数量
  - 当前系统中存储的波束信息数量
- `memAllocCount`: 内存分配次数
  - 累计的内存分配操作次数
- `memFreeCount`: 内存释放次数
  - 累计的内存释放操作次数
    **返回值**:
- `TRM_OK`: 获取成功
- `TRM_ERR_PARAM`: 参数错误
- 其他: 获取失败

#### `TRM_GetCurrentFrame`

```c
uint32_t TRM_GetCurrentFrame(void);
```

**功能**: 获取当前帧号
**返回值**: 当前帧号

- 范围: 0 - g_trmMaxFrameCount-1

#### `TRM_SetCurrentFrame`

```c
void TRM_SetCurrentFrame(uint32_t frameNo);
```

**功能**: 设置当前帧号
**参数**:

- `frameNo`: 帧号
  - 范围: 0 - g_trmMaxFrameCount-1

#### `TRM_SetMaxFrameCount`

```c
void TRM_SetMaxFrameCount(uint32_t maxCount);
```

**功能**: 设置最大帧数（超帧大小）
**参数**:

- `maxCount`: 最大帧数
  - 范围: 1 - 10000
  - 默认值: 1000

### 7. 回调函数管理

#### `TRM_RegisterDriverCallbacks`

```c
int TRM_RegisterDriverCallbacks(void);
```

**功能**: 注册TRM到Driver的回调函数
**返回值**:

- `TRM_OK`: 注册成功
- `TRM_ERR_NOT_INIT`: TRM未初始化
- 其他: 注册失败

**说明**: 此函数将TRM的回调适配函数注册到Driver的回调系统中

### 8. TRM日志系统

#### `TRM_LogInit`

```c
void TRM_LogInit(TRMLogLevel level);
```

**功能**: 初始化TRM日志系统
**参数**:

- `level`: 日志级别
  ```c
  typedef enum {
      TRM_LOG_NONE = 0,    /* 无日志 */
      TRM_LOG_ERROR = 1,   /* 错误 */
      TRM_LOG_WARN = 2,    /* 警告 */
      TRM_LOG_INFO = 3,    /* 信息 */
      TRM_LOG_DEBUG = 4,   /* 调试 */
      TRM_LOG_TRACE = 5    /* 跟踪 */
  } TRMLogLevel;
  ```

**说明**: 初始化TRM独立日志系统，设置默认日志级别

#### `TRM_LogSetLevel`

```c
void TRM_LogSetLevel(TRMLogLevel level);
```

**功能**: 设置TRM日志级别
**参数**:

- `level`: 日志级别 (同TRM_LogInit)
  **说明**: 动态调整日志输出级别，只有等于或低于此级别的日志才会输出

#### `TRM_LOG_*` 宏

```c
TRM_LOG_ERROR(fmt, ...);   // 错误日志
TRM_LOG_WARN(fmt, ...);    // 警告日志
TRM_LOG_INFO(fmt, ...);    // 信息日志
TRM_LOG_DEBUG(fmt, ...);   // 调试日志
TRM_LOG_TRACE(fmt, ...);   // 跟踪日志
```

**功能**: TRM日志输出宏
**参数**:

- `fmt`: 格式化字符串
- `...`: 可变参数
  **说明**:
- 自动包含文件名、行号、函数名等调试信息
- 根据当前设置的日志级别过滤输出
- 完全独立于TK8710日志系统，无任何耦合

**TRM日志系统特性**:

- ✅ **完全独立**: 无TK8710日志系统依赖
- ✅ **线程安全**: 支持多线程环境使用
- ✅ **级别过滤**: 支持动态调整日志级别
- ✅ **调试信息**: 自动包含文件、行号、函数信息
- ✅ **时间戳**: 可选的时间戳功能

### 9. TRM调试接口

#### `TRM_TxValidatorOnRxData`

```c
int TRM_TxValidatorOnRxData(const TRM_RxDataList* rxDataList);
```

**功能**: 接收数据事件中触发发送验证
**参数**:

- `rxDataList`: 接收数据列表指针

**TRM_RxDataList结构体定义**:

```c
typedef struct {
    uint8_t  slotIndex;         /* 时隙索引 */
    uint8_t  userCount;         /* 用户数量 */
    uint16_t reserved;
    uint32_t frameNo;           /* 帧号 */
    TRM_RxUserData* users;      /* 用户数据数组 */
} TRM_RxDataList;
```

**TRM_RxUserData结构体定义**:

```c
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
```

**结构体成员详细说明**:

- `slotIndex`: 时隙索引
  - 范围: 0 - 7
- `userCount`: 用户数量
  - 范围: 0 - 128
- `frameNo`: 帧号
- `users`: 用户数据数组指针
  - 每个用户包含:
    - `userId`: 用户ID
    - `slotIndex`: 时隙索引
    - `dataLen`: 数据长度
    - `rateMode`: 接收速率模式
    - `rssi`: 信号强度(dBm)
    - `snr`: 信噪比(dB)
    - `data`: 数据指针
    - `freq`: 频率(Hz)
    - `beam`: 波束信息(详见TRM_BeamInfo结构体)
      **返回值**:
- `TRM_OK`: 处理成功
- `TRM_ERR_PARAM`: 参数错误
- 其他: 处理失败
  **说明**: 这是TRM发送验证器的核心函数，用于在接收到数据后自动触发相应的发送验证操作

---

## 使用示例

### 完整系统初始化示例

```c
#include "tk8710_api.h"
#include "trm_api.h"

int main(void) {
    int ret;
  
    // 0. GPIO中断初始化配置
    ret = TK8710GpioInit();
    if (ret != TK8710_OK) {
        printf("GPIO初始化失败: %d\n", ret);
        return -1;
    }

    // 1. 启用GPIO中断
    ret = TK8710GpioIrqEnable();
    if (ret != TK8710_OK) {
        printf("GPIO中断使能失败: %d\n", ret);
        return -1;
    }
  
    // 2. 8710芯片初始化（数字/MAC层）
    ChipConfig chipConfig = {
        .bcn_agc = 32,           // BCN AGC长度
        .interval = 32,          // Intval长度
        .tx_dly = 1,             // 下行发送时延
        .tx_fix_info = 0,        // TX固定频点
        .offset_adj = 0,        // BCN sync窗口微调
        .tx_pre = 0,             // 发送数据窗口调整
        .conti_mode = 1,         // 连续工作模式
        .bcn_scan = 0,           // BCN SCAN模式
        .ant_en = 0xFF,          // 天线使能
        .rf_sel = 0xFF,          // 射频使能
        .tx_bcn_en = 1,          // 发送BCN使能
        .ts_sync = 0,            // 本地同步
        .rf_model = 2,           // 射频芯片型号: 2=SX1257
        .bcnbits = 0x1F,         // 信标标识位
        .anoiseThe1 = 0,         // 用户检测门限
        .power2rssi = 0,         // RSSI换算
        .irq_ctrl0 = 0xFFFFFFFF,  // 中断使能
        .irq_ctrl1 = 0           // 中断清理
    };
  
    ret = TK8710Init(&chipConfig);
    if (ret != TK8710_OK) {
        printf("芯片初始化失败: %d\n", ret);
        return -1;
    }
  
    // 3. 8710射频初始化
    ChiprfConfig rfConfig = {
        .rftype = TK8710_RF_TYPE_1257_32M,  // SX1257 32MHz
        .Freq = 2400000000,                  // 2.4GHz
        .rxgain = 1,                         // 接收增益
        .txgain = 20,                        // 发送增益
        .txadc = {
            {.i = 0, .q = 0},  // 天线1直流校准
            {.i = 0, .q = 0},  // 天线2直流校准
            // ... 其他天线
        }
    };
  
    ret = TK8710RfConfig(&rfConfig);
    if (ret != TK8710_OK) {
        printf("射频初始化失败: %d\n", ret);
        return -1;
    }
  
    // 4. 日志系统初始化
    ret = TK8710LogSimpleInit(TK8710_LOG_INFO, TK8710_LOG_MODULE_ALL);
    if (ret != TK8710_OK) {
        printf("日志系统初始化失败: %d\n", ret);
        return -1;
    }
  
    // 5. 注册Driver回调函数
    TK8710DriverCallbacks driverCallbacks = {
        .onRxData = OnDriverRxData,
        .onTxSlot = OnDriverTxSlot,
        .onSlotEnd = OnDriverSlotEnd,
        .onError = OnDriverError
    };
    TK8710RegisterCallbacks(&driverCallbacks);
  
    // 6. 初始化TRM系统
    TRM_InitConfig trmConfig = {
        .beamMode = TRM_BEAM_MODE_FULL_STORE,
        .beamMaxUsers = 3000,
        .beamTimeoutMs = 10000,
        .callbacks = {
            .onRxData = OnTrmRxData,
            .onTxComplete = OnTrmTxComplete
        },
        .platformConfig = NULL
    };
  
    ret = TRM_Init(&trmConfig);
    if (ret != TRM_OK) {
        printf("TRM初始化失败: %d\n", ret);
        return -1;
    }
  
    // 7. 8710配置（时隙配置、启动工作）
    // 配置时隙参数
    TK8710SetConfig(TK8710_CONFIG_TYPE_SLOT, &slotConfig);
  
    // 启动芯片工作
    ret = TK8710Start(TK8710_WORK_TYPE_MASTER, TK8710_WORK_MODE_CONTINUOUS);
    if (ret != TK8710_OK) {
        printf("芯片启动失败: %d\n", ret);
        return -1;
    }
  
    // 8. 启动TRM系统
    ret = TRM_Start();
    if (ret != TRM_OK) {
        printf("TRM启动失败: %d\n", ret);
        return -1;
    }
  
    printf("系统初始化完成\n");
    return 0;
}
```

### TRM上层回调接口示例

```c
// TRM上层回调接口实现 - TRM调用上层应用
void OnTrmRxData(const TRM_RxDataList* rxDataList) {
    // 数据处理流程简述：
    // 1. 验证接收数据有效性
    // 2. 提取用户数据和业务信息
    // 3. 更新业务状态和统计信息
    // 4. 触发相应的业务逻辑处理
  
    printf("TRM回调: 接收到数据 - 时隙=%d, 用户数=%d, 帧号=%u\n", 
           rxDataList->slotIndex, rxDataList->userCount, rxDataList->frameNo);
  
    // 业务数据处理逻辑（具体实现根据应用需求）
    for (int i = 0; i < rxDataList->userCount; i++) {
        TRM_RxUserData* user = &rxDataList->users[i];
        printf("处理用户数据: ID=0x%08X, 长度=%u\n", user->userId, user->dataLen);
        // ... 具体的业务处理
    }
}

void OnTrmTxComplete(uint32_t userId, TRM_TxResult result) {
    // 发送完成处理流程简述：
    // 1. 检查发送结果状态
    // 2. 更新发送统计信息
    // 3. 处理重发或确认逻辑
    // 4. 释放相关资源
  
    printf("TRM回调: 发送完成 - 用户ID=0x%08X, 结果=%s\n", 
           userId, result == TRM_TX_SUCCESS ? "成功" : "失败");
  
    if (result == TRM_TX_SUCCESS) {
        // 发送成功处理
    } else {
        // 发送失败处理（可能需要重发）
    }
}
```

### Driver回调注册示例

```c
// Driver回调函数实现 - Driver调用TRM
void OnDriverRxData(TK8710IrqResult* irqResult) {
    // Driver中断数据处理流程简述：
    // 1. 验证中断结果有效性
    // 2. 提取硬件状态信息
    // 3. 调用TRM核心处理函数
    // 4. 更新内部状态和统计
  
    printf("Driver回调: 接收数据中断 - 类型=%d, 用户数=%d\n", 
           irqResult->irqType, irqResult->userCount);
  
    // 调用TRM核心处理函数
    TRM_ProcessDriverIrq(irqResult);
}

void OnDriverTxSlot(TK8710IrqResult* irqResult) {
    // 发送时隙中断处理流程简述：
    // 1. 检查发送时隙状态
    // 2. 更新发送统计信息
    // 3. 触发TRM发送逻辑
    // 4. 准备下一时隙数据
  
    printf("Driver回调: 发送时隙中断 - 时隙=%d\n", irqResult->slotIndex);
  
    // 通知TRM发送完成
    TRM_NotifyTxSlotComplete(irqResult);
}

void OnDriverSlotEnd(TK8710IrqResult* irqResult) {
    // 时隙结束中断处理流程简述：
    // 1. 清理当前时隙资源
    // 2. 准备下一时隙配置
    // 3. 更新系统状态
    // 4. 触发时隙切换逻辑
  
    printf("Driver回调: 时隙结束中断 - 时隙=%d\n", irqResult->slotIndex);
  
    // 通知TRM时隙结束
    TRM_NotifySlotEnd(irqResult);
}

void OnDriverError(TK8710IrqResult* irqResult) {
    // 错误中断处理流程简述：
    // 1. 分析错误类型和严重程度
    // 2. 记录错误信息和统计
    // 3. 执行错误恢复策略
    // 4. 通知上层应用
  
    printf("Driver回调: 错误中断 - 类型=%d, 代码=%d\n", 
           irqResult->irqType, irqResult->errorCode);
  
    // 通知TRM发生错误
    TRM_NotifyError(irqResult);
}

// Driver回调注册
void RegisterDriverCallbacks(void) {
    TK8710DriverCallbacks callbacks = {
        .onRxData = OnDriverRxData,
        .onTxSlot = OnDriverTxSlot,
        .onSlotEnd = OnDriverSlotEnd,
        .onError = OnDriverError
    };
  
    TK8710RegisterCallbacks(&callbacks);
    printf("Driver回调注册完成\n");
}
```

### 数据发送示例

```c
// 单速率模式发送(使用帧号匹配)
uint8_t testData[] = {0x01, 0x02, 0x03};
uint32_t currentFrame = TRM_GetCurrentFrame();
ret = TRM_SendData(0x12345678, testData, sizeof(testData), 20, currentFrame + 1, 0);

// 多速率模式发送(使用速率模式匹配)
ret = TRM_SendData(0x12345678, testData, sizeof(testData), 20, currentFrame + 1, TK8710_RATE_MODE_5);
```

### Driver配置管理示例

```c
// 1. 获取当前配置
ChipConfig currentChipConfig;
ret = TK8710GetConfig(TK8710_CONFIG_TYPE_CHIP, &currentChipConfig);
if (ret == TK8710_OK) {
    printf("当前芯片配置: BCN_AGC=%u, 工作模式=%s\n", 
           currentChipConfig.bcn_agc, 
           currentChipConfig.conti_mode ? "连续" : "单次");
}

ChiprfConfig currentRfConfig;
ret = TK8710GetConfig(TK8710_CONFIG_TYPE_RF, &currentRfConfig);
if (ret == TK8710_OK) {
    printf("当前射频配置: 频率=%uHz, 接收增益=%u, 发送增益=%u\n",
           currentRfConfig.Freq, currentRfConfig.rxgain, currentRfConfig.txgain);
}

// 2. 动态修改配置
ChipConfig newChipConfig = currentChipConfig;
newChipConfig.conti_mode = 0;  // 切换到单次工作模式
newChipConfig.tx_dly = 2;      // 调整发送时延

ret = TK8710SetConfig(TK8710_CONFIG_TYPE_CHIP, &newChipConfig);
if (ret == TK8710_OK) {
    printf("芯片配置更新成功\n");
}

// 3. 时隙配置获取和使用
const slotCfg_t* slotConfig = TK8710GetSlotCfg();
if (slotConfig != NULL) {
    printf("时隙配置: 字节数=%u, 时间长度=%uus, 中心频点=%uHz\n",
           slotConfig->byteLen, slotConfig->timeLen, slotConfig->centerFreq);
  
    // 根据时隙配置调整业务逻辑
    if (slotConfig->byteLen > 512) {
        printf("大包模式，启用数据分片处理\n");
    }
}

// 4. 用户信息设置
TK8710UserInfo userInfo = {
    .userId = 0x12345678,
    .rateMode = TK8710_RATE_MODE_8,
    .power = 25,
    .freqOffset = 0
};

ret = TK8710SetTxUserInfo(0, &userInfo);
if (ret == TK8710_OK) {
    printf("用户信息设置成功: ID=0x%08X, 速率模式=%d\n", 
           userInfo.userId, userInfo.rateMode);
}
```

### 接收数据处理示例

```c
// 在中断回调中处理接收数据
void OnTrmRxData(const TRM_RxDataList* rxDataList) {
    printf("接收到数据: 时隙=%d, 用户数=%d, 帧号=%u\n", 
           rxDataList->slotIndex, rxDataList->userCount, rxDataList->frameNo);
  
    // 处理第一个用户的数据
    if (rxDataList->userCount > 0) {
        TRM_RxUserData* user = &rxDataList->users[0];
        printf("用户ID: 0x%08X, 速率模式: %d, 数据长度: %u\n", 
               user->userId, user->rateMode, user->dataLen);
    }
  
    // 调试接口：触发发送验证（仅在调试时使用）
    TRM_TxValidatorOnRxData(rxDataList);
}
```

### 日志系统使用示例

```c
// 1. 初始化日志系统
TK8710LogSimpleInit(TK8710_LOG_INFO, TK8710_LOG_MODULE_ALL);

// 2. 使用日志宏输出日志
TK8710_LOG_CORE_ERROR("系统初始化失败: %d", ret);
TK8710_LOG_CONFIG_WARN("配置参数无效: %s", param_name);
TK8710_LOG_IRQ_INFO("中断处理完成: 类型=%d", irq_type);
TK8710_LOG_HAL_DEBUG("寄存器读写: 地址=0x%04X, 值=0x%08X", reg_addr, value);

// 3. 不同级别和模块的日志示例
TK8710_LOG_ERROR(TK8710_LOG_MODULE_CORE, "严重错误: %s", error_msg);
TK8710_LOG_WARN(TK8710_LOG_MODULE_CONFIG, "警告: 配置超时");
TK8710_LOG_INFO(TK8710_LOG_MODULE_IRQ, "信息: 中断触发");
TK8710_LOG_DEBUG(TK8710_LOG_MODULE_HAL, "调试: 硬件初始化");
TK8710_LOG_TRACE(TK8710_LOG_MODULE_ALL, "跟踪: 函数进入");
```

### 调试接口使用示例

```c
// 1. 中断时间统计
// 获取特定中断类型的统计信息
uint32_t totalTime, maxTime, minTime, count;
int ret = TK8710GetIrqTimeStats(1, &totalTime, &maxTime, &minTime, &count);
if (ret == 0) {
    printf("中断类型1统计: 总时间=%uus, 最大=%uus, 最小=%uus, 次数=%u\n", 
           totalTime, maxTime, minTime, count);
}

// 打印完整的中断时间统计报告
TK8710PrintIrqTimeStats();

// 重置统计信息
TK8710ResetIrqTimeStats(255);  // 重置所有中断统计

// 2. 测试接口使用
// 强制处理所有用户数据（用于测试中断处理能力）
TK8710SetForceProcessAllUsers(1);
printf("当前强制处理所有用户: %s\n", 
       TK8710GetForceProcessAllUsers() ? "是" : "否");

// 强制按最大用户数发送（用于测试发送能力）
TK8710SetForceMaxUsersTx(1);
printf("当前强制最大用户发送: %s\n", 
       TK8710GetForceMaxUsersTx() ? "是" : "否");

// 3. 调试控制接口
// 获取FFT输出数据
uint8_t fftData[1024];
ret = TK8710DebugCtrl(TK8710_DBG_TYPE_FFT_OUT, 1, NULL, fftData);
if (ret == 0) {
    printf("FFT数据获取成功\n");
}

// 获取捕获数据
uint8_t captureData[2048];
ret = TK8710DebugCtrl(TK8710_DBG_TYPE_CAPTURE_DATA, 1, NULL, captureData);
if (ret == 0) {
    printf("捕获数据获取成功\n");
}
```

---

## 注意事项

1. **初始化顺序**: 必须先初始化Driver，再初始化TRM
2. **中断处理**: TRM的中断回调需要注册到Driver
3. **多速率支持**: 使用 `TRM_SendData`进行多速率发送
4. **资源管理**: 及时释放接收数据Buffer，避免内存泄漏
5. **错误处理**: 所有API调用都需要检查返回值
6. **线程安全**: 在多线程环境下需要注意临界区保护
7. **日志系统**:
   - 使用 `TK8710LogSimpleInit`快速初始化日志系统
   - 生产环境建议设置为INFO或WARN级别以减少性能影响
   - 日志系统支持模块化输出，便于调试和问题定位
   - 日志宏在编译时会根据级别和模块掩码进行优化，无效日志不会产生性能开销
8. **调试接口**:
   - 测试接口（ForceProcessAllUsers、ForceMaxUsersTx）仅用于开发测试，生产环境应禁用
   - 中断时间统计会影响性能，仅在调试时启用
   - 调试控制接口获取的数据量较大，注意缓冲区大小
   - FFT和捕获数据获取可能影响正常通信，应在非关键时段使用

---

## 版本信息

- **文档版本**: 1.0
- **API版本**: TK8710 HAL v1.3
- **更新日期**: 2026-02-27
