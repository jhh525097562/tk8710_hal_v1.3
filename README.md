# TK8710 HAL v1.3

TK8710芯片硬件抽象层（Hardware Abstraction Layer），提供分层独立的软件架构用于控制TK8710无线通信芯片。

## 🏗️ 架构设计

### 分层架构
```
┌─────────────────────────────────────┐
│           应用层 (Application)        │
├─────────────────────────────────────┤
│         TRM层 (TRM Layer)           │  ← 高级API，业务逻辑
├─────────────────────────────────────┤
│       Driver层 (Driver Layer)        │  ← 底层API，硬件控制
├─────────────────────────────────────┤
│         HAL层 (HAL Layer)           │  ← 平台适配，硬件抽象
├─────────────────────────────────────┤
│        硬件层 (Hardware)             │  ← TK8710芯片
└─────────────────────────────────────┘
```

### 模块独立性
- ✅ **TRM层**：独立的业务逻辑层，可单独使用
- ✅ **Driver层**：独立的硬件控制层，可单独使用  
- ✅ **松耦合**：TRM通过Driver API调用硬件功能
- ✅ **可替换**：可替换不同的Driver实现

## 特性

- **分层独立**：TRM API和Driver API完全独立
- **跨平台支持**：RK3506 Linux、Windows JTOOL
- **完整功能**：SPI通信、中断处理、波束管理、数据收发
- **高级特性**：黄金比例哈希、延时释放、队列监控
- **调试支持**：日志系统、中断时间统计、性能监控
- **测试覆盖**：88个测试用例全部通过

## 目录结构

```
tk8710_hal_v1.3/
├── inc/                    # 头文件
│   ├── common/             # 公共模块
│   │   ├── mempool.h       # 内存池
│   │   └── tk8710_log.h    # 日志系统
│   ├── driver/             # Driver层
│   │   ├── tk8710.h        # Driver主接口
│   │   ├── tk8710_api.h    # Driver API
│   │   ├── tk8710_regs.h   # 寄存器定义
│   │   └── tk8710_types.h  # 数据类型定义
│   └── trm/                # TRM层
│       ├── trm.h           # TRM主接口
│       ├── trm_beam.h      # 波束管理
│       ├── trm_data.h      # 数据管理
│       └── trm_config.h    # 配置管理
├── src/                    # 源文件
│   ├── common/             # 公共模块实现
│   ├── driver/             # Driver层实现
│   └── trm/                # TRM层实现
├── port/                   # 平台移植层
│   ├── tk8710_rk3506.c     # RK3506 Linux实现
│   └── tk8710_jtool.c      # Windows JTOOL实现
├── test/                   # 测试用例
│   ├── unit/               # 单元测试
│   ├── integration/        # 集成测试
│   └── example/            # 示例程序
├── cmake/                  # CMake工具链
├── build_rk3506/          # 编译输出（RK3506）
└── docs/                   # 文档
```

## 🚀 快速开始

### 1. 编译（RK3506 Linux）

```bash
# 使用WSL进入Ubuntu
wsl -d Ubuntu-22.04

# 设置交叉编译环境
source ~/arm-buildroot-linux-gnueabihf_sdk-buildroot/environment-setup

# 使用提供的脚本编译
./cmake/build_rk3506.sh

# 或使用Makefile
make -f cmake/Makefile.rk3506
```

### 2. 编译（Windows JTOOL）

```bash
# 使用批处理脚本编译
compile_test.bat

# 编译并运行测试
compile_test_suite.bat
```

## 📚 API使用指南

### TRM API（高级业务层）

**适用场景**：需要完整业务逻辑的应用，包括波束管理、数据收发、队列处理等

