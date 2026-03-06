# TK8710 TRM与Driver层功能与工作流程说明

## 修订记录

| 修订时间   | 修订版本 | 修订描述   | 修订人 |
| ---------- | -------- | ---------- | ------ |
| 2026-03-04 | v1.0     | 初始版本   | AI     |

---

## 1. 系统架构概述

### 1.1 分层架构图

```
┌─────────────────────────────────────────────────────────────┐
│                    应用层 (Application)                      │
│              用户业务逻辑、协议栈、数据处理                    │
├─────────────────────────────────────────────────────────────┤
│                    HAL API层 (8710_hal_api)                  │
│         hal_init / hal_config / hal_start / hal_sendData     │
├─────────────────────────────────────────────────────────────┤
│                    TRM层 (TRM Layer)                         │
│    时域资源管理 / 波束管理 / 数据调度 / 帧管理                 │
├─────────────────────────────────────────────────────────────┤
│                  Driver层 (Driver Layer)                     │
│    芯片控制 / 寄存器操作 / 中断处理 / SPI通信                  │
├─────────────────────────────────────────────────────────────┤
│                   HAL移植层 (Port Layer)                     │
│         SPI接口 / GPIO中断 / 延时函数 / 临界区                 │
├─────────────────────────────────────────────────────────────┤
│                   硬件层 (Hardware)                          │
│                    TK8710芯片 + RF                           │
└─────────────────────────────────────────────────────────────┘
```

### 1.2 模块职责划分

| 层级 | 模块 | 职责 |
|------|------|------|
| HAL API | 8710_hal_api | 统一对外接口，简化使用 |
| TRM | trm_core | 系统初始化、状态管理、回调注册 |
| TRM | trm_beam | 波束信息管理（哈希表存储） |
| TRM | trm_data | 数据发送队列管理、调度 |
| TRM | trm_slot | 时隙配置计算 |
| TRM | trm_log | TRM日志系统 |
| Driver | tk8710_core | 芯片初始化、启动、寄存器操作 |
| Driver | tk8710_config | 配置管理（时隙、RF等） |
| Driver | tk8710_irq | 中断处理（10种中断类型） |
| Driver | tk8710_log | Driver日志系统 |
| Port | tk8710_rk3506 | RK3506平台SPI/GPIO实现 |

---

## 2. Driver层功能详解

### 2.1 核心API接口

#### 2.1.1 初始化与控制

| API | 功能 | 参数 |
|-----|------|------|
| `TK8710Init()` | 芯片初始化 | ChipConfig* |
| `TK8710Start()` | 启动收发 | workType, workMode |
| `TK8710RfConfig()` | RF配置 | ChiprfConfig* |
| `TK8710Reset()` | 芯片复位 | rstType |

#### 2.1.2 数据传输

| API | 功能 | 参数 |
|-----|------|------|
| `TK8710SetTxData()` | 设置发送数据 | downlinkType, index, data, len, txPower, beamType |
| `TK8710SetTxUserInfo()` | 设置用户波束信息 | userIdx, freq, ahInfo, pilotPower |
| `TK8710GetRxUserData()` | 获取接收数据 | userIndex, data, dataLen |
| `TK8710GetRxUserInfo()` | 获取用户波束信息 | userIdx, freq, ahInfo, pilotPower |
| `TK8710GetRxUserSignalQuality()` | 获取信号质量 | userIndex, rssi, snr, freq |

#### 2.1.3 配置管理

| API | 功能 | 配置类型 |
|-----|------|----------|
| `TK8710SetConfig()` | 设置配置 | CHIP_INFO / SLOT_CFG / ADDTL |

### 2.2 中断处理机制

#### 2.2.1 中断类型定义

| 中断编号 | 中断名称 | 触发时机 | 处理内容 |
|----------|----------|----------|----------|
| 0 | RX_BCN_IRQ | BCN检测成功 | 同步信息获取 |
| 1 | BRD_UD_IRQ | FDL时隙UD | 广播用户检测 |
| 2 | BRD_DATA_IRQ | FDL时隙数据 | 广播数据接收 |
| 3 | MD_UD_IRQ | ADL/UL时隙UD | 用户波束信息获取 |
| 4 | MD_DATA_IRQ | ADL/UL时隙数据 | 用户数据接收+CRC校验 |
| 5 | S0_IRQ | Slot0结束 | BCN时隙结束处理 |
| 6 | S1_IRQ | Slot1结束 | 数据时隙1结束+自动发送 |
| 7 | S2_IRQ | Slot2结束 | 数据时隙2结束 |
| 8 | S3_IRQ | Slot3结束 | 数据时隙3结束 |
| 9 | ACM_IRQ | ACM校准结束 | 校准完成处理 |

