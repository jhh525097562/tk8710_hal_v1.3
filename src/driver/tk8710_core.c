/**
 * @file tk8710_core.c
 * @brief TK8710 核心功能实现
 */

#include "../inc/driver/tk8710.h"
#include "../inc/driver/tk8710_regs.h"
#include "../inc/driver/tk8710_rf_regs.h"
#include "driver/tk8710_log.h"
#include "../port/tk8710_hal.h"
#include <string.h>
#include <stddef.h>

/* 默认GPIO中断包装函数 */
static void default_gpio_irq_handler(TK8710IrqResult irqResult)
{
    (void)irqResult;  /* 默认处理不需要IRQ结果参数 */
    
    /* 调用Driver层中断处理函数 */
    TK8710_IRQHandler();
}

/* 速率模式参数查找表 */
static const RateModeParams g_rateModeParams[] = {
    /* mode, signalBwKHz, systemBwKHz(x10), maxUsers, maxPayloadLen */
    { 5,    2,    62500,   128,  260 },  /* 62.5KHz, 最大载荷260字节 */
    { 6,    4,   125000,   128,  260 },  /* 125KHz, 最大载荷260字节 */
    { 7,    8,   250000,   128,  260 },  /* 250KHz, 最大载荷260字节 */
    { 8,   16,   500000,   128,  260 },  /* 500KHz, 最大载荷260字节 */
    { 9,   32,   500000,    64,  520 },  /* 500KHz, 最大载荷520字节 */
    { 10,  64,   500000,    32,  520 },  /* 500KHz, 最大载荷520字节 */
    { 11, 128,   500000,    16,  520 },  /* 500KHz, 最大载荷520字节 */
    { 18, 128,   500000,    16,  520 },  /* 500KHz, 最大载荷520字节 */
};
#define RATE_MODE_COUNT (sizeof(g_rateModeParams) / sizeof(g_rateModeParams[0]))

/* 默认芯片配置 */
static const ChipConfig g_defaultChipConfig = {
    .bcn_agc     = 32,
    .interval    = 32,
    .tx_dly      = 1,
    .tx_fix_info = 0,
    .offset_adj  = 0,
    .tx_pre      = 0,
    .conti_mode  = 1,
    .bcn_scan    = 0,
    .ant_en      = 0xFF,
    .rf_sel      = 0xFF,
    .tx_bcn_en   = 1,
    .ts_sync     = 0,
    .rf_model    = 1,
    .bcnbits     = 0,
    .anoiseThe1  = 0,
    .power2rssi  = 0,
    .irq_ctrl0   = 0x7FF,
    .irq_ctrl1   = 0,
    .spiConfig   = NULL    /* 使用默认SPI配置 */
};

/* 注：工作类型、速率模式、天线使能、RF选择、广播用户数均已迁移到g_slotCfg中 */

/**
 * @brief 根据速率模式获取最大用户数
 * @param rateMode 速率模式
 * @return 最大用户数，失败返回0
 */
static uint8_t tk8710_get_max_users(uint8_t rateMode)
{
    size_t i;
    for (i = 0; i < RATE_MODE_COUNT; i++) {
        if (g_rateModeParams[i].mode == rateMode) {
            return g_rateModeParams[i].maxUsers;
        }
    }
    return 0;
}


/**
 * @brief 获取当前速率模式
 * @return 当前速率模式
 */
uint8_t TK8710GetRateMode(void)
{
    const slotCfg_t* slotCfg = TK8710GetSlotConfig();
    /* 获取当前中断结果中的速率序号，如果未初始化则使用第一个 */
    TK8710IrqResult irqResult;
    int ret = TK8710GetIrqResult(&irqResult);
    uint8_t currentIndex = 0;  /* 默认使用第一个 */
    
    if (ret == TK8710_OK) {
        currentIndex = irqResult.currentRateIndex;
    }
    
    /* 检查速率序号是否有效 */
    if (currentIndex >= slotCfg->rateCount) {
        currentIndex = 0;  /* 无效时使用第一个 */
    }
    
    return slotCfg->rateModes[currentIndex];
}


/**
 * @brief 获取广播用户数
 * @return 当前广播用户数
 */
uint8_t TK8710GetBrdUserNum(void)
{
    const slotCfg_t* slotCfg = TK8710GetSlotConfig();
    return slotCfg->brdUserNum;
}

/**
 * @brief 获取速率模式参数
 * @param rateMode 速率模式
 * @param params 输出参数结构体指针
 * @return 0-成功, 1-失败
 */
int TK8710GetRateModeParams(uint8_t rateMode, RateModeParams* params)
{
    size_t i;
    if (params == NULL) {
        return TK8710_ERR;
    }
    for (i = 0; i < RATE_MODE_COUNT; i++) {
        if (g_rateModeParams[i].mode == rateMode) {
            *params = g_rateModeParams[i];
            return TK8710_OK;
        }
    }
    return TK8710_ERR;
}


