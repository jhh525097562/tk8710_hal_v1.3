# TK8710 TRM和Driver API接口文档

## 概述

本文档整理了TK8710 HAL层中TRM(Transmission Resource Management)模块和Driver模块的主要API接口，包括接口参数说明和使用方法。

---

## Driver API接口

### 1. Driver API接口 (5大类，17+函数)

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
      uint32_t tx_dly;        /* 发送延迟, 默认1 */
      uint32_t offset_adj;    /* 偏移调整, 默认0 */
      uint32_t tx_pre;        /* 发送前导, 默认0 */
      uint32_t conti_mode;    /* 连续模式, 默认0 */
      uint32_t bcn_scan;      /* BCN扫描, 默认0 */
      uint32_t ant_en;        /* 天线使能, 默认1 */
      uint32_t rf_sel;        /* 射频选择, 默认0 */
      uint32_t tx_bcn_en;     /* 发送BCN使能, 默认1 */
      uint32_t ts_sync;       /* 时间同步, 默认0 */
      uint32_t rf_model;      /* 射频模式, 默认1 */
      uint32_t bcnbits;       /* BCN bit数, 默认0 */
      uint32_t anoiseThe1;    /* 噪声阈值1, 默认0 */
      uint32_t power2rssi;    /* 功率转RSSI, 默认0 */
      uint32_t irq_ctrl0;     /* 中断控制 */
      uint32_t irq_ctrl1;     /* 中断清理 */
  } ChipConfig;
  ```
- **返回值**: 0-成功, 1-失败, 2-超时

#### `TK8710Startwork`

```c
int TK8710Startwork(uint8_t workType, uint8_t workMode);
```

**功能**: 芯片进入收发状态
**参数**:

- `workType`: 工作类型 (1=Master, 2=Slave)
- `workMode`: 工作模式 (1=连续, 2=单次)
  **返回值**: 0-成功, 1-失败, 2-超时

#### `TK8710RfInit`

```c
int TK8710RfInit(const ChiprfConfig* initrfConfig);
```

**功能**: 初始化芯片连接射频
**参数**:

- `initrfConfig`: 射频初始化配置参数
  **返回值**: 0-成功, 1-失败, 2-超时

#### `TK8710ResetChip`

```c
int TK8710ResetChip(uint8_t rstType);
```

**功能**: 芯片复位
**参数**:

- `rstType`: 复位类型 (1=仅复位状态机, 2=复位状态机+寄存器)
  **返回值**: 0-成功, 1-失败, 2-超时

### 2. 配置管理

#### `TK8710SetConfig`

```c
int TK8710SetConfig(TK8710ConfigType type, const void* params);
```

**功能**: 设置芯片配置
**参数**:

- `type`: 配置类型
- `params`: 配置参数指针
  **返回值**: 0-成功, 1-失败, 2-超时

#### `TK8710GetConfig`

```c
int TK8710GetConfig(TK8710ConfigType type, void* params);
```

**功能**: 获取芯片配置
**参数**:

- `type`: 配置类型
- `params`: 配置参数输出指针
  **返回值**: 0-成功, 1-失败, 2-超时

### 3. 数据传输

#### `TK8710SetDownlink1DataWithPower`

```c
int TK8710SetDownlink1DataWithPower(uint8_t userIndex, const uint8_t* data, uint16_t dataLen, uint8_t power, TK8710BrdDataType dataType);
```

**功能**: 设置下行1数据(广播数据)
**参数**:

- `userIndex`: 用户索引
- `data`: 数据指针
- `dataLen`: 数据长度
- `power`: 发射功率
- `dataType`: 数据类型
  **返回值**: TK8710_OK-成功, 其他-失败

#### `TK8710SetDownlink2DataWithPower`

```c
int TK8710SetDownlink2DataWithPower(uint8_t userIndex, const uint8_t* data, uint16_t dataLen, uint8_t power, TK8710UserDataType dataType);
```

**功能**: 设置下行2数据(用户数据)
**参数**:

- `userIndex`: 用户索引
- `data`: 数据指针
- `dataLen`: 数据长度
- `power`: 发射功率
- `dataType`: 数据类型
  **返回值**: TK8710_OK-成功, 其他-失败

#### `TK8710SetTxUserInfo`

```c
int TK8710SetTxUserInfo(uint8_t userIndex, uint32_t freq, const uint8_t* ahData, uint8_t pilotPower);
```

**功能**: 设置发送用户信息
**参数**:

- `userIndex`: 用户索引
- `freq`: 频率
- `ahData`: AH数据
- `pilotPower`: 导频功率
  **返回值**: TK8710_OK-成功, 其他-失败

### 4. 数据接收

#### `TK8710GetRxData`

```c
int TK8710GetRxData(uint8_t userIndex, uint8_t** data, uint16_t* dataLen);
```

**功能**: 获取接收数据
**参数**:

- `userIndex`: 用户索引
- `data`: 数据指针输出
- `dataLen`: 数据长度输出
  **返回值**: TK8710_OK-成功, 其他-失败

#### `TK8710GetRxUserInfo`

```c
int TK8710GetRxUserInfo(uint8_t userIndex, uint32_t* freq, uint32_t* ahData, uint64_t* pilotPower);
```

**功能**: 获取接收用户信息 (从MD_UD中断获取的数据)
**参数**:

- `userIndex`: 用户索引 (0-127)
- `freq`: 输出频率指针 (Hz)
- `ahData`: 输出AH数据数组 (16个32位数据)
- `pilotPower`: 输出Pilot功率指针
  **返回值**: TK8710_OK-成功, 其他-失败

**说明**: TRM层使用此接口获取用户波束信息进行波束跟踪和管理

#### `TK8710GetSignalInfo`

```c
int TK8710GetSignalInfo(uint8_t userIndex, uint32_t* rssi, uint8_t* snr, uint32_t* freq);
```

**功能**: 获取信号质量信息
**参数**:

- `userIndex`: 用户索引
- `rssi`: RSSI输出
- `snr`: SNR输出
- `freq`: 频率输出
  **返回值**: TK8710_OK-成功, 其他-失败

#### `TK8710ReleaseRxData`

```c
void TK8710ReleaseRxData(uint8_t userIndex);
```

**功能**: 释放接收数据Buffer
**参数**:

- `userIndex`: 用户索引

### 5. 状态查询

#### `TK8710GetSlotCfg`

```c
const slotCfg_t* TK8710GetSlotCfg(void);
```

**功能**: 获取时隙配置
**返回值**: 时隙配置指针
**说明**: TRM和其他业务模块通过此接口获取完整的slot配置信息，包括速率模式配置等

### 6. Driver回调函数

Driver层提供多回调架构，支持为不同中断类型注册专用回调函数，提供更灵活的事件处理机制。

#### `TK8710RegisterCallbacks`

```c
void TK8710RegisterCallbacks(const TK8710DriverCallbacks* callbacks);
```

**功能**: 注册Driver多回调函数
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

- `onRxData`: 当接收到MD_DATA中断时调用
- `onTxSlot`: 当S1时隙发送完成时调用
- `onSlotEnd`: 当时隙结束时调用
- `onError`: 当发生错误中断时调用

**说明**:

- 替代了原有的单一回调接口 `TK8710IrqInit`
- 支持为不同中断类型注册专用回调函数
- 所有回调都使用统一的 `TK8710IrqResult*` 参数
- 提供更灵活的中断处理机制

#### `TK8710GpioInit`

```c
int TK8710GpioInit(int pin, TK8710GpioEdge edge, TK8710GpioIrqCallback cb, void* user);
```

**功能**: 初始化GPIO中断
**参数**:

- `pin`: GPIO引脚号
- `edge`: 中断触发边沿
- `cb`: GPIO中断回调函数
- `user`: 用户数据指针
  **返回值**: TK8710_OK-成功, 其他-失败

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
  **返回值**: TK8710_OK-成功, 其他-失败

#### Driver回调使用示例

```c
// 定义Driver回调函数
void OnDriverRxData(TK8710IrqResult* irqResult)
{
    printf("接收到MD_DATA中断: 用户数=%d\n", irqResult->crcValidCount);
    // 处理接收数据...
}