#### 2.2.2 中断处理流程

```
GPIO中断触发 (PAD_IRQ拉高)
    │
    ▼
TK8710_IRQHandler()
    │
    ├── 1. 读取中断状态寄存器 (irq_res)
    │
    ├── 2. 清除中断状态 (irq_ctrl1)
    │
    └── 3. 遍历处理各中断
            │
            ├── RX_BCN_IRQ → tk8710_handle_rx_bcn()
            ├── BRD_UD_IRQ → tk8710_handle_brd_ud()
            ├── BRD_DATA_IRQ → tk8710_handle_brd_data()
            ├── MD_UD_IRQ → tk8710_handle_md_ud()
            │                   └── 获取用户波束信息 (freq, AH, pilotPower)
            ├── MD_DATA_IRQ → tk8710_handle_md_data()
            │                   └── 读取用户数据 + CRC校验
            ├── S0_IRQ → tk8710_handle_slot0()
            │                   └── BCN天线轮换
            ├── S1_IRQ → tk8710_handle_slot1()
            │                   └── 自动发送处理 + 广播发送
            ├── S2_IRQ → tk8710_handle_slot2()
            ├── S3_IRQ → tk8710_handle_slot3()
            └── ACM_IRQ → tk8710_handle_acm()
```

### 2.3 回调机制

```c
typedef struct {
    TK8710RxDataCallback onRxData;     // 接收数据回调
    TK8710TxSlotCallback onTxSlot;     // 发送时隙回调
    TK8710SlotEndCallback onSlotEnd;   // 时隙结束回调
    TK8710ErrorCallback onError;       // 错误回调
} TK8710DriverCallbacks;
```

### 2.4 数据Buffer管理

| Buffer类型 | 数量 | 用途 |
|------------|------|------|
| TX数据Buffer | 128 | 用户数据发送 |
| TX广播Buffer | 16 | 广播数据发送 |
| RX数据Buffer | 128 | 用户数据接收 |
| RX广播Buffer | 16 | 广播数据接收 |
| 信号质量Buffer | 128 | RSSI/SNR/频率 |

---

## 3. TRM层功能详解

### 3.1 核心API接口

#### 3.1.1 系统管理

| API | 功能 | 参数 |
|-----|------|------|
| `TRM_Init()` | TRM初始化 | TRM_InitConfig* |
| `TRM_Deinit()` | TRM清理 | 无 |
| `TRM_GetStats()` | 获取统计信息 | TRM_Stats* |
| `TRM_LogConfig()` | 日志配置 | level |

#### 3.1.2 数据发送

| API | 功能 | 参数 |
|-----|------|------|
| `TRM_SetTxData`            | 统一发送接口 | downlinkType, userId, data, len, txPower, frameNo, rateMode, beamType |

#### 3.1.3 波束管理

| API | 功能 | 参数 |
|-----|------|------|
| `TRM_GetBeamInfo()` | 获取波束信息 | userId, beamInfo |
| `TRM_SetBeamInfo()` | 设置波束信息 | userId, beamInfo |
| `TRM_ClearBeamInfo()` | 清除波束信息 | userId |

### 3.2 波束管理机制

#### 3.2.1 波束存储模式

| 模式 | 说明 | 适用场景 |
|------|------|----------|
| FULL_STORE | 完整存储在CPU RAM | 大容量、灵活管理 |
| MAPPING | 映射到8710 RAM | 快速访问、低延迟 |

#### 3.2.2 波束信息结构

```c
typedef struct {
    uint32_t userId;        // 用户ID
    uint32_t freq;          // 频率 (26位格式)
    uint32_t ahData[16];    // AH数据: 8天线×2(I/Q)
    uint64_t pilotPower;    // Pilot功率
    uint32_t timestamp;     // 更新时间戳
    uint8_t  valid;         // 有效标志
} TRM_BeamInfo;
```

#### 3.2.3 波束哈希表

- **哈希算法**: 黄金比例哈希 `(userId * 2654435761) % 4096`
- **冲突处理**: 链表法
- **超时机制**: 可配置超时时间，自动清理过期波束
- **最大用户数**: 默认3000

### 3.3 数据调度机制

#### 3.3.1 发送队列