/**
 * @brief 获取当前工作类型
 * @return 当前工作类型
 */
uint8_t TK8710GetWorkType(void)
{
    const slotCfg_t* slotCfg = TK8710GetSlotConfig();
    return slotCfg->msMode;
}

/*============================================================================
 * RF寄存器读写内部函数
 *============================================================================*/

/**
 * @brief 写RF寄存器 (内部函数)
 * @param rfSel RF选择 (bit0-7对应RF0-7)
 * @param addr RF寄存器地址 (7位)
 * @param data 写入数据
 * @return 0-成功, 非0-失败
 */
static int tk8710_rf_write(uint8_t rfSel, uint16_t addr, uint32_t data)
{
    int ret;
    s_init_9 init9;
    s_spi_cfg1 spiCfg1;
    s_spi_cfg2 spiCfg2;
    
    /* 1. 写mac->init_9 选择RF: (rf_sel << 8) + 0xff */
    if (rfSel < 0xff) {
        init9.data = ((uint32_t)rfSel << 8) | 0xff;
        ret = TK8710SpiWriteReg(MAC_BASE + offsetof(struct mac, init_9), &init9.data, 1);
        if (ret != 0) return TK8710_ERR;
    }
    
    /* 2. 写rx_fe->spi_cfg1: (0x1<<31)|(1<<15)|(addr<<8)|data */
    spiCfg1.data = ((uint32_t)0x1 << 31) | ((uint32_t)1 << 15) | ((uint32_t)(addr & 0x7f) << 8) | (data & 0xff);
    ret = TK8710SpiWriteReg(RX_FE_BASE + offsetof(struct rx_top, spi_cfg1), &spiCfg1.data, 1);
    if (ret != 0) return TK8710_ERR;
    
    /* 3. 写rx_fe->spi_cfg2 发送写命令: 0x80<<8 | 0x01 */
    spiCfg2.data = (0x80 << 8) | 0x01;
    ret = TK8710SpiWriteReg(RX_FE_BASE + offsetof(struct rx_top, spi_cfg2), &spiCfg2.data, 1);
    if (ret != 0) return TK8710_ERR;
    
    return TK8710_OK;
}

/**
 * @brief 读RF寄存器 (内部函数)
 * @param rfSel RF选择 (bit0-7对应RF0-7)
 * @param addr RF寄存器地址 (7位)
 * @param data 读取数据输出
 * @return 0-成功, 非0-失败
 */
static int tk8710_rf_read(uint8_t rfSel, uint16_t addr, uint32_t* data)
{
    int ret;
    s_init_9 init9;
    s_spi_cfg1 spiCfg1;
    s_spi_cfg2 spiCfg2;
    s_spi_res0 spiRes0;
    
    /* 1. 写mac->init_9 选择RF: (1 << (rf_sel + 8)) + 0xff */
    init9.data = ((uint32_t)rfSel << 8) | 0xff;
    ret = TK8710SpiWriteReg(MAC_BASE + offsetof(struct mac, init_9), &init9.data, 1);
    if (ret != 0) return TK8710_ERR;
    
    /* 2. 写rx_fe->spi_cfg1 设置地址: (addr&0x7f)<<8 */
    spiCfg1.data = (addr & 0x7f) << 8;
    ret = TK8710SpiWriteReg(RX_FE_BASE + offsetof(struct rx_top, spi_cfg1), &spiCfg1.data, 1);
    if (ret != 0) return TK8710_ERR;
    
    /* 3. 写rx_fe->spi_cfg2 发送读命令: 0x80<<8 | 0x01 */
    spiCfg2.data = (0x80 << 8) | 0x01;
    ret = TK8710SpiWriteReg(RX_FE_BASE + offsetof(struct rx_top, spi_cfg2), &spiCfg2.data, 1);
    if (ret != 0) return TK8710_ERR;
    
    /* 4. 读rx_fe->spi_res0 获取结果 */
    ret = TK8710SpiReadReg(RX_FE_BASE + offsetof(struct rx_top, spi_res0), &spiRes0.data, 1);
    if (ret != 0) return TK8710_ERR;
    
    *data = spiRes0.b.spi_rd_data;
    return TK8710_OK;
}

/**
 * @brief 初始化TK8710芯片
 * @param initConfig 初始化配置参数，为NULL时使用默认配置
 * @return 0-成功, 1-失败, 2-超时
 */