void OnDriverTxSlot(TK8710IrqResult* irqResult)
{
    printf("S1时隙发送完成\n");
    // 处理发送完成...
}

void OnDriverError(TK8710IrqResult* irqResult)
{
    printf("Driver错误: 中断类型=%d\n", irqResult->irq_type);
    // 错误处理...
}

// 注册Driver回调
TK8710DriverCallbacks driverCallbacks = {
    .onRxData = OnDriverRxData,
    .onTxSlot = OnDriverTxSlot,
    .onSlotEnd = NULL,  // 可选
    .onError = OnDriverError
};

TK8710RegisterCallbacks(&driverCallbacks);
```

### 7. 中断处理

中断处理系统负责处理硬件中断事件，并将事件分发到相应的Driver回调函数。

**中断类型**:

- `TK8710_IRQ_RX_BCN`: 接收信标中断
- `TK8710_IRQ_BRD_UD`: 广播用户数据中断
- `TK8710_IRQ_BRD_DATA`: 广播数据中断
- `TK8710_IRQ_MD_UD`: 主数据用户中断（用户波束信息）
- `TK8710_IRQ_MD_DATA`: 主数据中断（用户数据）
- `TK8710_IRQ_S0-S3`: 时隙中断
- `TK8710_IRQ_ACM`: 自适应调制中断

**处理流程**:

1. 硬件中断触发
2. GPIO中断回调被调用
3. Driver层处理中断
4. 根据中断类型调用相应的Driver回调
5. 应用层处理具体事件

**说明**: 中断处理通过Driver回调函数实现事件分发，具体的中断类型处理逻辑在相应的回调函数中实现。

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
int TRM_SendData(uint32_t userId, const uint8_t* data, uint16_t len, uint8_t txPower, uint32_t frameNo, uint8_t targetRateMode, uint8_t dataType);
```

