# TK8710 Driver API参数详细说明

## 概述

本文档详细说明TK8710 Driver API接口的参数含义、取值范围和使用注意事项。

---

## 1. 芯片初始化与控制

### `TK8710Init`

```c
int TK8710Init(const ChipConfig* initConfig, const TK8710IrqCallback* irqCallback);
```

#### 参数详细说明

**`initConfig` - 芯片初始化配置**
- **类型**: `const ChipConfig*`
- **含义**: 芯片初始化参数结构体指针
- **取值**: 
  - `NULL`: 使用默认配置
  - 非空指针: 使用自定义配置
- **结构体成员**:
  ```c
  typedef struct {
      uint8_t workMode;        // 工作模式: 1=Master, 2=Slave
      uint8_t chipMode;        // 芯片模式
      uint32_t freq;           // 工作频率
      // ... 其他配置参数
  } ChipConfig;
  ```
- **使用建议**: 通常传入`NULL`使用默认配置，除非有特殊需求

**`irqCallback` - 中断回调函数**
- **类型**: `const TK8710IrqCallback*`
- **含义**: 中断发生时的回调函数指针
- **结构体定义**:
  ```c
  typedef struct {
      void (*onTxSlot)(uint8_t slotIndex, uint8_t maxUserCount, TK8710IrqResult* irqResult);
      void (*onRxSlot)(uint8_t slotIndex, uint8_t maxUserCount, TK8710IrqResult* irqResult);
      void (*onBeacon)(uint8_t slotIndex, TK8710IrqResult* irqResult);
      // ... 其他回调函数
  } TK8710IrqCallback;
  ```
- **使用注意**: 必须提供有效的回调函数指针，通常使用`TRM_GetIrqCallback()`获取

---

### `TK8710Startwork`

```c
int TK8710Startwork(uint8_t workType, uint8_t workMode);
```

#### 参数详细说明

**`workType` - 工作类型**
- **类型**: `uint8_t`
- **含义**: 芯片的工作类型
- **取值**:
  - `TK8710_MODE_MASTER (1)`: 主模式，作为时隙同步的主设备
  - `TK8710_MODE_SLAVE (2)`: 从模式，跟随主设备同步
- **使用场景**: 
  - 单设备测试: 使用`TK8710_MODE_MASTER`
  - 多设备组网: 一个设备为Master，其他为Slave

**`workMode` - 工作模式**
- **类型**: `uint8_t`
- **含义**: 芯片的连续工作模式
- **取值**:
  - `TK8710_WORK_MODE_CONTINUOUS (1)`: 连续工作模式，持续收发
  - `TK8710_WORK_MODE_SINGLE (2)`: 单次工作模式，执行一次后停止
- **使用建议**: 通常使用`TK8710_WORK_MODE_CONTINUOUS`进行持续通信

---

### `TK8710RfInit`

```c
int TK8710RfInit(const ChiprfConfig* initrfConfig);
```

#### 参数详细说明

**`initrfConfig` - 射频初始化配置**
- **类型**: `const ChiprfConfig*`
- **含义**: 射频前端初始化参数
- **结构体成员**:
  ```c
  typedef struct {
      uint32_t freq;           // 射频频率 (Hz)
      uint8_t bandwidth;       // 带宽配置
      uint8_t power;           // 发射功率
      uint8_t gain;            // 接收增益
      // ... 其他射频参数
  } ChiprfConfig;
  ```
- **频率范围**: 根据硬件规格，通常在2.4GHz或5GHz频段
- **功率范围**: 0-31，对应不同的功率等级
- **使用注意**: 需要根据实际硬件和法规要求配置

---

### `TK8710ResetChip`

```c
int TK8710ResetChip(uint8_t rstType);
```

#### 参数详细说明

**`rstType` - 复位类型**
- **类型**: `uint8_t`
- **含义**: 芯片复位的深度
- **取值**:
  - `1`: 仅复位状态机，保留寄存器配置
  - `2`: 复位状态机+寄存器，完全复位
