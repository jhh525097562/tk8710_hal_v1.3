# TK8710 Driver API 完整参数与结构体说明

## 概述

本文档提供TK8710 Driver API的完整参数说明，包括所有结构体成员的详细定义、取值范围和使用指南。

---

## 1. 数据类型定义

### 1.1 基础数据类型

#### 返回值定义
```c
#define TK8710_OK           0    // 操作成功
#define TK8710_ERR          1    // 一般错误
#define TK8710_ERR_PARAM    2    // 参数错误
#define TK8710_ERR_STATE    3    // 状态错误
#define TK8710_TIMEOUT      4    // 操作超时
```

#### 最大用户数定义
```c
#define TK8710_MAX_DATA_USERS   128    // 最大数据用户数
#define TK8710_MAX_BRD_USERS    16     // 最大广播用户数
#define TK8710_MAX_ANTENNAS     8      // 最大天线数
```

### 1.2 枚举类型

#### 速率模式枚举
```c
typedef enum {
    TK8710_RATE_MODE_5  = 5,   // 速率模式5: 2KHz信号带宽, 62.5KHz系统带宽, 128用户, 260字节载荷
    TK8710_RATE_MODE_6  = 6,   // 速率模式6: 4KHz信号带宽, 125KHz系统带宽, 128用户, 260字节载荷
    TK8710_RATE_MODE_7  = 7,   // 速率模式7: 8KHz信号带宽, 250KHz系统带宽, 128用户, 260字节载荷
    TK8710_RATE_MODE_8  = 8,   // 速率模式8: 16KHz信号带宽, 500KHz系统带宽, 128用户, 260字节载荷
    TK8710_RATE_MODE_9  = 9,   // 速率模式9: 32KHz信号带宽, 500KHz系统带宽, 64用户, 520字节载荷
    TK8710_RATE_MODE_10 = 10,  // 速率模式10: 64KHz信号带宽, 500KHz系统带宽, 32用户, 520字节载荷
    TK8710_RATE_MODE_11 = 11,  // 速率模式11: 128KHz信号带宽, 500KHz系统带宽, 16用户, 520字节载荷
    TK8710_RATE_MODE_18 = 18,  // 速率模式18: 128KHz信号带宽, 500KHz系统带宽, 16用户, 520字节载荷
} rateMode_e;
```

#### 主从模式枚举
```c
typedef enum {
    TK8710_MODE_SLAVE  = 0,    // 从模式，跟随主设备同步
    TK8710_MODE_MASTER = 1,    // 主模式，提供时隙同步
} msMode_e;
```

#### 工作类型枚举
```c
typedef enum {
    TK8710_WORK_TYPE_MASTER = 1,    // 主工作类型
    TK8710_WORK_TYPE_SLAVE  = 2,    // 从工作类型
} workType_e;
```

#### 工作模式枚举
```c
typedef enum {
    TK8710_WORK_MODE_CONTINUOUS = 1,    // 连续工作模式，持续收发
    TK8710_WORK_MODE_SINGLE     = 2,    // 单次工作模式，执行一次后停止
} workMode_e;
```

#### 复位类型枚举
```c
typedef enum {
    TK8710_RST_STATE_MACHINE = 1,    // 仅复位状态机，保留寄存器配置
    TK8710_RST_ALL           = 2,    // 复位状态机+寄存器，完全复位
} rstType_e;
```

#### 射频类型枚举
```c
typedef enum {
    TK8710_RF_TYPE_1255_1M  = 0,    // SX1255 1MHz射频芯片
    TK8710_RF_TYPE_1255_32M = 1,    // SX1255 32MHz射频芯片
    TK8710_RF_TYPE_1257_32M = 2,    // SX1257 32MHz射频芯片
    TK8710_RF_TYPE_OTHER    = 3,    // 其他射频芯片
} rfType_e;
```

