# TK8710 TRM和Driver API接口文档

## 概述

本文档整理了TK8710 HAL层中TRM(Transmission Resource Management)模块和Driver模块的主要API接口，包括接口参数说明和使用方法。

---

## Driver API接口

### 1. Driver API接口 (5大类，17+函数)

### 1. 芯片初始化与控制

#### `TK8710Init`
```c
int TK8710Init(const ChipConfig* initConfig, const TK8710IrqCallback* irqCallback);
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
- `irqCallback`: 中断回调函数指针
  ```c
  typedef void (*TK8710IrqCallback)(TK8710IrqResult irqResult);
  
  typedef struct {
      irqType_e irq_type;             /* 中断类型 */
      int32_t   bcn_freq_offset;      /* BCN offset */
      uint8_t   rx_bcnbits;           /* 接收BCN bit数 */
      uint8_t   rxbcn_status;         /* 接收BCN状态 */
      
      /* MD_UD中断专用信息 */
      uint8_t  mdUserDataValid; /* MD_UD用户波束信息有效性 */
      
      /* BRD_UD中断专用信息 */
      uint8_t  brdUserDataValid; /* BRD_UD用户波束信息有效性 */

      /* MD_DATA中断专用信息 */
      uint8_t  mdDataValid;         /* MD_DATA数据有效性 */
      uint8_t  crcValidCount;       /* CRC正确的用户数量 */
      uint8_t  crcErrorCount;       /* CRC错误的用户数量 */
      uint8_t  maxUsers;            /* 当前速率模式最大用户数 */
      TK8710CrcResult crcResults[128]; /* CRC结果数组 (最多128个用户) */
 
      /* BRD_DATA中断专用信息 */
      uint8_t  brdDataValid;         /* BRD_DATA数据有效性 */
      uint8_t  brdCrcValidCount;       /* CRC正确的用户数量 */
      uint8_t  brdCrcErrorCount;       /* CRC错误的用户数量 */
      uint8_t  brdMaxUsers;            /* 当前速率模式最大用户数 */
      TK8710CrcResult brdCrcResults[16]; /* CRC结果数组 (最多16个用户) */

      /* S1时隙自动发送信息 */
      uint8_t  autoTxValid;         /* 自动发送数据有效性 */
      uint8_t  autoTxCount;         /* 自动发送用户数量 */
      
      /* 广播发送信息 */
      uint8_t  brdTxValid;          /* 广播发送数据有效性 */
      uint8_t  brdTxCount;          /* 广播发送用户数量 */
      
      /* 信号质量信息 */
      uint8_t  signalInfoValid;     /* 信号信息有效性 */
      uint8_t  currentRateIndex;    /* 当前速率序号 (0-based) */
  } TK8710IrqResult;
  ```
**返回值**: 0-成功, 1-失败, 2-超时

#### `TK8710Startwork`
```c
int TK8710Startwork(uint8_t workType, uint8_t workMode);
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

