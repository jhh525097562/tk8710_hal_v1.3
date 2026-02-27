# TK8710 HAL API参考手册

## 目录

1. [TRM模块（对外接口）](#trm模块对外接口)
2. [Driver模块（内部接口）](#driver模块内部接口)
3. [日志模块](#日志模块)
4. [调试模块](#调试模块)
5. [错误码](#错误码)

---

## TRM模块

### TRM_Init

初始化TRM模块。

```c
int TRM_Init(const TRM_InitConfig* config);
```

**参数：**

- `config` - 初始化配置结构体指针

**返回值：**

- `TRM_OK` - 成功
- `TRM_ERR_PARAM` - 参数错误
- `TRM_ERR_STATE` - 状态错误（已初始化）

**示例：**

```c
TRM_InitConfig config = {0};
config.beamMode = TRM_BEAM_MODE_FULL_STORE;
config.beamMaxUsers = 3000;
config.rateMode = TRM_RATE_4M;
config.antennaCount = 8;
config.callbacks.onRxData = MyRxCallback;

int ret = TRM_Init(&config);
```

---

### TRM_Deinit

反初始化TRM模块，释放资源。

```c
void TRM_Deinit(void);
```

---

### TRM_Start

启动TRM，开始收发数据。

```c
int TRM_Start(void);
```

**返回值：**

- `TRM_OK` - 成功
- `TRM_ERR_STATE` - 状态错误（未初始化或已运行）

---

### TRM_Stop

停止TRM。

```c
int TRM_Stop(void);
```

**返回值：**

- `TRM_OK` - 成功
- `TRM_ERR_STATE` - 状态错误（未运行）

---

### TRM_SendData

发送用户数据（支持单速率和多速率模式）。

```c
int TRM_SendData(uint32_t userId, const uint8_t* data, uint16_t len, uint8_t txPower, uint32_t frameNo, uint8_t targetRateMode);
```

**参数：**

- `userId` - 目标用户ID
- `data` - 数据指针
- `len` - 数据长度（1-512字节）
- `txPower` - 发射功率
- `frameNo` - 目标帧号
- `targetRateMode` - 目标速率模式 (0=使用帧号匹配, 5-11,18=使用速率模式匹配)

**返回值：**

- `TRM_OK` - 成功（已加入发送队列）
- `TRM_ERR_PARAM` - 参数错误
- `TRM_ERR_QUEUE_FULL` - 发送队列已满

**示例：**

```c
uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
int ret = TRM_SendData(0x12345678, data, sizeof(data), 20, 0, 0);
```

---

### TRM_SetBeamInfo

设置用户波束信息。

```c
int TRM_SetBeamInfo(uint32_t userId, const TRM_BeamInfo* beamInfo);
```

**参数：**

- `userId` - 用户ID
- `beamInfo` - 波束信息结构体指针

**返回值：**

- `TRM_OK` - 成功
- `TRM_ERR_PARAM` - 参数错误
- `TRM_ERR_NO_MEM` - 内存不足（超过最大用户数）

---

### TRM_GetBeamInfo

获取用户波束信息。

```c
int TRM_GetBeamInfo(uint32_t userId, TRM_BeamInfo* beamInfo);
```

**参数：**

- `userId` - 用户ID
- `beamInfo` - 输出波束信息

**返回值：**

- `TRM_OK` - 成功
- `TRM_ERR_PARAM` - 参数错误
- `TRM_ERR_NO_BEAM` - 波束不存在或已超时

---

### TRM_ClearBeamInfo

清除波束信息。

```c
int TRM_ClearBeamInfo(uint32_t userId);
```

**参数：**

- `userId` - 用户ID，`0xFFFFFFFF`表示清除所有

**返回值：**

- `TRM_OK` - 成功

---

### TRM_GetStats

获取统计信息。

```c
int TRM_GetStats(TRM_Stats* stats);
```

**参数：**

- `stats` - 输出统计信息

**返回值：**

- `TRM_OK` - 成功
- `TRM_ERR_PARAM` - 参数错误

---

### TRM_IsRunning

检查TRM是否运行中。

```c
int TRM_IsRunning(void);
```

**返回值：**

- `1` - 运行中
- `0` - 未运行

---

## Driver模块

### TK8710_Init

初始化Driver层。

```c
int TK8710_Init(const TK8710_Config* config, const TK8710_Callbacks* callbacks);
```

**参数：**

- `config` - 芯片配置结构体指针
- `callbacks` - 回调函数结构体指针

**返回值：**

- `TK8710_OK` - 成功
- `TK8710_ERR_PARAM` - 参数错误
- `TK8710_ERR_STATE` - 状态错误

---

### TK8710_Deinit

反初始化Driver层。

```c
void TK8710_Deinit(void);
```

---

### TK8710_Start

启动Driver，开始芯片工作。

```c
int TK8710_Start(void);
```

**返回值：**

- `TK8710_OK` - 成功
- `TK8710_ERR_STATE` - 状态错误

---

### TK8710_Stop

停止Driver。

```c
int TK8710_Stop(void);
```

**返回值：**

- `TK8710_OK` - 成功
- `TK8710_ERR_STATE` - 状态错误

---

### TK8710_Reset

复位芯片。

```c
int TK8710_Reset(uint8_t resetType);
```

**参数：**

- `resetType` - 复位类型

**返回值：**

- `TK8710_OK` - 成功

---

### TK8710_SetTxUserData

设置用户发送数据（在发送时隙回调中调用）。

```c
int TK8710_SetTxUserData(uint8_t slotIndex, uint32_t userId, 
                         const uint8_t* data, uint16_t len,
                         uint8_t power, const TK8710_BeamInfo* beam);
```

**参数：**

- `slotIndex` - 时隙索引
- `userId` - 用户ID
- `data` - 数据指针
- `len` - 数据长度
- `power` - 发射功率
- `beam` - 波束信息

**返回值：**

- `TK8710_OK` - 成功
- `TK8710_ERR_PARAM` - 参数错误

---

### TK8710_SetTxBrdData

设置广播发送数据。

```c
int TK8710_SetTxBrdData(uint8_t brdIndex, const uint8_t* data, 
                        uint16_t len, uint8_t power);
```

**参数：**

- `brdIndex` - 广播索引
- `data` - 数据指针
- `len` - 数据长度
- `power` - 发射功率

**返回值：**

- `TK8710_OK` - 成功
- `TK8710_ERR_PARAM` - 参数错误

---

### TK8710_ClearTxBuffer

清除发送缓冲区。

```c
int TK8710_ClearTxBuffer(uint8_t slotIndex);
```

**参数：**

- `slotIndex` - 时隙索引，`0xFF`表示清除所有

**返回值：**

- `TK8710_OK` - 成功

---

### TK8710_IsRunning

检查Driver是否运行中。

```c
int TK8710_IsRunning(void);
```

**返回值：**

- `1` - 运行中
- `0` - 未运行

---

### TK8710_IRQHandler

中断处理函数（由GPIO中断回调调用）。

```c
void TK8710_IRQHandler(const TK8710_Callbacks* callbacks);
```

**参数：**

- `callbacks` - 回调函数指针

---

### SPI协议层接口

以下为底层SPI协议接口：

```c
int TK8710_ReadReg(uint16_t addr, uint32_t* value);
int TK8710_WriteReg(uint16_t addr, uint32_t value);
int TK8710_ReadBuffer(uint16_t addr, uint8_t* data, uint16_t len);
int TK8710_WriteBuffer(uint16_t addr, const uint8_t* data, uint16_t len);
```

---

### Driver回调函数类型

```c
// 时隙结束回调
typedef void (*TK8710_OnSlotEnd)(uint8_t slotType, uint8_t slotIndex, uint32_t frameNo);

// 接收数据回调
typedef void (*TK8710_OnRxData)(const TK8710_RxResult* rxResult);

// 发送时隙回调
typedef void (*TK8710_OnTxSlot)(uint8_t slotIndex, uint8_t maxUserCount);

// 错误回调
typedef void (*TK8710_OnError)(int errorCode);
```

---

### Driver数据结构

**TK8710_Config**

```c
typedef struct {
    TK8710_RateMode rateMode;   // 速率模式
    uint8_t antennaCount;       // 天线数量
    uint8_t bcnSlotCount;       // BCN时隙数
    uint8_t brdSlotCount;       // 广播时隙数
    uint8_t ulSlotCount;        // 上行时隙数
    uint8_t dlSlotCount;        // 下行时隙数
    uint32_t rfFreq;            // RF频率
    uint8_t txPower;            // 默认发射功率
} TK8710_Config;
```

**TK8710_BeamInfo**

```c
typedef struct {
    uint32_t userId;            // 用户ID
    uint32_t freq;              // 频率
    uint32_t ahData[16];        // AH数据
    uint64_t pilotPower;        // Pilot功率
} TK8710_BeamInfo;
```

**TK8710_RxResult**

```c
typedef struct {
    uint8_t slotType;           // 时隙类型
    uint8_t slotIndex;          // 时隙索引
    uint8_t userCount;          // 用户数量
    uint32_t frameNo;           // 帧号
    TK8710_RxUserData* users;   // 用户数据数组
} TK8710_RxResult;
```

---

## 日志模块

### TK8710_LogSimpleInit

简单初始化日志系统。

```c
int TK8710_LogSimpleInit(TK8710_LogLevel level, uint32_t moduleMask);
```

**参数：**

- `level` - 日志级别
- `moduleMask` - 模块掩码

**日志级别：**

- `TK8710_LOG_NONE` - 关闭
- `TK8710_LOG_ERROR` - 错误
- `TK8710_LOG_WARN` - 警告
- `TK8710_LOG_INFO` - 信息
- `TK8710_LOG_DEBUG` - 调试
- `TK8710_LOG_TRACE` - 跟踪

**模块掩码：**

- `TK8710_LOG_MOD_CORE` - 核心模块
- `TK8710_LOG_MOD_SPI` - SPI模块
- `TK8710_LOG_MOD_IRQ` - 中断模块
- `TK8710_LOG_MOD_CONFIG` - 配置模块
- `TK8710_LOG_MOD_TRM` - TRM模块
- `TK8710_LOG_MOD_PORT` - 移植层
- `TK8710_LOG_MOD_ALL` - 所有模块

**示例：**

```c
// 显示INFO及以上级别，所有模块
TK8710_LogSimpleInit(TK8710_LOG_INFO, TK8710_LOG_MOD_ALL);

// 只显示ERROR，只显示CORE和IRQ模块
TK8710_LogSimpleInit(TK8710_LOG_ERROR, TK8710_LOG_MOD_CORE | TK8710_LOG_MOD_IRQ);
```

---

### 日志宏

```c
LOG_CORE_ERROR(fmt, ...)   // CORE模块ERROR日志
LOG_CORE_WARN(fmt, ...)    // CORE模块WARN日志
LOG_CORE_INFO(fmt, ...)    // CORE模块INFO日志
LOG_CORE_DEBUG(fmt, ...)   // CORE模块DEBUG日志

LOG_TRM_INFO(fmt, ...)     // TRM模块INFO日志
LOG_IRQ_DEBUG(fmt, ...)    // IRQ模块DEBUG日志
// ... 其他模块类似
```

---

## 调试模块

### TK8710_GetIrqTimeStats

获取中断处理时间统计。

```c
int TK8710_GetIrqTimeStats(uint8_t irqType, TK8710_IrqTimeStats* stats);
```

**参数：**

- `irqType` - 中断类型（0-9）
- `stats` - 输出统计信息

**中断类型：**

- 0: RX_BCN
- 1: BRD_UD
- 2: BRD_DATA
- 3: MD_UD
- 4: MD_DATA
- 5-8: S0-S3
- 9: ACM

---

### TK8710_PrintIrqTimeStats

打印中断时间统计报告。

```c
void TK8710_PrintIrqTimeStats(void);
```

---

### TK8710_ResetIrqCounters

重置中断计数器和时间统计。

```c
void TK8710_ResetIrqCounters(void);
```

---

### TK8710_SetForceProcessAllUsers

设置强制处理所有用户数据（测试用）。

```c
void TK8710_SetForceProcessAllUsers(uint8_t enable);
```

---

## 错误码

| 错误码             | 值 | 说明       |
| ------------------ | -- | ---------- |
| TRM_OK             | 0  | 成功       |
| TRM_ERR_PARAM      | -1 | 参数错误   |
| TRM_ERR_STATE      | -2 | 状态错误   |
| TRM_ERR_TIMEOUT    | -3 | 超时       |
| TRM_ERR_NO_MEM     | -4 | 内存不足   |
| TRM_ERR_NO_BEAM    | -5 | 波束不存在 |
| TRM_ERR_NOT_INIT   | -6 | 未初始化   |
| TRM_ERR_QUEUE_FULL | -7 | 队列已满   |

---

## 数据结构

### TRM_InitConfig

```c
typedef struct {
    TRM_BeamMode beamMode;      // 波束存储模式
    uint32_t beamMaxUsers;      // 最大用户数
    uint32_t beamTimeoutMs;     // 波束超时时间(ms)
    TRM_RateMode rateMode;      // 速率模式
    uint8_t antennaCount;       // 天线数量
    TRM_SlotConfig slotConfig;  // 时隙配置
    uint32_t rfFreq;            // RF频率
    uint8_t txPower;            // 默认发射功率
    TRM_Callbacks callbacks;    // 回调函数
} TRM_InitConfig;
```

### TRM_BeamInfo

```c
typedef struct {
    uint32_t userId;        // 用户ID
    uint32_t freq;          // 频率
    uint32_t ahData[16];    // AH数据
    uint64_t pilotPower;    // Pilot功率
    uint32_t timestamp;     // 更新时间戳
    uint8_t valid;          // 有效标志
} TRM_BeamInfo;
```

### TRM_Stats

```c
typedef struct {
    uint32_t txCount;           // 发送次数
    uint32_t txSuccessCount;    // 发送成功次数
    uint32_t rxCount;           // 接收次数
    uint32_t beamCount;         // 当前波束数量
    uint32_t memAllocCount;     // 内存分配次数
    uint32_t memFreeCount;      // 内存释放次数
} TRM_Stats;
```