```
用户调用 TRM_SetTxData()
    │
    ▼
广播数据? ──是──▶ 直接调用 TK8710SetTxData()
    │
    否
    ▼
缓存到发送队列 (TRM_SendData)
    │
    ▼
S1中断触发时调度发送
    │
    ▼
TRM_ProcessTxSlot() 处理发送
```

#### 3.3.2 发送模式

| 模式 | 说明 | 配置 |
|------|------|------|
| 自动发送 | 使用上行接收的波束信息 | txAutoMode=0 |
| 指定信息发送 | 使用预设的波束信息 | txAutoMode=1 |

### 3.4 帧管理机制

- **帧号管理**: 全局帧号 `g_trmCurrentFrame`
- **最大帧数**: 可配置 `g_trmMaxFrameCount`
- **帧号更新**: 每个TDD周期自动递增

### 3.5 回调机制

```c
typedef struct {
    TRM_OnRxData      onRxData;       // 接收数据回调
    TRM_OnTxComplete  onTxComplete;   // 发送完成回调
} TRM_Callbacks;
```

---

## 4. 工作流程详解

### 4.1 初始化流程

```
应用层调用 hal_init()
    │
    ▼
┌─────────────────────────────────────┐
│ 1. TK8710Init() - 芯片初始化         │
│    - 配置MAC寄存器                   │
│    - 设置BCN/AGC/Interval参数        │
│    - 配置中断使能                    │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ 2. TK8710RfConfig() - RF配置         │
│    - 配置RF类型和频率                │
│    - 设置TX/RX增益                   │
│    - 配置TX DAC直流                  │
│    - 打开LNA和PA                     │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ 3. TK8710LogConfig() - 日志配置      │
│    - 设置日志级别                    │
│    - 配置模块掩码                    │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ 4. TRM_Init() - TRM初始化            │
│    - 初始化波束管理                  │
│    - 初始化数据队列                  │
│    - 注册Driver回调                  │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ 5. TRM_LogConfig() - TRM日志配置     │
└─────────────────────────────────────┘
```

### 4.2 配置与启动流程

```
应用层调用 hal_config()
    │
    ▼
┌─────────────────────────────────────┐
│ TK8710SetConfig(SLOT_CFG, slotCfg)  │
│    - 配置时隙参数                    │
│    - 设置速率模式                    │
│    - 配置广播用户数                  │
└─────────────────────────────────────┘
    │
    ▼
应用层调用 hal_start()
    │
    ▼
┌─────────────────────────────────────┐
│ TK8710Start(MASTER, CONTINUOUS)     │
│    - 配置工作模式                    │
│    - 使能中断                        │
│    - 触发主动传输                    │
└─────────────────────────────────────┘
```

### 4.3 TDD帧周期工作流程

```
┌─────────────────────────────────────────────────────────────────┐
│                         TDD帧结构                                │
├─────────┬─────────┬─────────┬─────────┬─────────────────────────┤
│ Slot0   │ Slot1   │ Slot2   │ Slot3   │                         │
│ (BCN)   │ (Data)  │ (Data)  │ (Data)  │                         │
├─────────┴─────────┴─────────┴─────────┴─────────────────────────┤
│                                                                  │
│  BCN Slot:  Interval + AGC + BCN                                │
│  Data Slot: Interval + AGC + Payload                            │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘

Master模式工作流程:
═══════════════════

Slot0 (BCN时隙)
    │
    ├── RX_BCN_IRQ → 同步信息获取
    │
    └── S0_IRQ → BCN天线轮换
            │
            ▼
Slot1 (上行数据时隙)
    │
    ├── MD_UD_IRQ → 获取用户波束信息
    │       │
    │       └── 存储到波束表 (TRM_SetBeamInfo)
    │
    ├── MD_DATA_IRQ → 接收用户数据
    │       │
    │       ├── 读取Buffer数据
    │       ├── CRC校验
    │       └── 回调上层 (onRxData)
    │
    └── S1_IRQ → 下行发送处理
            │
            ├── 自动发送模式: 使用上行波束信息
            │       │
            │       └── 遍历发送队列，写入TX Buffer
            │
            └── 广播发送: 写入广播Buffer
            │
            ▼
Slot2/Slot3 (可选数据时隙)
    │
    └── 类似Slot1处理
```

### 4.4 上行数据接收流程