#### `TK8710RfInit`
```c
int TK8710RfInit(const ChiprfConfig* initrfConfig);
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

#### `TK8710ResetChip`
```c
int TK8710ResetChip(uint8_t rstType);
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
      TK8710_CFG_TYPE_FIX_INFO_LIST,    /* 固定信息列表配置 */
      TK8710_CFG_TYPE_DL_USER_POWER,    /* 下行用户功率配置 */
      TK8710_CFG_TYPE_SLOT_CFG,         /* 时隙配置 */
      TK8710_CFG_TYPE_ADDTL,            /* 附加位配置 */
  } TK8710ConfigType;
  ```
- `params`: 配置参数指针，具体类型根据type决定
**返回值**: 0-成功, 1-失败, 2-超时

#### `TK8710GetConfig`
```c
int TK8710GetConfig(TK8710ConfigType type, void* params);
```
**功能**: 获取芯片配置
**参数**:
- `type`: 配置类型 (同TK8710SetConfig)
- `params`: 配置参数输出指针
**返回值**: 0-成功, 1-失败, 2-超时

### 3. 数据传输

#### `TK8710SetDownlink1DataWithPower`
```c
int TK8710SetDownlink1DataWithPower(uint8_t userIndex, const uint8_t* data, uint16_t dataLen, uint8_t power, TK8710BrdDataType dataType);
```
**功能**: 设置下行1广播数据
**参数**:
- `userIndex`: 广播用户索引 (0-15)
- `data`: 广播数据指针
- `dataLen`: 数据长度 (0-512字节)
- `power`: 发射功率 (0-31)
- `dataType`: 数据类型
  ```c
  #define TK8710_BRD_DATA_TYPE_NORMAL     0   /* 正常广播: 使用Driver自动生成的波束信息 */
  #define TK8710_BRD_DATA_TYPE_SLOT3      1   /* 与Slot3共用波束信息 */
  ```
**返回值**: 0-成功, 1-失败, 2-超时

#### `TK8710SetDownlink2DataWithPower`
```c
int TK8710SetDownlink2DataWithPower(uint8_t userIndex, const uint8_t* data, uint16_t dataLen, uint8_t power, TK8710UserDataType dataType);
```
**功能**: 设置下行2用户数据
**参数**:
- `userIndex`: 用户索引 (0-127)
- `data`: 用户数据指针
- `dataLen`: 数据长度 (0-512字节)
- `power`: 发射功率 (0-31)
- `dataType`: 数据类型
  ```c
  #define TK8710_USER_DATA_TYPE_NORMAL    0   /* 正常发送: 使用指定信息模式的波束信息 */
  #define TK8710_USER_DATA_TYPE_SLOT1     1   /* 与Slot1共用波束信息 (Driver自动生成) */
  ```
**返回值**: 0-成功, 1-失败, 2-超时

#### `TK8710SetTxUserInfo`
```c
int TK8710SetTxUserInfo(uint8_t userIndex, uint32_t freq, const uint8_t* ahData, uint8_t pilotPower);
```
**功能**: 设置发送用户信息
**参数**:
- `userIndex`: 用户索引 (0-127)
- `freq`: 用户频率 (Hz)
- `ahData`: AH数据指针 (64字节数组)
- `pilotPower`: 导频功率 (0-31)
**返回值**: 0-成功, 1-失败, 2-超时

### 4. 数据接收

#### `TK8710GetRxData`
```c
int TK8710GetRxData(uint8_t userIndex, uint8_t** data, uint16_t* dataLen);
```
**功能**: 获取接收数据
**参数**:
- `userIndex`: 用户索引 (0-127)
- `data`: 数据指针输出
- `dataLen`: 数据长度输出
**返回值**: 0-成功, 1-失败, 2-超时

#### `TK8710GetSignalInfo`
```c
int TK8710GetSignalInfo(uint8_t userIndex, uint32_t* rssi, uint8_t* snr, uint32_t* freq);
```
**功能**: 获取信号质量信息
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

### 6. 中断处理

#### `TK8710IrqInit`
```c
int TK8710IrqInit(const TK8710IrqCallback* irqCallback);
```
**功能**: 初始化驱动层中断系统
**参数**:
- `irqCallback`: 中断回调函数指针 (参见TK8710Init参数说明)
**返回值**: 0-成功, 1-失败, 2-超时

#### `TK8710GpioInit`
```c
int TK8710GpioInit(uint8_t gpioPin, TK8710GpioEdge edge, const TK8710IrqCallback* callback, void* userData);
```
**功能**: 初始化GPIO中断
**参数**:
- `gpioPin`: GPIO引脚号
- `edge`: 中断触发边沿
  ```c
  typedef enum {
      TK8710_GPIO_EDGE_RISING = 0,   /* 上升沿触发 */
      TK8710_GPIO_EDGE_FALLING = 1,  /* 下降沿触发 */
      TK8710_GPIO_EDGE_BOTH = 2,      /* 双边沿触发 */
  } TK8710GpioEdge;
  ```
- `callback`: 回调函数指针
- `userData`: 用户数据指针
**返回值**: 0-成功, 1-失败, 2-超时

#### `TK8710GpioIrqEnable`
```c
int TK8710GpioIrqEnable(uint8_t gpioPin, uint8_t enable);
```
**功能**: 使能GPIO中断
**参数**:
- `gpioPin`: GPIO引脚号
- `enable`: 使能标志 (1=使能, 0=禁用)
**返回值**: 0-成功, 1-失败, 2-超时

---

## TRM API接口

### 1. 系统初始化与控制

#### `TRM_Init`
```c
int TRM_Init(const TRM_InitConfig* config);
```
**功能**: 初始化TRM系统
**参数**:
- `config`: TRM初始化配置
**返回值**: TRM_OK-成功, 其他-失败

#### `TRM_Start`
```c
int TRM_Start(void);
```
**功能**: 启动TRM系统
**返回值**: TRM_OK-成功, 其他-失败

#### `TRM_Stop`
```c
int TRM_Stop(void);
```
**功能**: 停止TRM系统
**返回值**: TRM_OK-成功, 其他-失败

#### `TRM_Deinit`
```c
void TRM_Deinit(void);
```
**功能**: 清理TRM系统

### 2. 数据发送

#### `TRM_SendData`
```c
int TRM_SendData(uint32_t userId, const uint8_t* data, uint16_t dataLen, uint8_t power, uint32_t frameNo);
```
**功能**: 发送数据(单速率模式)
**参数**:
- `userId`: 用户ID
- `data`: 数据指针
- `dataLen`: 数据长度
- `power`: 发射功率
- `frameNo`: 目标帧号
**返回值**: TRM_OK-成功, 其他-失败

#### `TRM_SendDataWithRateMode`
```c
int TRM_SendDataWithRateMode(uint32_t userId, const uint8_t* data, uint16_t dataLen, uint8_t power, uint32_t frameNo, uint8_t targetRateMode);
```
**功能**: 发送数据(多速率模式)
**参数**:
- `userId`: 用户ID
- `data`: 数据指针
- `dataLen`: 数据长度
- `power`: 发射功率
- `frameNo`: 目标帧号
- `targetRateMode`: 目标速率模式
**返回值**: TRM_OK-成功, 其他-失败

### 3. 状态查询

#### `TRM_GetCurrentFrame`
```c
uint32_t TRM_GetCurrentFrame(void);
```
**功能**: 获取当前帧号
**返回值**: 当前帧号

#### `TRM_GetIrqCallback`
```c
TK8710IrqCallback* TRM_GetIrqCallback(void);
```
**功能**: 获取TRM中断回调函数
**返回值**: 中断回调函数指针

### 4. 日志系统

#### `TRM_LogInit`
```c
void TRM_LogInit(TRM_LogLevel level);
```
**功能**: 初始化TRM日志系统
**参数**:
- `level`: 日志级别

#### `TRM_LOG_*` 宏
```c
TRM_LOG_ERROR(fmt, ...);  // 错误日志
TRM_LOG_WARN(fmt, ...);   // 警告日志
TRM_LOG_INFO(fmt, ...);   // 信息日志
TRM_LOG_DEBUG(fmt, ...);  // 调试日志
```

### 5. 发送验证器

#### `TRM_TxValidatorInit`
```c
int TRM_TxValidatorInit(const TRM_TxValidatorConfig* config);
```
**功能**: 初始化发送验证器
**参数**:
- `config`: 验证器配置
**返回值**: TRM_OK-成功, 其他-失败

#### `TRM_TxValidatorOnRxData`
```c
int TRM_TxValidatorOnRxData(const TRM_RxDataList* rxDataList);
```
**功能**: 处理接收数据并触发发送
**参数**:
- `rxDataList`: 接收数据列表
**返回值**: TRM_OK-成功, 其他-失败

#### `TRM_TxValidatorGetStats`
```c
int TRM_TxValidatorGetStats(TRM_TxValidatorStats* stats);
```
**功能**: 获取验证器统计信息
**参数**:
- `stats`: 统计信息输出
**返回值**: TRM_OK-成功, 其他-失败

#### `TRM_TxValidatorDeinit`
```c
void TRM_TxValidatorDeinit(void);
```
**功能**: 清理发送验证器

---

## 使用示例

### 基本初始化流程
```c
// 1. 初始化Driver
ChipConfig chipConfig = {0};
TK8710IrqCallback* trmCallback = TRM_GetIrqCallback();
ret = TK8710Init(&chipConfig, trmCallback);