- **使用场景**:
  - 软件故障: 使用类型1
  - 完全重新初始化: 使用类型2
- **使用建议**: 通常使用类型2进行完全复位

---

## 2. 配置管理

### `TK8710SetConfig`

```c
int TK8710SetConfig(TK8710ConfigType type, const void* params);
```

#### 参数详细说明

**`type` - 配置类型**
- **类型**: `TK8710ConfigType`
- **含义**: 要配置的参数类型
- **常用取值**:
  - `TK8710_CFG_TYPE_SLOT_CFG`: 时隙配置
  - `TK8710_CFG_TYPE_RF_CFG`: 射频配置
  - `TK8710_CFG_TYPE_MAC_CFG`: MAC层配置
  - `TK8710_CFG_TYPE_PHY_CFG`: PHY层配置

**`params` - 配置参数**
- **类型**: `const void*`
- **含义**: 指向配置参数结构体的指针
- **具体类型**: 根据`type`参数决定
- **时隙配置示例**:
  ```c
  slotCfg_t slotCfg = {
      .rateCount = 4,
      .rateModes = {TK8710_RATE_MODE_5, TK8710_RATE_MODE_6, 
                    TK8710_RATE_MODE_7, TK8710_RATE_MODE_8},
      .frameTimeLen = 0,
      // ... 其他参数
  };
  TK8710SetConfig(TK8710_CFG_TYPE_SLOT_CFG, &slotCfg);
  ```

---

### `TK8710GetConfig`

```c
int TK8710GetConfig(TK8710ConfigType type, void* params);
```

#### 参数详细说明

**`type` - 配置类型**
- **类型**: `TK8710ConfigType`
- **含义**: 要获取的配置类型
- **取值**: 与`TK8710SetConfig`相同

**`params` - 配置参数输出**
- **类型**: `void*`
- **含义**: 指向接收配置参数的结构体指针
- **使用注意**: 必须确保指向的结构体类型与`type`匹配
- **示例**:
  ```c
  slotCfg_t slotCfg;
  TK8710GetConfig(TK8710_CFG_TYPE_SLOT_CFG, &slotCfg);
  ```

---

## 3. 数据传输

### `TK8710SetDownlink1DataWithPower`

```c
int TK8710SetDownlink1DataWithPower(uint8_t userIndex, const uint8_t* data, 
                                   uint16_t dataLen, uint8_t power, 
                                   TK8710BrdDataType dataType);
```

#### 参数详细说明

**`userIndex` - 用户索引**
- **类型**: `uint8_t`
- **含义**: 下行1广播用户的索引
- **取值范围**: 0-7 (通常最多8个广播用户)
- **使用建议**: 广播数据通常使用索引0

**`data` - 数据指针**
- **类型**: `const uint8_t*`
- **含义**: 要发送的数据缓冲区指针
- **要求**: 不能为NULL，数据长度必须与`dataLen`一致
- **数据内容**: 根据应用协议定义

**`dataLen` - 数据长度**
- **类型**: `uint16_t`
- **含义**: 数据的字节长度
- **取值范围**: 0-512 (根据`TX_DATA_MAX_LEN`定义)
- **使用注意**: 不能超过最大长度限制

**`power` - 发射功率**
- **类型**: `uint8_t`
- **含义**: 发射功率等级
- **取值范围**: 0-31
- **功率对应**: 数值越大，功率越高
- **使用建议**: 根据距离和法规要求调整

**`dataType` - 数据类型**
- **类型**: `TK8710BrdDataType`
- **含义**: 广播数据的类型
- **常用取值**:
  - `TK8710_BRD_DATA_TYPE_NORMAL`: 普通广播数据
  - `TK8710_BRD_DATA_TYPE_CONTROL`: 控制广播数据
  - `TK8710_BRD_DATA_TYPE_EMERGENCY`: 紧急广播数据

---

### `TK8710SetDownlink2DataWithPower`