int TK8710Init(const ChipConfig* initConfig)
{
    
    int ret;
    s_init_0 init0;
    s_init_1 init1;
    s_init_2 init2;
    s_init_3 init3;
    s_init_4 init4;
    s_init_5 init5;
    s_init_9 init9;
    s_init_11 init11;
    s_init_18 init18;
    s_irq_ctrl0 irqCtrl0;
    s_irq_ctrl1 irqCtrl1;
    const ChipConfig* cfg = initConfig ? initConfig : &g_defaultChipConfig;
    
    
    /* 初始化默认日志系统（如果尚未初始化） */
    TK8710LogConfig_t defaultLogConfig = {
        .level = TK8710_LOG_INFO,
        .module_mask = TK8710_LOG_MODULE_ALL,
        .callback = NULL,
        .enable_timestamp = 1,
        .enable_module_name = 1
    };
    TK8710LogInit(&defaultLogConfig);
    
    /* 初始化SPI接口 */
    SpiConfig spiConfigToUse;
    if (cfg->spiConfig != NULL) {
        /* 使用自定义SPI配置 */
        spiConfigToUse = *cfg->spiConfig;
        TK8710_LOG_CORE_INFO("Using custom SPI config: speed=%u, mode=%u, bits=%u", 
                             spiConfigToUse.speed, spiConfigToUse.mode, spiConfigToUse.bits);
    } else {
        /* 使用默认SPI配置 */
        spiConfigToUse.speed = 16000000;    /* 16MHz */
        spiConfigToUse.mode = 0;           /* Mode 0 */
        spiConfigToUse.bits = 8;           /* 8位数据 */
        spiConfigToUse.lsb_first = 0;      /* MSB优先 */
        spiConfigToUse.cs_pin = 0;          /* CS引脚0 */
        TK8710_LOG_CORE_INFO("Using default SPI config: 16MHz, Mode0, MSB");
    }
    TK8710SpiInit(&spiConfigToUse);
    
    /* 初始化默认GPIO中断 */
    TK8710GpioInit(0, TK8710_GPIO_EDGE_RISING, default_gpio_irq_handler, NULL);
    
    /* 使能GPIO中断 */
    TK8710GpioIrqEnable(0, 1);
    
    TK8710_LOG_CORE_INFO("TK8710 initializing...");

    /* 注意：中断回调由TK8710IrqInit设置，这里不需要重复设置 */

    /* 配置 init_0: bcn_agc, interval, tx_freq_dly */
    init0.data = 0;
    init0.b.bcn_agc = cfg->bcn_agc & 0xFFF;
    init0.b.interval = cfg->interval & 0xFF;
    init0.b.tx_freq_dly = cfg->tx_dly & 0x07;  /* 从cfg获取tx_dly */
    ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, init_0), init0.data);
    if (ret != TK8710_OK) return ret;

    /* init_1, init_2, init_3, init_4, init_18 配置已移至 TK8710SetConfig TK8710_CFG_TYPE_SLOT_CFG */

    /* 配置 init_5: offset_adj, tx_pre, conti_mode, conti_scan */
    init5.data = 0;
    init5.b.offset_adj = cfg->offset_adj & 0x1FFF;
    init5.b.tx_pre = cfg->tx_pre & 0xFFF;
    init5.b.conti_mode = cfg->conti_mode & 0x01;
    init5.b.conti_scan = cfg->bcn_scan & 0x01;
    ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, init_5), init5.data);
    if (ret != TK8710_OK) return ret;

    /* 配置 init_9: ant_en, rf_sel, tx_bcn_en */
    init9.data = 0;
    init9.b.ant_en = cfg->ant_en;
    init9.b.rf_sel = cfg->rf_sel;
    init9.b.tx_bcn_en = cfg->tx_bcn_en;
    /* 更新antEn、rfSel和txBcnEn到g_slotCfg */
    {
        slotCfg_t* slotCfg = (slotCfg_t*)TK8710GetSlotConfig();
        slotCfg->antEn = cfg->ant_en;
        slotCfg->rfSel = cfg->rf_sel;
        slotCfg->txBcnEn = cfg->tx_bcn_en;
    }
    ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, init_9), init9.data);
    if (ret != TK8710_OK) return ret;

    /* 配置 init_11: rf_type */
    init11.data = 0;
    init11.b.rf_type = cfg->rf_model & 0x03;
    ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, init_11), init11.data);
    if (ret != TK8710_OK) return ret;

    /* init_18 配置已移至 TK8710SetConfig TK8710_CFG_TYPE_SLOT_CFG */

    /* 配置 irq_ctrl0: 中断使能 (注释掉，由TK8710Start配置) */
    /* irqCtrl0.data = cfg->irq_ctrl0; */
    /* ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, irq_ctrl0), irqCtrl0.data); */
    /* if (ret != TK8710_OK) return ret; */

    /* 配置 irq_ctrl1: 中断清理 */
    irqCtrl1.data = cfg->irq_ctrl1;
    ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, irq_ctrl1), irqCtrl1.data);
    if (ret != TK8710_OK) return ret;

    /* 配置 init_13: 初始化为0 */
    ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, init_13), 0);
    if (ret != TK8710_OK) return ret;

    /* 配置 init_14: 初始化为0 */
    ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, init_14), 0);
    if (ret != TK8710_OK) return ret;

    /* 配置 init_15: 初始化为0 */
    ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, init_15), 0);
    if (ret != TK8710_OK) return ret;

    /* 配置 init_16: 初始化为0 */
    ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, init_16), 0);
    if (ret != TK8710_OK) return ret;

    /* 配置 init_17: 初始化为0 */
    ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, init_17), 0);
    if (ret != TK8710_OK) return ret;
    
    TK8710_LOG_CORE_INFO("TK8710 initialized successfully");
    return TK8710_OK;
}