#### 中断类型枚举
```c
typedef enum {
    TK8710_IRQ_RX_BCN       = 0,    // 接收信标中断
    TK8710_IRQ_BRD_UD       = 1,    // 广播用户数据中断
    TK8710_IRQ_BRD_DATA     = 2,    // 广播数据中断
    TK8710_IRQ_MD_UD        = 3,    // 主数据用户中断
    TK8710_IRQ_MD_DATA      = 4,    // 主数据中断
    TK8710_IRQ_S0           = 5,    // 时隙0中断
    TK8710_IRQ_S1           = 6,    // 时隙1中断
    TK8710_IRQ_S2           = 7,    // 时隙2中断
    TK8710_IRQ_S3           = 8,    // 时隙3中断
    TK8710_IRQ_ACM          = 9,    // ACM校准中断
} irqType_e;
```

#### 配置类型枚举
```c
typedef enum {
    TK8710_CFG_TYPE_CHIP_INFO,        // 芯片信息配置
    TK8710_CFG_TYPE_FIX_INFO_LIST,    // 固定信息列表配置
    TK8710_CFG_TYPE_DL_USER_POWER,    // 下行用户功率配置
    TK8710_CFG_TYPE_SLOT_CFG,         // 时隙配置
    TK8710_CFG_TYPE_ADDTL,            // 附加位配置
} TK8710ConfigType;
```

#### 控制类型枚举
```c
typedef enum {
    TK8710_CTRL_TYPE_ACM_START,       // ACM校准开始
    TK8710_CTRL_TYPE_SEND_WAKEUP,     // 发送唤醒信号
} TK8710CtrlType;
```

#### 数据类型定义
```c
#define TK8710_USER_DATA_TYPE_NORMAL    0   // 正常发送: 使用指定信息模式的波束信息
#define TK8710_USER_DATA_TYPE_SLOT1     1   // 与Slot1共用波束信息 (Driver自动生成)

#define TK8710_BRD_DATA_TYPE_NORMAL     0   // 正常广播: 使用Driver自动生成的波束信息
#define TK8710_BRD_DATA_TYPE_SLOT3      1   // 与Slot3共用波束信息
```

---

## 2. 结构体定义

### 2.1 速率模式参数结构体

#### RateModeParams
```c
typedef struct {
    uint8_t  mode;           // 模式编号 (5-11, 18)
    uint8_t  signalBwKHz;    // 信号带宽 (KHz): 2/4/8/16/32/64/128
    uint32_t systemBwKHz;    // 系统带宽 (KHz): 62.5/125/250/500
    uint8_t  maxUsers;       // 最大并发用户数: 128/64/32/16
    uint16_t maxPayloadLen;  // 最大载荷长度: mode5-8=260, mode9/10/11/18=520
} RateModeParams;
```

**参数说明:**
- `mode`: 速率模式编号，对应TK8710_RATE_MODE_*枚举值
- `signalBwKHz`: 信号带宽，单位KHz，支持2/4/8/16/32/64/128
- `systemBwKHz`: 系统带宽，单位KHz，支持62.5/125/250/500
- `maxUsers`: 该速率模式支持的最大并发用户数
- `maxPayloadLen`: 该速率模式支持的最大载荷字节数

### 2.2 芯片配置结构体

#### ChipConfig
```c
typedef struct {
    uint32_t bcn_agc;       // BCN AGC长度, 默认32
    uint32_t interval;      // Intval长度, 默认32
    uint8_t  tx_dly;        // 下行发送时使用前几个上行ram中的信息, 默认1
    uint8_t  tx_fix_info;   // TX是否固定频点, 默认0
    int32_t  offset_adj;    // BCN sync窗口微调, 默认0, 单位us
    int32_t  tx_pre;        // 发送数据窗口调整, 默认0, 单位us
    uint8_t  conti_mode;    // 连续工作模式使能, 默认1
    uint8_t  bcn_scan;      // BCN SCAN模式使能, 默认0
    uint8_t  ant_en;        // 天线使能, 默认0xFF(8天线全使能)
    uint8_t  rf_sel;        // 射频使能, 默认0xFF(8射频全使能)
    uint8_t  tx_bcn_en;     // 发送BCN使能, 默认1
    uint8_t  ts_sync;       // 本地同步, 默认0
    uint8_t  rf_model;      // 射频芯片型号: 1=SX1255, 2=SX1257
    uint8_t  bcnbits;       // 信标标识位, 共5bit
    uint32_t anoiseThe1;    // 用户检测anoiseTh1门限, 默认0
    uint32_t power2rssi;    // RSSI换算系数
    uint32_t irq_ctrl0;     // 中断使能控制寄存器
    uint32_t irq_ctrl1;     // 中断清理控制寄存器
} ChipConfig;
```