```
MD_UD中断触发
    │
    ▼
┌─────────────────────────────────────┐
│ tk8710_md_ud_get_user_info()        │
│    - 读取用户频率信息                │
│    - 读取AH数据 (8天线×2)            │
│    - 读取Pilot功率                   │
│    - 存储到用户信息Buffer            │
└─────────────────────────────────────┘
    │
    ▼
MD_DATA中断触发
    │
    ▼
┌─────────────────────────────────────┐
│ tk8710_md_data_process()            │
│    - 读取CRC结果寄存器               │
│    - 遍历所有用户                    │
│    - 读取有效用户数据                │
│    - 读取信号质量信息                │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ 回调上层 onRxData()                  │
│    - 构造TRM_RxDataList              │
│    - 包含用户数据和波束信息          │
└─────────────────────────────────────┘
```

### 4.5 下行数据发送流程

```
应用层调用 hal_sendData()
    │
    ▼
┌─────────────────────────────────────┐
│ TRM_SetTxData()                 │
│    - 判断下行类型                    │
│    - 广播: 直接发送                  │
│    - 用户数据: 加入发送队列          │
└─────────────────────────────────────┘
    │
    ▼
S1中断触发
    │
    ▼
┌─────────────────────────────────────┐
│ tk8710_s1_auto_tx_process()         │
│    - 遍历发送队列                    │
│    - 获取用户波束信息                │
│    - 写入TX Buffer                   │
│    - 设置发送功率                    │
│    - 更新user_val寄存器              │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ tk8710_s1_broadcast_tx_process()    │
│    - 遍历广播Buffer                  │
│    - 写入广播数据                    │
│    - 设置广播功率                    │
└─────────────────────────────────────┘
```

---

## 5. 速率模式参数

| 模式 | 信号带宽(KHz) | 系统带宽(KHz) | 最大用户数 | 最大载荷(字节) | 灵敏度(dBm) |
|------|---------------|---------------|------------|----------------|-------------|
| 5    | 2             | 62.5          | 128        | 260            | -146        |
| 6    | 4             | 125           | 128        | 260            | -143        |
| 7    | 8             | 250           | 128        | 260            | -140        |
| 8    | 16            | 500           | 128        | 260            | -137        |
| 9    | 32            | 500           | 64         | 520            | -134        |
| 10   | 64            | 500           | 32         | 520            | -131        |
| 11   | 128           | 500           | 16         | 520            | -128        |
| 18   | 128           | 500           | 16         | 520            | -125        |

---

## 6. 错误码定义

### 6.1 HAL层错误码

| 错误码 | 值 | 说明 |
|--------|-----|------|
| TK8710_HAL_OK | 0 | 成功 |
| TK8710_HAL_ERROR_PARAM | -1 | 参数错误 |
| TK8710_HAL_ERROR_INIT | -2 | 初始化失败 |
| TK8710_HAL_ERROR_CONFIG | -3 | 配置失败 |
| TK8710_HAL_ERROR_START | -4 | 启动失败 |
| TK8710_HAL_ERROR_SEND | -5 | 发送失败 |
| TK8710_HAL_ERROR_STATUS | -6 | 状态查询失败 |
| TK8710_HAL_ERROR_RESET | -7 | 复位失败 |

### 6.2 TRM层错误码

| 错误码 | 值 | 说明 |
|--------|-----|------|
| TRM_OK | 0 | 成功 |
| TRM_ERR_PARAM | -1 | 参数错误 |
| TRM_ERR_STATE | -2 | 状态错误 |
| TRM_ERR_TIMEOUT | -3 | 超时 |
| TRM_ERR_NO_MEM | -4 | 内存不足 |
| TRM_ERR_NOT_INIT | -5 | 未初始化 |
| TRM_ERR_QUEUE_FULL | -6 | 队列满 |
| TRM_ERR_NO_BEAM | -7 | 无波束信息 |
| TRM_ERR_DRIVER | -8 | Driver错误 |

### 6.3 Driver层错误码

| 错误码 | 值 | 说明 |
|--------|-----|------|
| TK8710_OK | 0 | 成功 |
| TK8710_ERR | 1 | 失败 |
| TK8710_ERR_PARAM | 2 | 参数错误 |
| TK8710_ERR_STATE | 3 | 状态错误 |
| TK8710_TIMEOUT | 4 | 超时 |

---

## 7. 使用示例

### 7.1 初始化示例