/**
 * @brief 芯片进入收发状态
 * @param workType 工作类型: 1=Master, 2=Slave
 * @param workMode 工作模式: 1=连续, 2=单次
 * @return 0-成功, 1-失败, 2-超时
 */
int TK8710Start(uint8_t workType, uint8_t workMode)
{
    int ret;
    s_init_5 init5;
    s_trx_trig0 trig0;
    s_trx_trig1 trig1;
    
    /* 先读取init_5寄存器，设置conti_mode */
    ret = TK8710ReadReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, init_5), &init5.data);
    if (ret != TK8710_OK) return ret;
    
    /* 设置连续模式: 1=连续, 0=单次 */
    init5.b.conti_mode = (workMode == TK8710_WORK_MODE_CONTINUOUS) ? 1 : 0;
    
    ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, init_5), init5.data);
    if (ret != TK8710_OK) return ret;
    
    /* 根据工作类型启动传输 */
    if (workType == TK8710_MODE_MASTER) {
        /* Master模式: 配置trx_trig0寄存器 (0x74) 启动主动传输 */
        {
            slotCfg_t* slotCfg = (slotCfg_t*)TK8710GetSlotConfig();
            slotCfg->msMode = TK8710_MODE_MASTER;
            
            /* 配置中断使能 */
            {
                s_irq_ctrl0 irqCtrl0;
                irqCtrl0.data = 0xFFFF;  /* 默认全部关闭 */
                
                /* Master模式中断配置 */
                irqCtrl0.b.s0_irq_mask = 0;  /* S0中断使能 */

                if (slotCfg->s1Cfg[0].byteLen > 0) {
                    irqCtrl0.b.s1_irq_mask = 0;  /* S1中断使能 */
                }
                
                if (slotCfg->s2Cfg[0].byteLen > 0) {
                    irqCtrl0.b.md_ud_irq_mask = 0;  /* MD UD中断使能 */
                    irqCtrl0.b.md_irq_mask = 0;     /* MD中断使能 */
                }
                
                if (slotCfg->s3Cfg[0].byteLen > 0) {
                    irqCtrl0.b.s3_irq_mask = 0;  /* S3中断使能 */
                }
                
                ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, irq_ctrl0), irqCtrl0.data);
                if (ret != TK8710_OK) return ret;
            }
        }
        trig0.data = 0;
        trig0.b.active_trans = 1;
        ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, trx_trig0), trig0.data);
    } else if (workType == TK8710_MODE_SLAVE) {
        /* Slave模式: 配置trx_trig1寄存器 (0x78) 启动被动传输 */
        {
            slotCfg_t* slotCfg = (slotCfg_t*)TK8710GetSlotConfig();
            slotCfg->msMode = TK8710_MODE_SLAVE;
            
            /* 配置中断使能 */
            {
                s_irq_ctrl0 irqCtrl0;
                irqCtrl0.data = 0xFFFF;  /* 默认全部关闭 */
                
                /* Slave模式中断配置 */
                irqCtrl0.b.rxbcn_irq_mask = 0;  /* RX BCN中断使能 */
                
                if (slotCfg->s1Cfg[0].byteLen > 0) {
                    irqCtrl0.b.brd_ud_irq_mask = 0;  /* BRD UD中断使能 */
                    irqCtrl0.b.brd_irq_mask = 0;     /* BRD中断使能 */
                }
                
                if (slotCfg->s2Cfg[0].byteLen > 0) {
                    irqCtrl0.b.s2_irq_mask = 0;  /* S2中断使能 */
                }
                
                if (slotCfg->s3Cfg[0].byteLen > 0) {
                    irqCtrl0.b.md_ud_irq_mask = 0;  /* MD UD中断使能 */
                    irqCtrl0.b.md_irq_mask = 0;     /* MD中断使能 */
                }
                
                ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, irq_ctrl0), irqCtrl0.data);
                if (ret != TK8710_OK) return ret;
            }
        }
        trig1.data = 0;
        trig1.b.passive_trans = 1;
        ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, trx_trig1), trig1.data);
    } else {
        TK8710_LOG_CORE_ERROR("Invalid work type: %d", workType);
        return TK8710_ERR;
    }
    
    TK8710_LOG_CORE_INFO("Work started: type=%d, mode=%d", workType, workMode);
    return ret;
}

/**
 * @brief 初始化芯片连接射频
 * @param initrfConfig 射频初始化配置参数
 * @return 0-成功, 1-失败, 2-超时
 */