**参数说明:**
- `bcn_agc`: 信标AGC处理长度，影响信标接收质量
- `interval`: 间隔长度，用于时序控制
- `tx_dly`: 下行发送延迟，使用前几个上行RAM的信息
- `tx_fix_info`: 发送时是否使用固定频点
- `offset_adj`: 信标同步窗口微调，单位微秒
- `tx_pre`: 发送数据窗口提前量，单位微秒
- `conti_mode`: 连续工作模式使能，1=使能，0=禁用
- `bcn_scan`: 信标扫描模式使能
- `ant_en`: 天线使能位图，bit0-7对应天线0-7
- `rf_sel`: 射频使能位图，bit0-7对应射频0-7
- `tx_bcn_en`: 发送信标使能
- `ts_sync`: 本地同步使能
- `rf_model`: 射频芯片型号选择
- `bcnbits`: 信标标识位，5bit有效范围0-31
- `anoiseThe1`: 用户检测噪声门限
- `power2rssi`: 功率到RSSI的换算系数
- `irq_ctrl0`: 中断使能控制寄存器值
- `irq_ctrl1`: 中断清理控制寄存器值

### 2.3 射频配置结构体

#### TxAdcConfig
```c
typedef struct {
    int16_t i;              // I路直流偏移, 16bit有符号数
    int16_t q;              // Q路直流偏移, 16bit有符号数
} TxAdcConfig;
```

#### ChiprfConfig
```c
typedef struct {
    rfType_e rftype;                        // 射频类型
    uint32_t Freq;                          // 射频中心频率 (Hz)
    uint8_t  rxgain;                        // 射频接收增益 (0-31)
    uint8_t  txgain;                        // 射频发送增益 (0-31)
    TxAdcConfig txadc[TK8710_MAX_ANTENNAS]; // 射频发送直流配置数组
} ChiprfConfig;
```

**参数说明:**
- `rftype`: 射频芯片类型，参考rfType_e枚举
- `Freq`: 射频中心频率，单位Hz，如2412000000表示2.412GHz
- `rxgain`: 接收增益，0-31，数值越大增益越高
- `txgain`: 发射增益，0-31，数值越大功率越高
- `txadc`: 8个天线的I/Q直流偏移配置数组

### 2.4 中断相关结构体

#### TK8710CrcResult
```c
typedef struct {
    uint8_t  userIndex;     // 用户索引 (0-127)
    uint8_t  crcValid;      // CRC有效性: 1=正确, 0=错误
    uint8_t  dataValid;     // 数据有效性: 1=已读取, 0=未读取
    uint8_t  reserved;      // 保留字段
} TK8710CrcResult;
```

#### TK8710UserInfo
```c
typedef struct {
    uint32_t freq;          // 用户频率 (Hz)
    uint32_t pilotPower;    // Pilot功率
    uint32_t ahData[16];    // AH数据: 8天线 × 2(I/Q) = 16个uint32_t
    uint8_t  valid;         // 数据有效性标志
} TK8710UserInfo;
```

#### TK8710IrqResult
```c
typedef struct {
    irqType_e irq_type;             // 中断类型
    int32_t   bcn_freq_offset;      // BCN频率偏移
    uint8_t   rx_bcnbits;           // 接收BCN bit数
    uint8_t   rxbcn_status;         // 接收BCN状态
    
    /* MD_UD中断专用信息 */
    uint8_t  mdUserDataValid;       // MD_UD用户波束信息有效性
    
    /* BRD_UD中断专用信息 */
    uint8_t  brdUserDataValid;      // BRD_UD用户波束信息有效性

    /* MD_DATA中断专用信息 */
    uint8_t  mdDataValid;           // MD_DATA数据有效性
    uint8_t  crcValidCount;         // CRC正确的用户数量
    uint8_t  crcErrorCount;         // CRC错误的用户数量
    uint8_t  maxUsers;              // 当前速率模式最大用户数
    TK8710CrcResult crcResults[128]; // CRC结果数组 (最多128个用户)
 
    /* BRD_DATA中断专用信息 */
    uint8_t  brdDataValid;          // BRD_DATA数据有效性
    uint8_t  brdCrcValidCount;      // CRC正确的用户数量
    uint8_t  brdCrcErrorCount;      // CRC错误的用户数量
    uint8_t  brdMaxUsers;           // 当前速率模式最大用户数
    TK8710CrcResult brdCrcResults[16]; // CRC结果数组 (最多16个用户)

    /* S1时隙自动发送信息 */
    uint8_t  autoTxValid;           // 自动发送数据有效性
    uint8_t  autoTxCount;           // 自动发送用户数量
    
    /* 广播发送信息 */
    uint8_t  brdTxValid;            // 广播发送数据有效性
    uint8_t  brdTxCount;            // 广播发送用户数量
    
    /* 信号质量信息 */
    uint8_t  signalInfoValid;       // 信号信息有效性
    uint8_t  currentRateIndex;      // 当前速率序号 (0-based)
    
} TK8710IrqResult;
```