```c
// 1. 配置HAL初始化参数
TK8710_HalInitConfig halConfig = {
    .tk8710Init = NULL,              // 使用默认配置
    .rfConfig = NULL,                // 使用默认配置
    .driverLogConfig = {
        .logLevel = TK8710_LOG_INFO,
        .moduleMask = 0xFFFFFFFF
    },
    .trmInitConfig = &(TRM_InitConfig){
        .beamMode = TRM_BEAM_MODE_FULL_STORE,
        .beamMaxUsers = 3000,
        .beamTimeoutMs = 3000,
        .maxFrameCount = 100,
        .callbacks = {
            .onRxData = my_rx_callback,
            .onTxComplete = my_tx_callback
        }
    },
    .trmLogConfig = {
        .logLevel = 2,
        .enableStats = true
    }
};

// 2. 初始化HAL
hal_init(&halConfig);

// 3. 配置时隙
slotCfg_t slotConfig = {
    .msMode = TK8710_MODE_MASTER,
    .rateCount = 1,
    .rateModes = {TK8710_RATE_MODE_8},
    .brdUserNum = 4,
    // ... 其他配置
};
hal_config(&slotConfig);

// 4. 启动
hal_start();
```

### 7.2 数据发送示例

```c
// 发送用户数据
uint8_t userData[] = {0x01, 0x02, 0x03, 0x04};
hal_sendData(
    TK8710_DOWNLINK_B,    // 用户数据
    0x12345678,           // 用户ID
    userData,             // 数据
    sizeof(userData),     // 长度
    10,                   // 功率
    0,                    // 帧号
    TK8710_RATE_MODE_8,   // 速率模式
    TK8710_DATA_TYPE_DED  // 指定波束
);

// 发送广播数据
uint8_t brdData[] = {0xAA, 0xBB, 0xCC};
hal_sendData(
    TK8710_DOWNLINK_A,    // 广播数据
    0,                    // 广播索引
    brdData,              // 数据
    sizeof(brdData),      // 长度
    15,                   // 功率
    0,                    // 忽略
    0,                    // 忽略
    TK8710_DATA_TYPE_BRD  // 广播波束
);
```

### 7.3 接收回调示例

```c
void my_rx_callback(const TRM_RxDataList* rxDataList)
{
    printf("接收到 %d 个用户数据, 帧号: %u\n", 
           rxDataList->userCount, rxDataList->frameNo);
    
    for (int i = 0; i < rxDataList->userCount; i++) {
        TRM_RxUserData* user = &rxDataList->users[i];
        printf("用户ID: 0x%08X, 长度: %d, RSSI: %d, SNR: %d\n",
               user->userId, user->dataLen, user->rssi, user->snr);
        
        // 处理用户数据
        process_user_data(user->data, user->dataLen);
        
        // 获取波束信息用于下行发送
        if (user->beam.valid) {
            // 波束信息可用于下行发送
        }
    }
}
```

---

## 8. 附录

### 8.1 文件结构

```
tk8710_hal_v1.3/
├── inc/
│   ├── 8710_hal_api.h          # HAL API头文件
│   ├── driver/
│   │   ├── tk8710_driver_api.h        # Driver API头文件
│   │   ├── tk8710_types.h      # 类型定义
│   │   ├── tk8710_regs.h       # 寄存器定义
│   │   └── tk8710_log.h        # 日志定义
│   └── trm/
│       ├── trm_api.h           # TRM API头文件
│       ├── trm_internal.h      # TRM内部接口
│       └── trm_log.h           # TRM日志定义
├── src/
│   ├── 8710_hal_api.c          # HAL API实现
│   ├── driver/
│   │   ├── tk8710_core.c       # 核心功能
│   │   ├── tk8710_config.c     # 配置管理
│   │   ├── tk8710_irq.c        # 中断处理
│   │   └── tk8710_log.c        # 日志实现
│   └── trm/
│       ├── trm_core.c          # TRM核心
│       ├── trm_beam.c          # 波束管理
│       ├── trm_data.c          # 数据管理
│       └── trm_log.c           # TRM日志
└── port/
    ├── tk8710_hal.h            # HAL移植接口
    └── tk8710_rk3506.c         # RK3506平台实现
```

### 8.2 编译命令

```bash
# 进入WSL环境
wsl -d Ubuntu-22.04

# 进入项目目录
cd /mnt/d/Windsurf_workspace/RK3506/tk8710_hal_v1.3

# 设置交叉编译环境
source ~/arm-buildroot-linux-gnueabihf_sdk-buildroot/environment-setup

# 执行编译
./cmake/build_rk3506.sh
```