int TK8710RfConfig(const ChiprfConfig* initrfConfig)
{
    int ret;
    int i;
    s_tx_config_29 txConfig29;
    uint32_t txFeAddr;
    uint8_t rfSel;
    
    TK8710_LOG_CORE_INFO("Starting RF initialization...");
    
    if (initrfConfig == NULL) {
        TK8710_LOG_CORE_ERROR("RF init config is NULL");
        return TK8710_ERR;
    }
    
    /* 从g_slotCfg获取RF选择 */
    rfSel = TK8710GetSlotConfig()->rfSel;
    TK8710_LOG_CORE_INFO("RF config: type=%d, freq=%u Hz, rxgain=0x%02X, txgain=0x%02X, rfSel=0x%02X", 
                        initrfConfig->rftype, initrfConfig->Freq, initrfConfig->rxgain, initrfConfig->txgain, rfSel);
    
    /* 配置mac.init_11: rf_type (射频数字接口类型) */
    {
        s_init_11 init11;
        init11.data = 0;
        init11.b.rf_type = initrfConfig->rftype & 0x03;
        ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, init_11), init11.data);
        if (ret != TK8710_OK) {
            TK8710_LOG_CORE_ERROR("Failed to set RF type: %d", ret);
            return ret;
        }
        TK8710_LOG_CORE_DEBUG("RF type set to: %d", init11.b.rf_type);
    }
    
    /* 配置每根天线的txadc (i/q直流) */
    /* 每根天线的tx_fe地址 = TX_FE_BASE + antenna * 0x1000 */
    TK8710_LOG_CORE_DEBUG("Configuring TX ADC DC offsets for %d antennas", TK8710_MAX_ANTENNAS);
    for (i = 0; i < TK8710_MAX_ANTENNAS; i++) {
        txFeAddr = TX_FE_BASE + i * 0x1000;
        
        /* 读取tx_config_29寄存器 */
        ret = TK8710ReadReg(TK8710_REG_TYPE_GLOBAL, 
                            txFeAddr + offsetof(struct tx_dac_if, tx_config_29), 
                            &txConfig29.data);
        if (ret != TK8710_OK) {
            TK8710_LOG_CORE_ERROR("Failed to read TX config for antenna %d: %d", i, ret);
            return ret;
        }
        
        /* 配置tx_dci和tx_dcq (各16bit) */
        txConfig29.b.tx_dci = initrfConfig->txadc[i].i & 0xFFFF;
        txConfig29.b.tx_dcq = initrfConfig->txadc[i].q & 0xFFFF;
        
        /* 写回tx_config_29寄存器 */
        ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, 
                             txFeAddr + offsetof(struct tx_dac_if, tx_config_29), 
                             txConfig29.data);
        if (ret != TK8710_OK) {
            TK8710_LOG_CORE_ERROR("Failed to write TX DC offset for antenna %d: %d", i, ret);
            return ret;
        }
        
        TK8710_LOG_CORE_DEBUG("Antenna %d: DC I=0x%04X, DC Q=0x%04X", i, txConfig29.b.tx_dci, txConfig29.b.tx_dcq);
    }
    
    /* RF基础配置 */
    TK8710_LOG_CORE_DEBUG("Starting RF basic configuration sequence...");
    
    /* 1. RF关闭/复位序列 - 首次上电需要更完整的复位 */
    /* 复位序列1: 完全关闭 */
    ret = tk8710_rf_write(rfSel, RF_CMD_CLOSE_0 >> 8, RF_CMD_CLOSE_0 & 0xFF);
    if (ret != TK8710_OK) {
        TK8710_LOG_CORE_ERROR("RF close 0 sequence failed: %d", ret);
        return ret;
    }
    TK8710_LOG_CORE_DEBUG("RF close 0 sequence completed");
    
    /* 复位序列2: 复位状态 */
    ret = tk8710_rf_write(rfSel, RF_CMD_CLOSE_1 >> 8, RF_CMD_CLOSE_1 & 0xFF);
    if (ret != TK8710_OK) {
        TK8710_LOG_CORE_ERROR("RF close 1 sequence failed: %d", ret);
        return ret;
    }
    TK8710_LOG_CORE_DEBUG("RF close 1 sequence completed");
    
    /* 等待复位稳定 */
    usleep(10000);  /* 10ms等待复位完成 */

    /* 2. 采样率配置 (Sampling Rate) */
    ret = tk8710_rf_write(rfSel, RF_CMD_RX_FILTER >> 8, RF_CMD_RX_FILTER & 0xFF);
    if (ret != TK8710_OK) {
        TK8710_LOG_CORE_ERROR("RX filter configuration failed: %d", ret);
        return ret;
    }

    /* 3. DIG_BRIDGE设置 */
    ret = tk8710_rf_write(rfSel, RF_CMD_RX_BW >> 8, RF_CMD_RX_BW & 0xFF);
    if (ret != TK8710_OK) {
        TK8710_LOG_CORE_ERROR("DIG_BRIDGE setting failed: %d", ret);
        return ret;
    }
    
    /* 4. TX DAC带宽 */
    ret = tk8710_rf_write(rfSel, RF_CMD_TX_DAC_BW >> 8, RF_CMD_TX_DAC_BW & 0xFF);
    if (ret != TK8710_OK) {
        TK8710_LOG_CORE_ERROR("TX DAC bandwidth configuration failed: %d", ret);
        return ret;
    }
    
    /* 5. CLK设置: bit[1] output clock enabled on pad CLK_OUT */
    ret = tk8710_rf_write(rfSel, RF_CMD_CLK_SETTING >> 8, RF_CMD_CLK_SETTING & 0xFF);
    if (ret != TK8710_OK) {
        TK8710_LOG_CORE_ERROR("Clock setting failed: %d", ret);
        return ret;
    }
    TK8710_LOG_CORE_DEBUG("RF basic configuration completed");
    
    /* 6. RX/TX频率配置 (24bit: MSB/MID/LSB) */
    {
        double freq_step;
        uint32_t freq_reg;
        
        /* 根据射频类型选择频率步进 */
        if (initrfConfig->rftype == TK8710_RF_TYPE_1257_32M) {
            freq_step = RF_SX1257_FREQ_STEP;
        } else {
            freq_step = RF_SX1255_FREQ_STEP;
        }
        freq_reg = (uint32_t)((double)initrfConfig->Freq / freq_step);
        
        TK8710_LOG_CORE_INFO("Configuring frequency: %u Hz (step=%.2f, reg=0x%06X)", 
                            initrfConfig->Freq, freq_step, freq_reg);
        
        /* RX频率 */
        ret = tk8710_rf_write(rfSel, RF_CMD_FRF_RX_MSB >> 8, (freq_reg >> 16) & 0xFF);
        if (ret != TK8710_OK) {
            TK8710_LOG_CORE_ERROR("RX frequency MSB write failed: %d", ret);
            return ret;
        }
        
        ret = tk8710_rf_write(rfSel, RF_CMD_FRF_RX_MID >> 8, (freq_reg >> 8) & 0xFF);
        if (ret != TK8710_OK) {
            TK8710_LOG_CORE_ERROR("RX frequency MID write failed: %d", ret);
            return ret;
        }
        
        ret = tk8710_rf_write(rfSel, RF_CMD_FRF_RX_LSB >> 8, (freq_reg >> 0) & 0xFF);
        if (ret != TK8710_OK) {
            TK8710_LOG_CORE_ERROR("RX frequency LSB write failed: %d", ret);
            return ret;
        }
        
        /* TX频率 */
        ret = tk8710_rf_write(rfSel, RF_CMD_FRF_TX_MSB >> 8, (freq_reg >> 16) & 0xFF);
        if (ret != TK8710_OK) {
            TK8710_LOG_CORE_ERROR("TX frequency MSB write failed: %d", ret);
            return ret;
        }
        
        ret = tk8710_rf_write(rfSel, RF_CMD_FRF_TX_MID >> 8, (freq_reg >> 8) & 0xFF);
        if (ret != TK8710_OK) {
            TK8710_LOG_CORE_ERROR("TX frequency MID write failed: %d", ret);
            return ret;
        }
        
        ret = tk8710_rf_write(rfSel, RF_CMD_FRF_TX_LSB >> 8, (freq_reg >> 0) & 0xFF);
        if (ret != TK8710_OK) {
            TK8710_LOG_CORE_ERROR("TX frequency LSB write failed: %d", ret);
            return ret;
        }
        
        TK8710_LOG_CORE_DEBUG("RX/TX frequency configuration completed");
    }
    
    /* 8. RX增益配置 */
    ret = tk8710_rf_write(rfSel, RF_CMD_RX_GAIN >> 8, initrfConfig->rxgain);
    if (ret != TK8710_OK) {
        TK8710_LOG_CORE_ERROR("RX gain configuration failed: %d", ret);
        return ret;
    }
    TK8710_LOG_CORE_DEBUG("RX gain set to: 0x%02X", initrfConfig->rxgain);
    
    /* 9. TX增益配置 */
    ret = tk8710_rf_write(rfSel, RF_CMD_TX_GAIN >> 8, initrfConfig->txgain);
    if (ret != TK8710_OK) {
        TK8710_LOG_CORE_ERROR("TX gain configuration failed: %d", ret);
        return ret;
    }
    TK8710_LOG_CORE_DEBUG("TX gain set to: 0x%02X", initrfConfig->txgain);
    
    /* 10. 打开lna */
    ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, init_10), 0);
    if (ret != TK8710_OK) {
        TK8710_LOG_CORE_ERROR("LNA enable failed: %d", ret);
        return ret;
    }
    TK8710_LOG_CORE_DEBUG("LNA enabled");

    /* 11.  RF打开 */
    ret = tk8710_rf_write(rfSel, RF_CMD_CLOSE_F >> 8, RF_CMD_CLOSE_F & 0xFF);
    if (ret != TK8710_OK) {
        TK8710_LOG_CORE_ERROR("RF open failed: %d", ret);
        return ret;
    }
    TK8710_LOG_CORE_DEBUG("RF opened");
    
    /* 等待RF打开稳定 */
    usleep(20000);  /* 20ms等待RF完全打开和稳定 */

    /* 12. 打开PA */
    ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, init_10), (1 << 2));
    if (ret != TK8710_OK) {
        TK8710_LOG_CORE_ERROR("PA enable failed: %d", ret);
        return ret;
    }
    TK8710_LOG_CORE_DEBUG("PA enabled");
    
    /* 等待PA稳定 */
    usleep(10000);  /* 10ms等待PA稳定 */
    
    /* 13. 配置rx_bcn.acm_ctrl30 (0x78) */
    ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, RX_BCN_BASE + 0x78, 0x1110018);
    if (ret != TK8710_OK) {
        TK8710_LOG_CORE_ERROR("BCN ACM control configuration failed: %d", ret);
        return ret;
    }
    TK8710_LOG_CORE_DEBUG("BCN ACM control configured");
    
    TK8710_LOG_CORE_INFO("RF initialization completed successfully");
    return ret;
}