```c
#include "trm/trm.h"
#include "trm/trm_log.h"

// 接收回调函数
void OnRxData(const TRM_RxDataList* rxDataList) {
    for (int i = 0; i < rxDataList->count; i++) {
        printf("Received data from user: 0x%08X\n", rxDataList->items[i].userId);
    }
}

int main(void) {
    // 初始化TRM日志
    TRM_LogInit(TRM_LOG_INFO);
    
    // 配置TRM参数
    TRM_InitConfig config = {0};
    config.beamMode = TRM_BEAM_MODE_FULL_STORE;
    config.beamMaxUsers = 3000;
    config.rateMode = TRM_RATE_4M;
    config.antennaCount = 8;
    config.slotConfig.ulSlotCount = 4;
    config.slotConfig.dlSlotCount = 4;
    config.callbacks.onRxData = OnRxData;
    
    // 初始化并启动TRM
    TRM_Init(&config);
    TRM_Start();
    
    // 发送数据（TRM自动处理波束查找、队列管理等）
    uint8_t data[] = {0x01, 0x02, 0x03};
    TRM_SendData(0x12345678, data, sizeof(data), 20, g_trmCurrentFrame);
    
    // 运行主循环
    while (running) {
        // TRM自动处理中断、数据收发、波束管理等
        sleep(1);
    }
    
    // 清理资源
    TRM_Stop();
    TRM_Deinit();
    
    return 0;
}
```

**TRM API 主要函数：**

| 函数 | 说明 | 特性 |
|------|------|------|
| `TRM_Init()` | 初始化TRM系统 | 配置波束、时隙、回调等 |
| `TRM_Start()` | 启动TRM系统 | 启动中断处理、数据收发 |
| `TRM_Stop()` | 停止TRM系统 | 停止所有处理 |
| `TRM_SendData()` | 发送用户数据 | 自动波束查找、队列管理 |
| `TRM_SetBeamInfo()` | 设置波束信息 | 黄金比例哈希优化 |
| `TRM_GetBeamInfo()` | 获取波束信息 | 线程安全读取 |
| `TRM_ScheduleBeamRamRelease()` | 调度延时释放 | 自动内存管理 |
| `TRM_GetStats()` | 获取统计信息 | 性能监控 |

### Driver API（底层硬件层）

**适用场景**：需要直接控制硬件的应用，或实现自定义业务逻辑

```c
#include "driver/tk8710.h"
#include "driver/tk8710_api.h"
#include "common/tk8710_log.h"

// 中断回调函数
void OnIrqEvent(int irqType, void* userData) {
    if (irqType == TK8710_IRQ_RX_DEADLINE) {
        // 处理接收中断
        TK8710_ProcessRxData();
    } else if (irqType == TK8710_IRQ_TX_DEADLINE) {
        // 处理发送中断
        TK8710_ProcessTxData();
    }
}

int main(void) {
    // 初始化Driver日志
    TK8710_LogSimpleInit(TK8710_LOG_INFO, TK8710_LOG_MOD_ALL);
    
    // 初始化Driver
    TK8710_InitConfig config = {0};
    config.spiMode = TK8710_SPI_MODE_0;
    config.spiSpeed = 1000000;  // 1MHz
    config.irqCallback = OnIrqEvent;
    
    TK8710_Init(&config);
    
    // 配置芯片参数
    TK8710_SetFrequency(2400);  // 2.4GHz
    TK8710_SetPowerLevel(20);   // 20dBm
    TK8710_SetAntennaConfig(8); // 8天线
    
    // 直接控制硬件发送
    uint8_t txData[] = {0xAA, 0xBB, 0xCC};
    uint8_t userIndex = 0;
    
    // 设置用户数据
    TK8710SetTxUserDataWithPower(userIndex, txData, sizeof(txData), 20, TK8710_USER_DATA_TYPE_NORMAL);
    
    // 设置用户信息（频率、波束等）
    uint32_t frequency = 2400;
    uint8_t ahData[8] = {0};  // 天线权重数据
    uint8_t pilotPower = 10;
    
    TK8710SetTxUserInfo(userIndex, frequency, ahData, pilotPower);
    
    // 触发发送
    TK8710_TriggerTx();
    
    // 运行主循环
    while (running) {
        // 应用层自定义处理逻辑
        sleep(1);
    }
    
    // 清理资源
    TK8710_Deinit();
    
    return 0;
}
```

**Driver API 主要函数：**

