# TK8710 Driver API 参数完整列表

## 概述

本文档提供TK8710 Driver API所有接口参数的完整列表和详细说明。

---

## 1. 芯片初始化与控制

### `TK8710Init`

```c
int TK8710Init(const ChipConfig* initConfig, const TK8710IrqCallback* irqCallback);
```

| 参数名 | 类型 | 说明 | 取值范围 | 默认值 |
|--------|------|------|----------|--------|
| `initConfig` | `const ChipConfig*` | 芯片初始化配置 | NULL或有效指针 | NULL(使用默认配置) |
| `irqCallback` | `const TK8710IrqCallback*` | 中断回调函数指针 | 有效回调指针 | 必须提供 |

**ChipConfig结构体成员**:
```c
typedef struct {
    uint8_t workMode;        // 工作模式: 1=Master, 2=Slave
    uint8_t chipMode;        // 芯片模式
    uint32_t freq;           // 工作频率(Hz)
    // ... 其他配置参数
} ChipConfig;
```

---

### `TK8710Startwork`

```c
int TK8710Startwork(uint8_t workType, uint8_t workMode);
```

| 参数名 | 类型 | 说明 | 取值范围 | 推荐值 |
|--------|------|------|----------|--------|
| `workType` | `uint8_t` | 工作类型 | 1=Master, 2=Slave | 1(Master) |
| `workMode` | `uint8_t` | 工作模式 | 1=连续, 2=单次 | 1(连续) |

---

### `TK8710RfInit`

```c
int TK8710RfInit(const ChiprfConfig* initrfConfig);
```

| 参数名 | 类型 | 说明 | 取值范围 | 注意事项 |
|--------|------|------|----------|----------|
| `initrfConfig` | `const ChiprfConfig*` | 射频初始化配置 | 有效指针 | 必须提供 |

**ChiprfConfig结构体成员**:
```c
typedef struct {
    uint32_t freq;           // 射频频率(Hz)
    uint8_t bandwidth;       // 带宽配置
    uint8_t power;           // 发射功率(0-31)
    uint8_t gain;            // 接收增益
    // ... 其他射频参数
} ChiprfConfig;
```

---

### `TK8710ResetChip`

```c
int TK8710ResetChip(uint8_t rstType);
```

| 参数名 | 类型 | 说明 | 取值范围 | 使用场景 |
|--------|------|------|----------|----------|
| `rstType` | `uint8_t` | 复位类型 | 1=状态机, 2=完全复位 | 2(完全复位) |

---

## 2. 配置管理

### `TK8710SetConfig`

```c
int TK8710SetConfig(TK8710ConfigType type, const void* params);
```

| 参数名 | 类型 | 说明 | 取值范围 | 常用值 |
|--------|------|------|----------|--------|
| `type` | `TK8710ConfigType` | 配置类型 | 枚举值 | SLOT_CFG, RF_CFG |
| `params` | `const void*` | 配置参数指针 | 有效指针 | 依赖type |

**TK8710ConfigType枚举**:
```c
typedef enum {
    TK8710_CFG_TYPE_SLOT_CFG,    // 时隙配置
    TK8710_CFG_TYPE_RF_CFG,      // 射频配置
    TK8710_CFG_TYPE_MAC_CFG,     // MAC层配置
    TK8710_CFG_TYPE_PHY_CFG      // PHY层配置
} TK8710ConfigType;
```

---

### `TK8710GetConfig`

```c
int TK8710GetConfig(TK8710ConfigType type, void* params);
```

| 参数名 | 类型 | 说明 | 取值范围 | 注意事项 |
|--------|------|------|----------|----------|
| `type` | `TK8710ConfigType` | 配置类型 | 同SetConfig | 必须匹配 |
| `params` | `void*` | 配置参数输出 | 有效指针 | 预分配内存 |

---

## 3. 数据传输

### `TK8710SetDownlink1DataWithPower`

```c
int TK8710SetDownlink1DataWithPower(uint8_t userIndex, const uint8_t* data, 
                                   uint16_t dataLen, uint8_t power, 
                                   TK8710BrdDataType dataType);
```

| 参数名 | 类型 | 说明 | 取值范围 | 推荐值 |
|--------|------|------|----------|--------|
| `userIndex` | `uint8_t` | 广播用户索引 | 0-7 | 0 |
| `data` | `const uint8_t*` | 数据指针 | 非NULL | 有效数据 |
| `dataLen` | `uint16_t` | 数据长度 | 0-512 | 实际长度 |
| `power` | `uint8_t` | 发射功率 | 0-31 | 20 |
| `dataType` | `TK8710BrdDataType` | 数据类型 | 枚举 | NORMAL |