```c
int TK8710SetDownlink2DataWithPower(uint8_t userIndex, const uint8_t* data, 
                                   uint16_t dataLen, uint8_t power, 
                                   TK8710UserDataType dataType);
```

#### 参数详细说明

**`userIndex` - 用户索引**
- **类型**: `uint8_t`
- **含义**: 下行2用户数据的索引
- **取值范围**: 0-127 (根据硬件最大用户数)
- **使用建议**: 每个用户使用唯一索引

**`data` - 数据指针**
- **类型**: `const uint8_t*`
- **含义**: 用户数据缓冲区指针
- **要求**: 不能为NULL

**`dataLen` - 数据长度**
- **类型**: `uint16_t`
- **含义**: 用户数据的字节长度
- **取值范围**: 0-512

**`power` - 发射功率**
- **类型**: `uint8_t`
- **含义**: 用户数据的发射功率
- **取值范围**: 0-31

**`dataType` - 数据类型**
- **类型**: `TK8710UserDataType`
- **含义**: 用户数据的类型
- **常用取值**:
  - `TK8710_USER_DATA_TYPE_NORMAL`: 普通用户数据
  - `TK8710_USER_DATA_TYPE_ACK`: ACK确认数据
  - `TK8710_USER_DATA_TYPE_CONTROL`: 控制数据

---

### `TK8710SetTxUserInfo`

```c
int TK8710SetTxUserInfo(uint8_t userIndex, uint32_t freq, 
                       const uint8_t* ahData, uint8_t pilotPower);
```

#### 参数详细说明

**`userIndex` - 用户索引**
- **类型**: `uint8_t`
- **含义**: 发送用户信息的索引
- **取值范围**: 0-127
- **使用注意**: 必须与`TK8710SetDownlink2DataWithPower`的索引对应

**`freq` - 频率**
- **类型**: `uint32_t`
- **含义**: 用户的工作频率
- **单位**: Hz
- **取值范围**: 根据硬件支持的频段
- **示例**: 2412000000 (2.412 GHz)

**`ahData` - AH数据**
- **类型**: `const uint8_t*`
- **含义**: 天线权重或波束成形数据
- **长度**: 通常为固定长度(如64字节)
- **来源**: 通常来自波束管理模块

**`pilotPower` - 导频功率**
- **类型**: `uint8_t`
- **含义**: 导频信号的功率等级
- **取值范围**: 0-31
- **作用**: 用于信道估计和同步

---

## 4. 数据接收

### `TK8710GetRxData`

```c
int TK8710GetRxData(uint8_t userIndex, uint8_t** data, uint16_t* dataLen);
```

#### 参数详细说明

**`userIndex` - 用户索引**
- **类型**: `uint8_t`
- **含义**: 要获取接收数据的用户索引
- **取值范围**: 0-127
- **使用注意**: 必须是有效接收到的用户索引

**`data` - 数据指针输出**
- **类型**: `uint8_t**`
- **含义**: 指向接收数据缓冲区的指针的指针
- **输出**: 指向Driver内部数据缓冲区
- **使用注意**: 不要修改返回的数据，使用后必须调用`TK8710ReleaseRxData`

**`dataLen` - 数据长度输出**
- **类型**: `uint16_t*`
- **含义**: 接收数据的字节长度
- **输出**: 实际接收到的数据长度

---

### `TK8710GetSignalInfo`

```c
int TK8710GetSignalInfo(uint8_t userIndex, uint32_t* rssi, uint8_t* snr, uint32_t* freq);
```

#### 参数详细说明

**`userIndex` - 用户索引**
- **类型**: `uint8_t`
- **含义**: 要获取信号信息的用户索引
- **取值范围**: 0-127

**`rssi` - RSSI输出**
- **类型**: `uint32_t*`
- **含义**: 接收信号强度指示
- **单位**: dBm
- **输出**: 信号强度值，负数表示

**`snr` - SNR输出**
- **类型**: `uint8_t*`
- **含义**: 信噪比
- **输出**: 信噪比值，通常需要除以4得到实际dB值