**功能**: 发送用户数据
**参数**:

- `userId`: 用户ID
- `data`: 数据缓冲区
- `len`: 数据长度
- `txPower`: 发射功率
- `frameNo`: 目标发送帧号
- `targetRateMode`: 目标速率模式 (0=使用帧号, 5-11,18=使用速率模式)
- `dataType`: 数据类型 (TK8710_USER_DATA_TYPE_NORMAL 或 TK8710_USER_DATA_TYPE_SLOT3)
- **返回值**: TRM_OK成功，其他失败

#### `TRM_SendBroadcast`

```c
int TRM_SendBroadcast(uint8_t brdIndex, const uint8_t* data, uint16_t len, uint8_t txPower, uint8_t dataType);
```

**功能**: 发送广播数据
**参数**:

- `brdIndex`: 广播索引
- `data`: 广播数据
- `len`: 数据长度
- `txPower`: 发射功率
- `dataType`: 数据类型 (TK8710_BRD_DATA_TYPE_NORMAL 或 TK8710_BRD_DATA_TYPE_SLOT3)
- **返回值**: TRM_OK成功，其他失败

### 3. TRM回调函数

TRM提供上层回调接口，用于通知应用层接收数据、发送完成和错误事件。

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

**说明**: 当接收到用户数据时调用，应用层需要同步处理数据

#### `TRM_OnTxComplete`

```c
typedef void (*TRM_OnTxComplete)(uint32_t userId, TRM_TxResult result);
```

**功能**: 发送完成回调函数
**参数**:

- `userId`: 用户ID
- `result`: 发送结果

**说明**: 当数据发送完成时调用，通知发送结果

#### `TRM_Callbacks`