**关键参数说明:**
- `irq_type`: 当前中断类型，参考irqType_e枚举
- `currentRateIndex`: 当前速率模式索引，用于多速率场景
- `signalInfoValid`: 信号信息有效性标志
- `crcResults`: MD_DATA中断的CRC检查结果数组
- `brdCrcResults`: BRD_DATA中断的CRC检查结果数组

### 2.5 数据缓冲区结构体

#### TK8710RxBuffer
```c
typedef struct {
    uint8_t* data;        // 数据指针
    uint16_t dataLen;     // 数据长度
    uint8_t  valid;       // 数据有效性: 1=有效, 0=无效
    uint8_t  userIndex;   // 用户索引
} TK8710RxBuffer;
```

#### TK8710TxBuffer
```c
typedef struct {
    uint8_t* data;        // 数据指针
    uint16_t dataLen;     // 数据长度
    uint8_t  valid;       // 数据有效性: 1=有效, 0=无效
    uint8_t  userIndex;   // 用户索引
    uint8_t  txPower;     // 发送功率
    uint8_t  dataType;    // 数据类型: 0=正常发送, 1=与Slot1共用波束信息
} TK8710TxBuffer;
```

#### TK8710BrdBuffer
```c
typedef struct {
    uint8_t* data;        // 数据指针
    uint16_t dataLen;     // 数据长度
    uint8_t  valid;       // 数据有效性: 1=有效, 0=无效
    uint8_t  brdIndex;    // 广播用户索引 (0-15)
    uint8_t  txPower;     // 发送功率
    uint8_t  dataType;    // 数据类型: 0=正常广播, 1=与Slot3共用波束信息
} TK8710BrdBuffer;
```

### 2.6 信号质量结构体

#### TK8710SignalInfo
```c
typedef struct {
    uint32_t rssi;        // RSSI值 (负数，单位dBm)
    uint8_t  snr;         // SNR值 (通常需要除以4得到实际dB值)
    uint32_t freq;        // 频率值 (Hz)
    uint8_t  valid;       // 数据有效性: 1=有效, 0=无效
} TK8710SignalInfo;
```

#### SignalQuality
```c
typedef struct {
    uint8_t snr_data[TK8710_MAX_DATA_USERS];    // 数据用户SNR数组
    uint8_t snr_brd[TK8710_MAX_BRD_USERS];      // 广播用户SNR数组
    uint8_t rssi_data[TK8710_MAX_DATA_USERS];   // 数据用户RSSI数组
    uint8_t rssi_brd[TK8710_MAX_BRD_USERS];     // 广播用户RSSI数组
} SignalQuality;
```

### 2.7 时隙配置结构体

#### SlotConfig
```c
typedef struct {
    uint16_t byteLen;       // 时隙字节数
    uint32_t timeLen;       // 时隙时间长度, 单位us (仅查询有效)
    uint32_t centerFreq;    // 时隙中心频点 (Hz)
    uint32_t FreqOffset;    // 相较于中心频点的偏移 (Hz)
    uint32_t da_m;          // 内部DAC参数，用于配置时隙末尾的空闲长度
} SlotConfig;
```