**`freq` - 频率输出**
- **类型**: `uint32_t*`
- **含义**: 接收信号的频率
- **单位**: Hz
- **输出**: 实际接收频率

---

### `TK8710ReleaseRxData`

```c
void TK8710ReleaseRxData(uint8_t userIndex);
```

#### 参数详细说明

**`userIndex` - 用户索引**
- **类型**: `uint8_t`
- **含义**: 要释放数据缓冲区的用户索引
- **取值范围**: 0-127
- **使用注意**: 调用`TK8710GetRxData`后必须调用此函数释放缓冲区

---

## 5. 状态查询

### `TK8710GetSlotCfg`

```c
const slotCfg_t* TK8710GetSlotCfg(void);
```

#### 返回值详细说明

**返回值** - 时隙配置指针
- **类型**: `const slotCfg_t*`
- **含义**: 指向当前时隙配置的指针
- **结构体成员**:
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
- **使用注意**: 返回的指针指向Driver内部数据，不要修改

---

## 6. 中断处理

### `TK8710IrqInit`

```c
int TK8710IrqInit(const TK8710IrqCallback* irqCallback);
```

#### 参数详细说明

**`irqCallback` - 中断回调函数**
- **类型**: `const TK8710IrqCallback*`
- **含义**: 中断回调函数结构体指针
- **结构体成员**: 参见`TK8710Init`的参数说明
- **使用注意**: 必须在调用`TK8710Init`之前调用

---

### `TK8710GpioInit`

```c
int TK8710GpioInit(uint8_t gpioPin, TK8710GpioEdge edge, 
                   const TK8710IrqCallback* callback, void* userData);
```

#### 参数详细说明

**`gpioPin` - GPIO引脚号**
- **类型**: `uint8_t`
- **含义**: 用于中断的GPIO引脚
- **取值范围**: 根据硬件平台定义
- **示例**: 0, 1, 2等

**`edge` - 触发边沿**
- **类型**: `TK8710GpioEdge`
- **含义**: 中断触发条件
- **取值**:
  - `TK8710_GPIO_EDGE_RISING`: 上升沿触发
  - `TK8710_GPIO_EDGE_FALLING`: 下降沿触发
  - `TK8710_GPIO_EDGE_BOTH`: 双边沿触发

**`callback` - 回调函数**
- **类型**: `const TK8710IrqCallback*`
- **含义**: GPIO中断时的回调函数
- **使用注意**: 可以为NULL，仅使用默认中断处理

**`userData` - 用户数据**
- **类型**: `void*`
- **含义**: 传递给回调函数的用户自定义数据
- **取值**: 可以为NULL

---

### `TK8710GpioIrqEnable`

```c
int TK8710GpioIrqEnable(uint8_t gpioPin, uint8_t enable);
```

#### 参数详细说明

**`gpioPin` - GPIO引脚号**
- **类型**: `uint8_t`
- **含义**: 要控制的GPIO引脚
- **取值范围**: 必须与`TK8710GpioInit`中使用的引脚一致

**`enable` - 使能标志**
- **类型**: `uint8_t`
- **含义**: 中断使能控制
- **取值**:
  - `1`: 使能中断
  - `0`: 禁用中断

---

## 使用注意事项

### 1. 参数验证
- 所有指针参数不能为NULL(除非特别说明)
- 数值参数必须在指定范围内
- 数据长度不能超过缓冲区大小

### 2. 内存管理
- 接收数据后必须调用释放函数
- 不要修改Driver返回的数据指针
- 注意缓冲区生命周期

### 3. 线程安全
- Driver API通常不是线程安全的
- 多线程环境下需要外部同步
- 中断回调函数中避免阻塞操作

### 4. 错误处理
- 所有函数都有返回值，必须检查
- 返回值为0表示成功，非0表示失败
- 根据错误码进行相应处理

---

## 版本信息

- **文档版本**: 1.0
- **API版本**: TK8710 HAL v1.3
- **更新日期**: 2026-02-27
