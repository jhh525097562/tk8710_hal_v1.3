# TRM发送验证器使用说明

## 概述

`trm_tx_validator`模块是一个可移植的TRM发送逻辑验证工具，用于测试和验证TRM的发送功能。

## 功能特性

1. **自动应答模式**：接收到数据后自动发送应答
2. **周期测试模式**：按固定间隔发送测试数据
3. **手动触发模式**：支持手动触发发送测试
4. **统计功能**：提供详细的发送统计信息
5. **配置灵活**：支持多种参数配置

## 快速开始

### 1. 包含头文件

```c
#include "trm_tx_validator.h"
```

### 2. 初始化验证器

```c
TRM_TxValidatorConfig config = {
    .txPower = 20,                    // 发射功率
    .frameOffset = 3,                 // 帧偏移（接收帧+偏移=发送帧）
    .userIdBase = 0x30000000,         // 用户ID基数
    .enableAutoResponse = true,       // 启用自动应答
    .enablePeriodicTest = true,       // 启用周期测试
    .periodicIntervalFrames = 10      // 周期测试间隔（帧数）
};

int ret = TRM_TxValidatorInit(&config);
if (ret != TRM_OK) {
    printf("验证器初始化失败: %d\n", ret);
}
```

### 3. 在接收回调中调用

```c
static void OnTrmRxData(const TRM_RxDataList* rxDataList)
{
    // ... 现有的接收处理代码 ...
    
    /* 调用验证器 */
    int ret = TRM_TxValidatorOnRxData(rxDataList);
    if (ret != TRM_OK) {
        printf("验证器处理失败: %d\n", ret);
    }
    
    /* 显示统计信息 */
    TRM_TxValidatorStats stats;
    if (TRM_TxValidatorGetStats(&stats) == TRM_OK) {
        printf("发送统计: 总触发=%u, 成功=%u, 失败=%u\n", 
               stats.totalTriggerCount, stats.successSendCount, stats.failedSendCount);
    }
}
```

### 4. 手动触发测试

```c
/* 使用默认数据 */
int ret = TRM_TxValidatorTriggerTest(NULL, 0, 0);

/* 使用自定义数据 */
uint8_t testData[] = {0xAA, 0xBB, 0xCC, 0xDD};
ret = TRM_TxValidatorTriggerTest(testData, sizeof(testData), 0x12345678);
```

### 5. 清理资源

```c
TRM_TxValidatorDeinit();
```

## 配置参数说明

| 参数 | 类型 | 说明 |
|------|------|------|
| txPower | uint8_t | 发射功率 (0-31) |
| frameOffset | uint8_t | 帧偏移，建议3-10 |
| userIdBase | uint32_t | 用户ID基数，用于生成测试用户ID |
| enableAutoResponse | bool | 是否启用自动应答模式 |
| enablePeriodicTest | bool | 是否启用周期测试模式 |
| periodicIntervalFrames | uint16_t | 周期测试间隔（帧数） |

## 工作模式

### 自动应答模式

当`enableAutoResponse`为true时，每接收到一个用户的数据，验证器会：

1. 生成应答数据（包含接收数据的部分内容）
2. 计算应答用户ID（通过位操作生成唯一ID）
3. 在指定帧偏移后发送应答

### 周期测试模式

当`enablePeriodicTest`为true时，验证器会：

1. 每隔指定帧数触发一次发送
2. 使用递增的用户ID
3. 生成伪随机测试数据

## 统计信息

```c
TRM_TxValidatorStats stats;
TRM_TxValidatorGetStats(&stats);

printf("总触发次数: %u\n", stats.totalTriggerCount);
printf("成功发送次数: %u\n", stats.successSendCount);
printf("失败发送次数: %u\n", stats.failedSendCount);
printf("最后错误码: %u\n", stats.lastErrorCode);
printf("当前用户ID: 0x%08X\n", stats.currentUserId);
```

## 移植指南

### 1. 复制文件

将以下文件复制到目标项目：
- `trm_tx_validator.h`
- `trm_tx_validator.c`

### 2. 依赖项

确保目标项目包含：
- TRM库 (`trm.h`, `trm_log.h`)
- 标准C库 (`stdint.h`, `stdbool.h`, `string.h`)

### 3. 集成步骤

1. 在接收回调中添加`TRM_TxValidatorOnRxData()`调用
2. 在系统初始化时调用`TRM_TxValidatorInit()`
3. 在系统清理时调用`TRM_TxValidatorDeinit()`

## 注意事项

1. **帧偏移设置**：建议设置为3-10，确保有足够的处理时间
2. **用户ID冲突**：确保`userIdBase`不与实际用户ID冲突
3. **发送队列**：监控发送队列状态，避免队列满载
4. **资源管理**：确保在程序退出前调用`Deinit()`

## 示例场景

### 场景1：基本应答测试

```c
TRM_TxValidatorConfig config = {
    .txPower = 20,
    .frameOffset = 3,
    .userIdBase = 0x30000000,
    .enableAutoResponse = true,
    .enablePeriodicTest = false,
    .periodicIntervalFrames = 0
};
```

### 场景2：压力测试

```c
TRM_TxValidatorConfig config = {
    .txPower = 25,
    .frameOffset = 2,
    .userIdBase = 0x30000000,
    .enableAutoResponse = true,
    .enablePeriodicTest = true,
    .periodicIntervalFrames = 5  // 每5帧发送一次
};
```

### 场景3：手动测试

```c
TRM_TxValidatorConfig config = {
    .txPower = 15,
    .frameOffset = 5,
    .userIdBase = 0x30000000,
    .enableAutoResponse = false,
    .enablePeriodicTest = false,
    .periodicIntervalFrames = 0
};

// 手动触发
TRM_TxValidatorTriggerTest(testData, dataLen, targetUserId);
```

## 故障排除

### 发送失败率高

1. 检查帧偏移是否过小
2. 检查发送队列是否满载
3. 检查TRM系统状态

### 统计信息异常

1. 调用`TRM_TxValidatorResetStats()`重置统计
2. 检查初始化是否成功
3. 验证配置参数合理性

### 内存问题

1. 确保调用`TRM_TxValidatorDeinit()`
2. 检查是否有重复初始化
3. 验证线程安全性（如果使用多线程）