**TK8710BrdDataType枚举**:
```c
typedef enum {
    TK8710_BRD_DATA_TYPE_NORMAL,     // 普通广播数据
    TK8710_BRD_DATA_TYPE_CONTROL,    // 控制广播数据
    TK8710_BRD_DATA_TYPE_EMERGENCY   // 紧急广播数据
} TK8710BrdDataType;
```

---

### `TK8710SetDownlink2DataWithPower`

```c
int TK8710SetDownlink2DataWithPower(uint8_t userIndex, const uint8_t* data, 
                                   uint16_t dataLen, uint8_t power, 
                                   TK8710UserDataType dataType);
```

| 参数名 | 类型 | 说明 | 取值范围 | 推荐值 |
|--------|------|------|----------|--------|
| `userIndex` | `uint8_t` | 用户数据索引 | 0-127 | 唯一索引 |
| `data` | `const uint8_t*` | 数据指针 | 非NULL | 有效数据 |
| `dataLen` | `uint16_t` | 数据长度 | 0-512 | 实际长度 |
| `power` | `uint8_t` | 发射功率 | 0-31 | 20 |
| `dataType` | `TK8710UserDataType` | 数据类型 | 枚举 | NORMAL |

**TK8710UserDataType枚举**:
```c
typedef enum {
    TK8710_USER_DATA_TYPE_NORMAL,    // 普通用户数据
    TK8710_USER_DATA_TYPE_ACK,       // ACK确认数据
    TK8710_USER_DATA_TYPE_CONTROL    // 控制数据
} TK8710UserDataType;
```

---

### `TK8710SetTxUserInfo`

```c
int TK8710SetTxUserInfo(uint8_t userIndex, uint32_t freq, 
                       const uint8_t* ahData, uint8_t pilotPower);
```

| 参数名 | 类型 | 说明 | 取值范围 | 单位 |
|--------|------|------|----------|------|
| `userIndex` | `uint8_t` | 用户索引 | 0-127 | - |
| `freq` | `uint32_t` | 工作频率 | 硬件支持频段 | Hz |
| `ahData` | `const uint8_t*` | 天线权重数据 | 64字节 | - |
| `pilotPower` | `uint8_t` | 导频功率 | 0-31 | - |

---

## 4. 数据接收

### `TK8710GetRxData`

```c
int TK8710GetRxData(uint8_t userIndex, uint8_t** data, uint16_t* dataLen);
```

| 参数名 | 类型 | 说明 | 取值范围 | 注意事项 |
|--------|------|------|----------|----------|
| `userIndex` | `uint8_t` | 用户索引 | 0-127 | 有效用户 |
| `data` | `uint8_t**` | 数据指针输出 | 非NULL | Driver内部指针 |
| `dataLen` | `uint16_t*` | 数据长度输出 | 非NULL | 实际长度 |

---

### `TK8710GetSignalInfo`

```c
int TK8710GetSignalInfo(uint8_t userIndex, uint32_t* rssi, uint8_t* snr, uint32_t* freq);
```

| 参数名 | 类型 | 说明 | 取值范围 | 单位 |
|--------|------|------|----------|------|
| `userIndex` | `uint8_t` | 用户索引 | 0-127 | - |
| `rssi` | `uint32_t*` | RSSI输出 | 非NULL | dBm |
| `snr` | `uint8_t*` | SNR输出 | 非NULL | 需除以4 |
| `freq` | `uint32_t*` | 频率输出 | 非NULL | Hz |

---

### `TK8710ReleaseRxData`

```c
void TK8710ReleaseRxData(uint8_t userIndex);
```

| 参数名 | 类型 | 说明 | 取值范围 | 必要性 |
|--------|------|------|----------|--------|
| `userIndex` | `uint8_t` | 用户索引 | 0-127 | 必须调用 |

---

## 5. 状态查询

### `TK8710GetSlotCfg`

```c
const slotCfg_t* TK8710GetSlotCfg(void);
```

| 返回值 | 类型 | 说明 | 注意事项 |
|--------|------|------|----------|
| 配置指针 | `const slotCfg_t*` | 时隙配置 | 不要修改 |

**slotCfg_t结构体成员**:
```c
typedef struct {
    uint8_t rateCount;        // 速率模式数量 (1-8)
    uint8_t rateModes[8];     // 速率模式数组
    uint8_t workType;         // 工作类型 (1=Master, 2=Slave)
    uint8_t brdUserNum;       // 广播用户数量
    uint16_t frameTimeLen;    // 帧时间长度
    uint32_t slotDuration;    // 时隙持续时间
    // ... 其他配置参数
} slotCfg_t;
```