/**
 * @brief 控制发送BCN的天线和bcnbits
 * @param bcnantennasel 发送BCN天线配置 (配置mac.init_9.tx_bcn_en)
 * @param bcnbits 发送BCN信号中的bcnbits (配置tx_fe.tx_bcn_0.bcn_bits)
 * @return 0-成功, 1-失败, 2-超时
 */
int TK8710Txbcnctl(uint8_t bcnantennasel, uint8_t bcnbits)
{
    int ret;
    s_init_9 init9;
    s_tm_bcn_0 tmBcn0;
    
    /* 1. 读取并配置mac.init_9.tx_bcn_en */
    ret = TK8710ReadReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, init_9), &init9.data);
    if (ret != TK8710_OK) return ret;
    
    init9.b.tx_bcn_en = bcnantennasel;
    
    ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, init_9), init9.data);
    if (ret != TK8710_OK) return ret;
    
    /* 2. 读取并配置tx_top.tm_bcn_0.bcn_bits */
    ret = TK8710ReadReg(TK8710_REG_TYPE_GLOBAL, TX_MOD_BASE + offsetof(struct tx_top, tm_bcn_0), &tmBcn0.data);
    if (ret != TK8710_OK) return ret;
    
    tmBcn0.b.bcn_bits = bcnbits;
    
    ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, TX_MOD_BASE + offsetof(struct tx_top, tm_bcn_0), tmBcn0.data);
    
    return ret;
}