// 2. 初始化TRM
TRM_InitConfig trmConfig = {
    .beamMode = TRM_BEAM_MODE_FULL_STORE,
    .beamMaxUsers = 3000,
    .beamTimeoutMs = 10000,
    // ... 其他配置
};
ret = TRM_Init(&trmConfig);

// 3. 启动系统
ret = TK8710Startwork(TK8710_MODE_MASTER, TK8710_WORK_MODE_CONTINUOUS);
ret = TRM_Start();
```

### 数据发送示例
```c
// 单速率模式发送
uint8_t testData[] = {0x01, 0x02, 0x03};
uint32_t currentFrame = TRM_GetCurrentFrame();
ret = TRM_SendData(0x12345678, testData, sizeof(testData), 20, currentFrame + 1);

// 多速率模式发送
ret = TRM_SendDataWithRateMode(0x12345678, testData, sizeof(testData), 20, currentFrame + 1, TK8710_RATE_MODE_5);
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
    
    // 触发发送验证
    TRM_TxValidatorOnRxData(rxDataList);
}
```

---

## 注意事项

1. **初始化顺序**: 必须先初始化Driver，再初始化TRM
2. **中断处理**: TRM的中断回调需要注册到Driver
3. **多速率支持**: 使用`TRM_SendDataWithRateMode`进行多速率发送
4. **资源管理**: 及时释放接收数据Buffer，避免内存泄漏
5. **错误处理**: 所有API调用都需要检查返回值
6. **线程安全**: 在多线程环境下需要注意临界区保护

---

## 版本信息

- **文档版本**: 1.0
- **API版本**: TK8710 HAL v1.3
- **更新日期**: 2026-02-27