```c
typedef struct {
    TRM_OnRxData      onRxData;        /* 接收数据回调 */
    TRM_OnTxComplete  onTxComplete;    /* 发送完成回调 */
} TRM_Callbacks;
```

**说明**: TRM回调函数结构体，在 `TRM_InitConfig` 中使用

### 4. 状态查询

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
    uint32_t totalFrames;        /* 总帧数 */
    uint32_t successfulFrames;   /* 成功帧数 */
    uint32_t failedFrames;       /* 失败帧数 */
    uint32_t totalUsers;         /* 总用户数 */
    uint32_t activeUsers;        /* 活跃用户数 */
    uint32_t beamTableSize;      /* 波束表大小 */
    uint32_t currentFrame;       /* 当前帧号 */
    uint8_t  state;              /* TRM状态 */
    uint8_t  reserved[3];        /* 保留字段 */
} TRM_Stats;
```

#### `TRM_GetCurrentFrame`

```c
uint32_t TRM_GetCurrentFrame(void);
```

**功能**: 获取当前帧号
**返回值**: 当前帧号

#### `TRM_RegisterDriverCallbacks`

```c
int TRM_RegisterDriverCallbacks(void);
```

**功能**: 注册TRM到Driver的回调函数
**返回值**:

- `TRM_OK`: 注册成功
- `TRM_ERR_NOT_INIT`: TRM未初始化
- 其他: 注册失败

**说明**: 此函数将TRM的回调适配函数注册到Driver的多回调系统中

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

### 5. 调试接口

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

## 使用示例

### 基本初始化流程

```c
// 1. 初始化中断系统
TK8710IrqCallback* trmCallback = TRM_GetIrqCallback();
TK8710IrqInit(trmCallback);  // 设置TRM回调

// 1. 初始化Driver
ChipConfig chipConfig = {0};
ret = TK8710Init(&chipConfig);

// 2. 初始化TRM
TRM_InitConfig trmConfig = {
    .beamMode = TRM_BEAM_MODE_FULL_STORE,
    .beamMaxUsers = 3000,
    .beamTimeoutMs = 10000,
    // ... 其他配置
};
ret = TRM_Init(&trmConfig);

// 3. 注册TRM到Driver的回调函数
ret = TRM_RegisterDriverCallbacks();

// 4. 启动系统
ret = TK8710Startwork(TK8710_MODE_MASTER, TK8710_WORK_MODE_CONTINUOUS);
ret = TRM_Start();
```

### 数据发送示例

```c
// 单速率模式发送(使用帧号匹配)
uint8_t testData[] = {0x01, 0x02, 0x03};
uint32_t currentFrame = TRM_GetCurrentFrame();
ret = TRM_SendData(0x12345678, testData, sizeof(testData), 20, currentFrame + 1, 0, TK8710_USER_DATA_TYPE_NORMAL);

// 多速率模式发送(使用速率模式匹配)
ret = TRM_SendData(0x12345678, testData, sizeof(testData), 20, currentFrame + 1, TK8710_RATE_MODE_5, TK8710_USER_DATA_TYPE_NORMAL);

// 广播发送
uint8_t brdData[] = {0xAA, 0xBB, 0xCC};
ret = TRM_SendBroadcast(0, brdData, sizeof(brdData), 35, TK8710_BRD_DATA_TYPE_NORMAL);
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
2. **回调注册**: TRM的回调需要通过 `TRM_RegisterDriverCallbacks()` 注册到Driver
3. **多速率支持**: 使用 `TRM_SendData`进行多速率发送
4. **资源管理**: 及时释放接收数据Buffer，避免内存泄漏
5. **错误处理**: 所有API调用都需要检查返回值
6. **线程安全**: 在多线程环境下需要注意临界区保护
7. **多回调架构**: 使用新的多回调接口替代单一回调接口
8. **GPIO中断**: GPIO中断使用独立的回调类型 `TK8710GpioIrqCallback`

---

## 版本信息

- **文档版本**: 1.0
- **API版本**: TK8710 HAL v1.3
- **更新日期**: 2026-02-27