| 函数 | 说明 | 特性 |
|------|------|------|
| `TK8710_Init()` | 初始化Driver | SPI、中断、基础配置 |
| `TK8710_Deinit()` | 反初始化Driver | 清理资源 |
| `TK8710SetTxUserDataWithPower()` | 设置发送数据 | 直接硬件控制 |
| `TK8710SetTxUserInfo()` | 设置用户信息 | 频率、波束、功率 |
| `TK8710SetTxBrdDataWithPower()` | 设置广播数据 | 广播发送 |
| `TK8710_ProcessRxData()` | 处理接收数据 | 手动数据处理 |
| `TK8710_GetChipStatus()` | 获取芯片状态 | 硬件状态监控 |
| `TK8710_SetFrequency()` | 设置频率 | 频率控制 |
| `TK8710_SetPowerLevel()` | 设置功率 | 功率控制 |

## 🔧 核心特性

### 1. 黄金比例哈希波束管理
```c
// 使用黄金比例哈希，减少冲突
static uint32_t BeamHashFunc(uint32_t userId) {
    return (userId * 2654435761U) % BEAM_TABLE_SIZE;
}
```

### 2. 延时释放机制
```c
// 发送完成后自动延时释放波束RAM
TRM_ScheduleBeamRamRelease(userId, 16);  // 16帧后释放
```

### 3. 队列监控
```c
// 实时监控队列状态
TRM: Queue status - BeamRelease: 123/512, TxQueue: 45/256, processed=5 this frame
```

### 4. 线程安全设计
```c
// 读写分离锁机制
TRM_GetBeamInfo()  // 读操作无锁
TRM_SetBeamInfo()  // 写操作加锁
```

## 🎯 使用场景选择

### 选择TRM API当：
- ✅ 需要完整的业务逻辑
- ✅ 需要自动波束管理
- ✅ 需要队列和缓存管理
- ✅ 快速开发应用

### 选择Driver API当：
- ✅ 需要精细硬件控制
- ✅ 实现自定义业务逻辑
- ✅ 性能要求极高
- ✅ 学习硬件协议

## 🧪 测试

```bash
# 运行所有测试
./cmake/build_rk3506.sh

# 测试结果：88/88 通过
# - Driver单元测试: 25个
# - TRM集成测试: 22个  
# - 边界/压力测试: 41个
```

## 📖 示例程序

- `test/example/basic_example.c` - TRM基础使用示例
- `test/example/advanced_example.c` - TRM高级功能演示
- `test/example/test_Driver_TRM_main_3506.c` - Driver+TRM综合测试
- `test/example/gateway_demo.c` - 网关模式演示

## 🚀 平台移植

移植到新平台需要实现HAL接口（参考`port/tk8710_rk3506.c`）：

```c
// SPI接口
int TK8710_Port_SpiInit(void);
int TK8710_Port_SpiTransfer(const uint8_t* tx, uint8_t* rx, uint16_t len);

// GPIO中断  
int TK8710_Port_GpioIrqInit(TK8710_GpioIrqCallback callback, void* userData);

// 系统接口
void TK8710_Port_DelayMs(uint32_t ms);
uint64_t TK8710_Port_GetTimeUs(void);
void TK8710_Port_EnterCritical(void);
void TK8710_Port_ExitCritical(void);

// 内存接口
void* TK8710_Port_Malloc(size_t size);
void TK8710_Port_Free(void* ptr);
```

## 📊 性能特性

- **波束查找**：O(1)平均时间复杂度（黄金比例哈希）
- **内存管理**：自动延时释放，防止内存泄漏
- **并发安全**：读写分离锁，支持高并发
- **队列管理**：循环队列，高效内存使用
- **实时监控**：队列状态、处理统计实时报告

## 📋 版本历史

- **v1.3** (2026-02-25)
  - ✨ 新增：TRM API和Driver API分层独立
  - ✨ 新增：黄金比例哈希波束管理
  - ✨ 新增：延时释放机制
  - ✨ 新增：队列实时监控
  - 🐛 修复：波束查找冲突问题
  - 🐛 修复：并发访问死锁问题
  - 🔧 优化：发送队列大小可配置
  - 🔧 优化：日志系统模块化

- **v1.0** (2026-02-09)
  - 🎉 初始版本发布
  - 📦 Driver层：SPI协议、中断处理、芯片配置
  - 📦 TRM层：波束管理、数据收发
  - 📦 平台支持：RK3506 Linux、Windows JTOOL
  - 📦 日志系统、调试功能
  - 📦 88个测试用例

## 📄 许可证

内部使用

## 🌐 GitHub仓库

https://github.com/jhh525097562/tk8710_hal_v1.3
