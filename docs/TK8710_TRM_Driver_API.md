# TK8710 TRM和Driver API接口文档

## 概述

本文档整理了TK8710 HAL层中TRM(Transmission Resource Management)模块和Driver模块的主要API接口，包括接口参数说明和使用方法。

---

## Driver API接口

### 1. 芯片初始化与控制

#### `TK8710Init`
```c
int TK8710Init(const ChipConfig* initConfig, const TK8710IrqCallback* irqCallback);
```
**功能**: 初始化TK8710芯片
**参数**:
- `initConfig`: 初始化配置参数，为NULL时使用默认配置
- `irqCallback`: 中断回调函数指针
**返回值**: 0-成功, 1-失败, 2-超时

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

#### `TK8710GetRateMode`
```c
uint8_t TK8710GetRateMode(void);
```
**功能**: 获取当前速率模式
**返回值**: 速率模式值

#### `TK8710GetWorkType`
```c
uint8_t TK8710GetWorkType(void);
```
**功能**: 获取工作类型
**返回值**: 工作类型值

#### `TK8710GetBrdUserNum`
```c
uint8_t TK8710GetBrdUserNum(void);
```
**功能**: 获取广播用户数量
**返回值**: 用户数量

### 6. 中断处理

#### `TK8710IrqInit`
```c
int TK8710IrqInit(const TK8710IrqCallback* irqCallback);
```
**功能**: 初始化驱动层中断系统
**参数**:
- `irqCallback`: 中断回调函数指针
**返回值**: TK8710_OK-成功, 其他-失败

#### `TK8710GpioInit`
```c
int TK8710GpioInit(uint8_t gpioPin, TK8710GpioEdge edge, const TK8710IrqCallback* callback, void* userData);
```
**功能**: 初始化GPIO中断
**参数**:
- `gpioPin`: GPIO引脚号
- `edge`: 中断触发边沿
- `callback`: 回调函数
- `userData`: 用户数据
**返回值**: TK8710_OK-成功, 其他-失败

#### `TK8710GpioIrqEnable`
```c
int TK8710GpioIrqEnable(uint8_t gpioPin, uint8_t enable);
```
**功能**: 使能GPIO中断
**参数**:
- `gpioPin`: GPIO引脚号
- `enable`: 使能标志 (1=使能, 0=禁用)
**返回值**: TK8710_OK-成功, 其他-失败

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