---

## 6. 中断处理

### `TK8710IrqInit`

```c
int TK8710IrqInit(const TK8710IrqCallback* irqCallback);
```

| 参数名 | 类型 | 说明 | 取值范围 | 获取方式 |
|--------|------|------|----------|----------|
| `irqCallback` | `const TK8710IrqCallback*` | 中断回调 | 有效指针 | TRM_GetIrqCallback() |

**TK8710IrqCallback结构体**:
```c
typedef struct {
    void (*onTxSlot)(uint8_t slotIndex, uint8_t maxUserCount, TK8710IrqResult* irqResult);
    void (*onRxSlot)(uint8_t slotIndex, uint8_t maxUserCount, TK8710IrqResult* irqResult);
    void (*onBeacon)(uint8_t slotIndex, TK8710IrqResult* irqResult);
    // ... 其他回调函数
} TK8710IrqCallback;
```

---

### `TK8710GpioInit`

```c
int TK8710GpioInit(uint8_t gpioPin, TK8710GpioEdge edge, 
                   const TK8710IrqCallback* callback, void* userData);
```

| 参数名 | 类型 | 说明 | 取值范围 | 推荐值 |
|--------|------|------|----------|--------|
| `gpioPin` | `uint8_t` | GPIO引脚号 | 硬件定义 | 0,1,2 |
| `edge` | `TK8710GpioEdge` | 触发边沿 | 枚举 | RISING |
| `callback` | `const TK8710IrqCallback*` | 回调函数 | 有效指针或NULL | NULL |
| `userData` | `void*` | 用户数据 | 任意指针 | NULL |

**TK8710GpioEdge枚举**:
```c
typedef enum {
    TK8710_GPIO_EDGE_RISING,    // 上升沿触发
    TK8710_GPIO_EDGE_FALLING,   // 下降沿触发
    TK8710_GPIO_EDGE_BOTH       // 双边沿触发
} TK8710GpioEdge;
```

---

### `TK8710GpioIrqEnable`

```c
int TK8710GpioIrqEnable(uint8_t gpioPin, uint8_t enable);
```

| 参数名 | 类型 | 说明 | 取值范围 | 推荐值 |
|--------|------|------|----------|--------|
| `gpioPin` | `uint8_t` | GPIO引脚号 | 硬件定义 | 同GpioInit |
| `enable` | `uint8_t` | 使能标志 | 0=禁用, 1=使能 | 1 |

---

## 参数使用总结

### 常用参数值

#### **工作模式**
```c
workType: TK8710_MODE_MASTER (1)     // 主模式
workType: TK8710_MODE_SLAVE (2)      // 从模式
workMode: TK8710_WORK_MODE_CONTINUOUS (1)  // 连续工作
workMode: TK8710_WORK_MODE_SINGLE (2)      // 单次工作
```

#### **功率等级**
```c
power: 0-31  // 数值越大功率越高
常用值: 15-25  // 中等功率
```

#### **用户索引范围**
```c
广播用户: 0-7    // 最多8个广播用户
用户数据: 0-127  // 最多128个用户
```

#### **数据长度**
```c
最大长度: 512字节  // TX_DATA_MAX_LEN
推荐长度: <256字节 // 避免缓冲区问题
```

#### **频率范围**
```c
2.4GHz频段: 2400-2500 MHz
5GHz频段: 5000-6000 MHz (根据硬件支持)
```

### 参数验证规则

#### **指针参数**
- 所有指针参数不能为NULL(除非特别说明)
- 数据指针必须有足够的内存空间
- 输出指针必须指向有效的内存区域

#### **数值参数**
- 索引参数必须在指定范围内
- 长度参数不能超过缓冲区大小
- 枚举参数必须是有效的枚举值

#### **内存管理**
- 接收数据后必须调用释放函数
- 不要修改Driver返回的数据指针
- 注意缓冲区的生命周期

### 错误处理

#### **返回值含义**
```c
TK8710_OK (0): 成功
其他值: 失败，具体错误码需要查看定义
```

#### **常见错误**
- 参数无效: 检查参数范围和指针有效性
- 内存不足: 检查缓冲区大小
- 硬件错误: 检查硬件连接和配置

---

## 版本信息

- **文档版本**: 1.0
- **API版本**: TK8710 HAL v1.3
- **更新日期**: 2026-02-27
- **参数总数**: 17个接口，40+个参数
