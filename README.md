# TK8710 SDK

TK8710芯片软件开发套件（Software Development Kit），提供完整的软件栈用于控制TK8710无线通信芯片。

## 特性

- **分层架构**：Driver层 + TRM层，职责清晰
- **跨平台支持**：RK3506 Linux、Windows JTOOL
- **完整功能**：SPI通信、中断处理、波束管理、数据收发
- **调试支持**：日志系统、中断时间统计
- **测试覆盖**：88个测试用例全部通过

## 目录结构

```
tk8710_sdk/
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
│       └── trm.h           # TRM主接口
├── src/                    # 源文件
│   ├── common/             # 公共模块实现
│   ├── driver/             # Driver层实现
├── hal/                    # 硬件抽象层
│   └── hal_interface.h     # HAL接口定义
├── port/                   # 平台移植层
│   └── trm/                # TRM层实现
├── port/                   # 平台移植层
│   ├── tk8710_port_rk3506.c  # RK3506 Linux
│   └── tk8710_port_jtool.c   # Windows JTOOL
├── test/                   # 测试用例
├── examples/               # 示例程序
├── cmake/                  # CMake工具链
└── docs/                   # 文档
```

## 快速开始

### 1. 编译（Windows）

```bash
# 编译HAL和示例程序
compile_test.bat

# 编译并运行测试
compile_test_suite.bat
```

### 2. 编译（RK3506 Linux）

```bash
# 使用WSL进入Ubuntu
wsl -d Ubuntu-22.04

# 设置交叉编译环境
source ~/arm-buildroot-linux-gnueabihf_sdk-buildroot/environment-setup

# 使用Makefile编译
make -f Makefile.rk3506

# 或使用CMake编译
./build_rk3506.sh
```

### 3. 基本使用

```c
#include "trm/trm.h"
#include "common/tk8710_log.h"

// 回调函数
void OnRxData(const TRM_RxDataList* rxDataList) {
    // 处理接收数据
}

int main(void) {
    // 初始化日志
    TK8710_LogSimpleInit(TK8710_LOG_INFO, TK8710_LOG_MOD_ALL);
    
    // 配置TRM
    TRM_InitConfig config = {0};
    config.beamMode = TRM_BEAM_MODE_FULL_STORE;
    config.beamMaxUsers = 3000;
    config.rateMode = TRM_RATE_4M;
    config.antennaCount = 8;
    config.slotConfig.ulSlotCount = 4;
    config.slotConfig.dlSlotCount = 4;
    config.callbacks.onRxData = OnRxData;
    
    // 初始化并启动
    TRM_Init(&config);
    TRM_Start();
    
    // 发送数据
    uint8_t data[] = {0x01, 0x02, 0x03};
    TRM_SendData(0x12345678, data, sizeof(data), 20);
    
    // 停止并清理
    TRM_Stop();
    TRM_Deinit();
    
    return 0;
}
```

## API概览

### TRM层（上层应用使用）

| 函数 | 说明 |
|------|------|
| `TRM_Init()` | 初始化TRM |
| `TRM_Deinit()` | 反初始化TRM |
| `TRM_Start()` | 启动TRM |
| `TRM_Stop()` | 停止TRM |
| `TRM_SendData()` | 发送用户数据 |
| `TRM_SetBeamInfo()` | 设置波束信息 |
| `TRM_GetBeamInfo()` | 获取波束信息 |
| `TRM_GetStats()` | 获取统计信息 |

### 日志系统

| 函数 | 说明 |
|------|------|
| `TK8710_LogSimpleInit()` | 简单初始化日志 |
| `TK8710_LogSetLevel()` | 设置日志级别 |
| `TK8710_LogSetModuleMask()` | 设置模块过滤 |
| `LOG_CORE_INFO()` | CORE模块INFO日志 |
| `LOG_TRM_DEBUG()` | TRM模块DEBUG日志 |

### 调试功能

| 函数 | 说明 |
|------|------|
| `TK8710_GetIrqTimeStats()` | 获取中断时间统计 |
| `TK8710_PrintIrqTimeStats()` | 打印中断统计报告 |
| `TK8710_ResetIrqCounters()` | 重置中断计数器 |

## 平台移植

移植到新平台需要实现以下接口（参考`tk8710_port.h`）：

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

## 测试

```bash
# 运行所有测试
compile_test_suite.bat

# 测试结果：88/88 通过
# - Driver单元测试: 25个
# - TRM集成测试: 22个
# - 边界/压力测试: 41个
```

## 示例程序

- `examples/basic_example.c` - 基础使用示例
- `examples/advanced_example.c` - 高级功能演示（日志、调试、波束管理）

## 版本历史

- **v1.0** (2026-02-09)
  - 初始版本
  - Driver层：SPI协议、中断处理、芯片配置
  - TRM层：波束管理、数据收发
  - 平台支持：RK3506 Linux、Windows JTOOL
  - 日志系统、调试功能
  - 88个测试用例

## 许可证

内部使用