#### slotCfg_t (完整时隙配置)
```c
typedef struct {
    msMode_e   msMode;          // 主从模式
    uint8_t    plCrcEn;         // Payload CRC使能
    uint8_t    rateCount;       // 速率个数，范围1~4，支持最多4个速率模式
    rateMode_e rateModes[4];    // 各时隙速率模式数组，对应s0Cfg-s3Cfg
    uint8_t    brdUserNum;      // 广播时隙用户数, 范围1~16
    float      brdFreq[TK8710_MAX_BRD_USERS]; // 广播时隙发送用户的频率数组
    uint8_t    antEn;           // 天线使能, 默认0xFF(8天线)
    uint8_t    rfSel;           // RF天线选择, 默认0xFF(8天线)
    uint8_t    txAutoMode;      // 下行发送模式: 0=自动发送, 1=指定信息发送
    uint8_t    txBcnEn;         // BCN发送使能
    uint8_t    bcnRotation[TK8710_MAX_ANTENNAS];  // BCN发送使能为0xff时，从bcnRotation中轮流获取当前发送bcn天线
    uint32_t   rx_delay;        // RX delay, 默认0
    uint32_t   md_agc;          // DATA AGC长度, 默认1024
    uint8_t    local_sync;      // 本地同步: 1=产生本地同步信号, 0=接收外部同步脉冲
    
    SlotConfig s0Cfg[4];           // 时隙0(BCN)配置数组
    SlotConfig s1Cfg[4];           // 时隙1配置数组
    SlotConfig s2Cfg[4];           // 时隙2配置数组
    SlotConfig s3Cfg[4];           // 时隙3配置数组
    uint32_t   frameTimeLen;       // TDD周期总时间长度, 单位us
} slotCfg_t;
```

**关键参数说明:**
- `rateCount`: 速率模式数量，1-4个
- `rateModes`: 速率模式数组，对应4个时隙的配置
- `brdUserNum`: 广播用户数量，1-16个
- `brdFreq`: 广播用户频率数组
- `txAutoMode`: 发送模式，0=自动，1=指定信息
- `frameTimeLen`: 完整帧周期时间，单位微秒

### 2.8 波束资源配置

#### fixInfo_t
```c
typedef struct {
    uint32_t* freqList;     // 频点列表, 128个uint32_t
    uint32_t* ahList;       // AH数据列表, 2048个数据点 (128用户 × 8天线 × 2(I/Q))
    uint32_t* pilotPwr;     // Pilot功率, 128个数据点
    uint32_t* anoise;       // Anoise数据, 8个数据点
    uint8_t   dataUserNum;  // 数据用户数, 范围1~128
} fixInfo_t;
```

#### pwrCfg_t
```c
typedef struct {
    uint8_t   start_index;  // 用户起始索引
    uint8_t*  tx_power;     // 功率数组
    size_t    count;        // 写入数量
} pwrCfg_t;
```

### 2.9 其他配置结构体

#### addtlCfg_u (附加位配置)
```c
typedef union {
    struct {
        uint8_t lenEn : 1;  // 物理层携带数据包长度
        uint8_t rfu   : 7;  // 保留
    } bits;
    uint8_t value;
} addtlCfg_u;
```

#### wakeUpParam_t (唤醒参数)
```c
typedef struct {
    wakeUpMode_e wakeUpMode;    // 唤醒模式, 范围1~3
    uint8_t      wakeUpId;      // 唤醒ID
    uint32_t     wakeUpLen;     // 唤醒信号时长, 单位ms
} wakeUpParam_t;
```

#### acmParam_t (ACM校准参数)
```c
typedef struct {
    uint8_t  calibCount;        // 连续校准次数
    uint8_t  snrThreshold;      // 校准信号SNR门限
    uint8_t  snrPassCount;      // SNR大于门限的个数
} acmParam_t;
```

#### TxToneConfig (TX Tone配置)
```c
typedef struct {
    uint32_t freq;          // Tone频点 (Hz)
    uint8_t  gain;          // Tone增益 (0-31)
} TxToneConfig;
```

---

## 3. 回调函数类型

### 3.1 中断回调函数
```c
typedef void (*TK8710IrqCallback)(TK8710IrqResult irqResult);
```

**说明**: 中断发生时调用的回调函数，参数为中断结果结构体

### 3.2 日志回调函数
```c
typedef void (*TK8710LogCallback)(const char* message);
```

**说明**: 日志输出回调函数，用于自定义日志处理