/**
 * @brief 芯片复位
 * @param rstType 复位类型: 1=仅复位状态机, 2=复位状态机+寄存器
 * @return 0-成功, 1-失败, 2-超时
 */
int TK8710Reset(uint8_t rstType)
{
    int ret;
    uint8_t resetConfig = 0;
    
    /* 根据复位类型设置复位配置 */
    switch (rstType) {
        case TK8710_RST_STATE_MACHINE:
            resetConfig = 0x01;  /* bit0: SM复位 */
            break;
        case TK8710_RST_ALL:
            resetConfig = 0x03;  /* SM+REG复位 */
            break;
        default:
            return TK8710_ERR;
    }
    
    /* 调用HAL层SPI复位命令 */
    ret = TK8710SpiReset(resetConfig);
    
    return (ret == 0) ? TK8710_OK : TK8710_ERR;
}

/**
 * @brief 写寄存器
 * @param regType 寄存器类型: 0=全局, 非0=RF寄存器(bit0-7对应RF0-7)
 * @param addr 寄存器地址 (16位)
 * @param data 写入数据 (32位)
 * @return 0-成功, 1-失败, 2-超时
 */
int TK8710WriteReg(uint8_t regType, uint16_t addr, uint32_t data)
{
    int ret;
    
    if (regType == TK8710_REG_TYPE_GLOBAL) {
        /* 全局寄存器: 直接通过SPI写入 */
        ret = TK8710SpiWriteReg(addr, &data, 1);
        return (ret == 0) ? TK8710_OK : TK8710_ERR;
    } else {
        /* RF寄存器: 通过内部SPI接口写入 */
        return tk8710_rf_write(regType, addr, data);
    }
}

/**
 * @brief 读寄存器
 * @param regType 寄存器类型: 0=全局, 非0=RF寄存器(bit0-7对应RF0-7)
 * @param addr 寄存器地址 (16位)
 * @param data 读取数据输出指针 (32位)
 * @return 0-成功, 1-失败, 2-超时
 */