---

## 4. API参数详细说明

### 4.1 芯片初始化与控制API

#### TK8710Init
```c
int TK8710Init(const ChipConfig* initConfig, const TK8710IrqCallback* irqCallback);
```

**参数说明:**
- `initConfig`: 芯片初始化配置，NULL时使用默认配置
- `irqCallback`: 中断回调函数指针，必须提供有效函数

**返回值:**
- 0: 成功
- 1: 失败
- 2: 超时

#### TK8710Startwork
```c
int TK8710Startwork(uint8_t workType, uint8_t workMode);
```

**参数说明:**
- `workType`: 工作类型，1=Master, 2=Slave
- `workMode`: 工作模式，1=连续, 2=单次

#### TK8710RfInit
```c
int TK8710RfInit(const ChiprfConfig* initrfConfig);
```

**参数说明:**
- `initrfConfig`: 射频初始化配置参数

#### TK8710ResetChip
```c
int TK8710ResetChip(uint8_t rstType);
```

**参数说明:**
- `rstType`: 复位类型，1=仅复位状态机, 2=复位状态机+寄存器

### 4.2 配置管理API

#### TK8710SetConfig
```c
int TK8710SetConfig(TK8710ConfigType type, const void* params);
```

**参数说明:**
- `type`: 配置类型，参考TK8710ConfigType枚举
- `params`: 配置参数指针，类型根据type决定

#### TK8710GetConfig
```c
int TK8710GetConfig(TK8710ConfigType type, void* params);
```

**参数说明:**
- `type`: 配置类型，参考TK8710ConfigType枚举
- `params`: 配置参数输出指针，类型根据type决定

### 4.3 数据传输API

#### TK8710SetDownlink1DataWithPower
```c
int TK8710SetDownlink1DataWithPower(uint8_t downlink1Index, const uint8_t* data, 
                                   uint16_t dataLen, uint8_t txPower, uint8_t dataType);
```

**参数说明:**
- `downlink1Index`: 下行1索引 (0-15)
- `data`: 下行1数据指针，不能为NULL
- `dataLen`: 数据长度 (0-512字节)
- `txPower`: 发送功率 (0-31)
- `dataType`: 数据类型，0=正常下行1, 1=与Slot3共用波束信息

#### TK8710SetDownlink2DataWithPower
```c
int TK8710SetDownlink2DataWithPower(uint8_t downlink2Index, const uint8_t* data, 
                                   uint16_t dataLen, uint8_t txPower, uint8_t dataType);
```

**参数说明:**
- `downlink2Index`: 下行2索引 (0-127)
- `data`: 下行2数据指针，不能为NULL
- `dataLen`: 数据长度 (0-512字节)
- `txPower`: 发送功率 (0-31)
- `dataType`: 数据类型，0=正常下行2, 1=与Slot1共用波束信息

#### TK8710SetTxUserInfo
```c
int TK8710SetTxUserInfo(uint8_t userIndex, uint32_t freq, 
                       const uint32_t* ahData, uint64_t pilotPower);
```

**参数说明:**
- `userIndex`: 用户索引 (0-127)
- `freq`: 用户频率 (Hz)
- `ahData`: AH数据数组 (16个32位数据)
- `pilotPower`: Pilot功率

### 4.4 数据接收API

#### TK8710GetRxData
```c
int TK8710GetRxData(uint8_t userIndex, uint8_t** data, uint16_t* dataLen);
```

**参数说明:**
- `userIndex`: 用户索引 (0-127)
- `data`: 数据指针输出，指向Driver内部缓冲区
- `dataLen`: 数据长度输出

**注意**: 获取数据后必须调用TK8710ReleaseRxData释放缓冲区

#### TK8710ReleaseRxData
```c
int TK8710ReleaseRxData(uint8_t userIndex);
```

**参数说明:**
- `userIndex`: 用户索引 (0-127)

#### TK8710GetSignalInfo
```c
int TK8710GetSignalInfo(uint8_t userIndex, uint32_t* rssi, uint8_t* snr, uint32_t* freq);
```

**参数说明:**
- `userIndex`: 用户索引 (0-127)
- `rssi`: RSSI值输出 (dBm)
- `snr`: SNR值输出
- `freq`: 频率值输出 (Hz)