int TK8710ReadReg(uint8_t regType, uint16_t addr, uint32_t* data)
{
    int ret;
    
    if (data == NULL) {
        return TK8710_ERR;
    }
    
    if (regType == TK8710_REG_TYPE_GLOBAL) {
        /* 全局寄存器: 直接通过SPI读取 */
        ret = TK8710SpiReadReg(addr, data, 1);
        return (ret == 0) ? TK8710_OK : TK8710_ERR;
    } else {
        /* RF寄存器: 通过内部SPI接口读取 */
        return tk8710_rf_read(regType, addr, data);
    }
}

/**
 * @brief 写发送数据缓冲区
 * @param start_index 数据开始index (0-127数据用户, 128-143广播用户)
 * @param data 用户数据指针
 * @param len 数据长度 (mode5/6/7/8: 单块26字节,最大260; mode9/10/11: 单块26字节,最大520; mode18: 单块40字节,最大520)
 * @return 0-成功, 1-失败, 2-超时
 */
int TK8710WriteBuffer(uint8_t start_index, const uint8_t* data, size_t len)
{
    int ret;
    
    if (data == NULL || len == 0) {
        return TK8710_ERR;
    }
    
    /* 调用HAL层SPI写Buffer命令 */
    ret = TK8710SpiWriteBuffer(start_index, data, (uint16_t)len);
    
    return (ret == 0) ? TK8710_OK : TK8710_ERR;
}

/**
 * @brief 读接收数据缓冲区
 * @param start_index 数据开始index (0-127数据用户, 128-143广播用户)
 * @param data_out 数据输出指针
 * @param len 数据长度
 * @return 0-成功, 1-失败, 2-超时
 */
int TK8710ReadBuffer(uint8_t start_index, uint8_t* data_out, size_t len)
{
    int ret;
    
    if (data_out == NULL || len == 0) {
        return TK8710_ERR;
    }
    
    /* 调用HAL层SPI读Buffer命令 */
    ret = TK8710SpiReadBuffer(start_index, data_out, (uint16_t)len);
    
    return (ret == 0) ? TK8710_OK : TK8710_ERR;
}

/**
 * @brief 写Efuse
 * @param start_bit 起始bit
 * @param data_bits 写入数据
 * @param len_bits 写入长度 (len_bits=1等价于单bit)
 * @return 0-成功, 1-失败, 2-超时
 */
int TK8710EfuseWrite(uint16_t start_bit, const uint8_t* data_bits, size_t len_bits)
{
    int ret;
    s_obv_6 obv6;
    s_efuse_0 efuse0;
    int timeout;
    
    (void)data_bits;
    (void)len_bits;
    
    /* 1. 等待efuse不忙 */
    timeout = 1000;
    do {
        ret = TK8710SpiReadReg(MAC_BASE + offsetof(struct mac, obv_6), &obv6.data, 1);
        if (ret != 0) return TK8710_ERR;
        if (--timeout <= 0) return TK8710_TIMEOUT;
    } while (obv6.b.efuse_busy);
    
    /* 2. 写efuse_0: (1 << 12) | addr */
    efuse0.data = ((uint32_t)1 << 12) | start_bit;
    ret = TK8710SpiWriteReg(MAC_BASE + offsetof(struct mac, efuse_0), &efuse0.data, 1);
    if (ret != 0) return TK8710_ERR;
    
    return TK8710_OK;
}

/**
 * @brief 读Efuse
 * @param start_bit 起始bit
 * @param data_bits 读取数据输出
 * @param len_bits 读取长度
 * @return 0-成功, 1-失败, 2-超时
 */
int TK8710EfuseRead(uint16_t start_bit, uint8_t* data_bits, size_t len_bits)
{
    int ret;
    s_efuse_0 efuse0;
    s_obv_6 obv6;
    
    (void)len_bits;
    
    if (data_bits == NULL) {
        return TK8710_ERR;
    }
    
    /* 1. 写efuse_0: addr */
    efuse0.data = start_bit;
    ret = TK8710SpiWriteReg(MAC_BASE + offsetof(struct mac, efuse_0), &efuse0.data, 1);
    if (ret != 0) return TK8710_ERR;
    
    /* 2. 读efuse_0 (触发读取) */
    ret = TK8710SpiReadReg(MAC_BASE + offsetof(struct mac, efuse_0), &efuse0.data, 1);
    if (ret != 0) return TK8710_ERR;
    
    /* 3. 读obv_6 获取结果 */
    ret = TK8710SpiReadReg(MAC_BASE + offsetof(struct mac, obv_6), &obv6.data, 1);
    if (ret != 0) return TK8710_ERR;
    
    /* 4. 返回efuse_out */
    *data_bits = (uint8_t)obv6.b.efuse_out;
    
    return TK8710_OK;
}