### 4.5 状态查询API

#### TK8710GetSlotCfg
```c
const slotCfg_t* TK8710GetSlotCfg(void);
```

**返回值**: 时隙配置指针，指向Driver内部数据，不要修改

#### TK8710GetRateMode
```c
uint8_t TK8710GetRateMode(void);
```

**返回值**: 当前速率模式

#### TK8710GetWorkType
```c
uint8_t TK8710GetWorkType(void);
```

**返回值**: 当前工作类型

#### TK8710GetBrdUserNum
```c
uint8_t TK8710GetBrdUserNum(void);
```

**返回值**: 当前广播用户数

### 4.6 中断管理API

#### TK8710IrqInit
```c
void TK8710IrqInit(const TK8710IrqCallback* irqCallback);
```

**参数说明:**
- `irqCallback`: 中断回调函数指针

#### TK8710GetIrqStatus
```c
uint32_t TK8710GetIrqStatus(void);
```

**返回值**: 中断状态位图

#### TK8710GetAllIrqCounters
```c
void TK8710GetAllIrqCounters(uint32_t* counters);
```

**参数说明:**
- `counters`: 输出数组指针，至少10个元素

---

## 5. 使用注意事项

### 5.1 参数验证
- 所有指针参数不能为NULL(除非特别说明)
- 数值参数必须在指定范围内
- 数据长度不能超过缓冲区大小限制

### 5.2 内存管理
- 接收数据后必须调用释放函数
- 不要修改Driver返回的数据指针
- 注意缓冲区生命周期管理

### 5.3 线程安全
- Driver API通常不是线程安全的
- 多线程环境下需要外部同步
- 中断回调函数中避免阻塞操作

### 5.4 错误处理
- 所有函数都有返回值，必须检查
- 返回值为0表示成功，非0表示失败
- 根据错误码进行相应处理

### 5.5 配置顺序
1. 首先调用TK8710Init初始化芯片
2. 然后调用TK8710RfInit初始化射频
3. 接着调用TK8710SetConfig配置参数
4. 最后调用TK8710Startwork启动工作

---

## 6. 常用配置示例

### 6.1 基本初始化配置
```c
// 使用默认配置初始化
int ret = TK8710Init(NULL, trmCallback);

// 射频配置
ChiprfConfig rfConfig = {
    .rftype = TK8710_RF_TYPE_1257_32M,
    .Freq = 2412000000,  // 2.412GHz
    .rxgain = 20,
    .txgain = 25,
    .txadc = {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}
};
ret = TK8710RfInit(&rfConfig);
```

### 6.2 时隙配置示例
```c
slotCfg_t slotCfg = {
    .msMode = TK8710_MODE_MASTER,
    .rateCount = 4,
    .rateModes = {TK8710_RATE_MODE_5, TK8710_RATE_MODE_6, 
                  TK8710_RATE_MODE_7, TK8710_RATE_MODE_8},
    .brdUserNum = 4,
    .antEn = 0xFF,
    .rfSel = 0xFF,
    .txAutoMode = 0,
    .txBcnEn = 1,
    .frameTimeLen = 1000000  // 1秒
};
ret = TK8710SetConfig(TK8710_CFG_TYPE_SLOT_CFG, &slotCfg);
```

### 6.3 数据发送示例
```c
// 下行1广播数据发送
uint8_t brdData[] = {0x01, 0x02, 0x03};
ret = TK8710SetDownlink1DataWithPower(0, brdData, sizeof(brdData), 20, 
                                     TK8710_BRD_DATA_TYPE_NORMAL);

// 下行2用户数据发送
uint8_t userData[] = {0x04, 0x05, 0x06};
ret = TK8710SetDownlink2DataWithPower(0, userData, sizeof(userData), 25, 
                                     TK8710_USER_DATA_TYPE_NORMAL);

// 设置用户信息
uint32_t ahData[16] = { /* AH数据 */ };
ret = TK8710SetTxUserInfo(0, 2412000000, ahData, 1000000);
```

---

## 版本信息

- **文档版本**: 2.0
- **API版本**: TK8710 HAL v1.3
- **更新日期**: 2026-02-27
- **文档状态**: 完整版，包含所有参数和结构体说明
