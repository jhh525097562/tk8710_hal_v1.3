/**
 * @file tk8710_config.c
 * @brief TK8710 配置管理实现
 */

#include "../inc/driver/tk8710.h"
#include "../inc/driver/tk8710_regs.h"
#include "driver/tk8710_log.h"
#include "../port/tk8710_hal.h"
#include <stddef.h>
#include <string.h>

/*============================================================================
 * 全局变量 (保存配置状态)
 *============================================================================*/

/* 时隙配置全局变量 */
static slotCfg_t g_slotCfg = {0};

/* ACM校准增益 */
static uint16_t g_acm_dgain0[16] = {16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,0};
static uint16_t g_acm_dgain1[16] = {16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,0};
static uint16_t g_acm_again0[16] = {16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,0};
static uint16_t g_acm_again1[16] = {16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,0};

/*============================================================================
 * ACM校准内部辅助函数
 *============================================================================*/

/**
 * @brief 触发ACM校准
 */
static void tk8710_acm_trig(void)
{
    s_acm_ctrl acm_ctrl;
    acm_ctrl.data = 0;
    acm_ctrl.b.acm_trig = 1;
    TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, 
        MAC_BASE + offsetof(struct mac, acm_ctrl), acm_ctrl.data);
}

/* 前向声明 */
static int tk8710_acm_get_snr(int* TmpSNR, uint8_t snrThreshold);

/*============================================================================
 * ACM校准内部函数
 *============================================================================*/

/**
 * @brief ACM校准步骤1: 自动获取校准增益
 * @return 0-成功, 非0-失败
 */
static int tk8710_acm_get_gain(void)
{
    int ret;
    int i, j, i1, i2;
    int SumSNR_0[32], SumSNR_1[32], TmpSNR[32], MaxSNR[32];
    int GainStep = 4;
    uint32_t regVal;
    s_irq_ctrl0 irqCtrl0;
    
    /* 校准信号频点和相位配置数据 */
    uint16_t freqPose[16] = {0x7d,0x7e,0x7f,0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0};
    uint16_t dphase[16] = {0xa6,0x1a4,0x1c6,0x3c4,0x32e,0x2a4,0xa4,0x3de,0x1ec,0x368,0x182,0x3ec,0xe,0x358,0x3c4,0};
    uint16_t aphase[16] = {0xa6,0x1a4,0x1c6,0x3c4,0x32e,0x2a4,0xa4,0x3de,0x1ec,0x368,0x182,0x3ec,0xe,0x358,0x3c4,0};
    uint16_t dtone_en[8] = {0x2aab,0x4,0x10,0x40,0x100,0x400,0x1000,0x4000};
    uint16_t atone_en[8] = {0x3,0,0,0,0,0,0,0};
    
    /* 初始化SNR数组 */
    memset((void*)SumSNR_0, 0, sizeof(SumSNR_0));
    memset((void*)MaxSNR, 0, sizeof(MaxSNR));
    
    /* 重置增益到初始值 */
    for (i = 0; i < 16; i++) {
        g_acm_dgain0[i] = 16;
        g_acm_dgain1[i] = 16;
        g_acm_again0[i] = 16;
        g_acm_again1[i] = 16;
    }
    g_acm_dgain0[15] = 0;
    g_acm_dgain1[15] = 0;
    g_acm_again0[15] = 0;
    g_acm_again1[15] = 0;
    
    /* 步骤1: 关闭PA (mac.init_10, 值1<<3) */
    ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL,
        MAC_BASE + offsetof(struct mac, init_10), (1 << 3));
    if (ret != TK8710_OK) return ret;
    
    /* 步骤2: 关闭LNA (0x9478=0x10100010, mac.init_10=0x1a) */
    ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, 0x9478, 0x10100010);
    if (ret != TK8710_OK) return ret;
    ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL,
        MAC_BASE + offsetof(struct mac, init_10), 0x1a);
    if (ret != TK8710_OK) return ret;
    
    /* 步骤3: 设置校准信号频率 (acm_ctrl11) */
    for (i = 0; i < 8; i++) {
        regVal = (freqPose[i*2] << 16) | freqPose[i*2+1];
        ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL,
            ACM_BASE + offsetof(struct acm, acm_ctrl11) + 0x4 * i, regVal);
        if (ret != TK8710_OK) return ret;
    }
    
    /* 步骤4: 设置数据通道校准信号相位 (acm_ctrl25) */
    for (i = 0; i < 8; i++) {
        regVal = (dphase[i*2] << 16) | dphase[i*2+1];
        ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL,
            ACM_BASE + offsetof(struct acm, acm_ctrl25) + 0x4 * i, regVal);
        if (ret != TK8710_OK) return ret;
    }
    
    /* 步骤5: 设置数据通道校准信号tone使能 (acm_ctrl41) */
    for (i = 0; i < 4; i++) {
        regVal = (dtone_en[i*2] << 16) | dtone_en[i*2+1];
        ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL,
            ACM_BASE + offsetof(struct acm, acm_ctrl41) + 0x4 * i, regVal);
        if (ret != TK8710_OK) return ret;
    }
    
    /* 步骤6: 设置ACM通道校准信号相位 (acm_ctrl33) */
    for (i = 0; i < 8; i++) {
        regVal = (aphase[i*2] << 16) | aphase[i*2+1];
        ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL,
            ACM_BASE + offsetof(struct acm, acm_ctrl33) + 0x4 * i, regVal);
        if (ret != TK8710_OK) return ret;
    }
    
    /* 步骤7: 设置ACM通道校准信号tone使能 (acm_ctrl41) - 同步骤5 */
    for (i = 0; i < 4; i++) {
        regVal = (atone_en[i*2] << 16) | atone_en[i*2+1];
        ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL,
            ACM_BASE + offsetof(struct acm, acm_ctrl41) + 0x4 * i, regVal);
        if (ret != TK8710_OK) return ret;
    }
    
    /* 步骤8: 设置ACM中断使能 (irq_ctrl0.acm_irq_mask = 0) */
    ret = TK8710ReadReg(TK8710_REG_TYPE_GLOBAL,
        MAC_BASE + offsetof(struct mac, irq_ctrl0), &regVal);
    if (ret != TK8710_OK) return ret;
    irqCtrl0.data = regVal;
    irqCtrl0.b.acm_irq_mask = 0;
    ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL,
        MAC_BASE + offsetof(struct mac, irq_ctrl0), irqCtrl0.data);
    if (ret != TK8710_OK) return ret;
    
    /* 步骤9: 校准增益自动获取算法 */
    for (i = 1; i < 16; i++) {
        memset((void*)SumSNR_1, 0, sizeof(SumSNR_1));
        
        for (j = 0; j < 5; j++) {
            /* 设置acm_ctrl17~24 (again) 和 acm_ctrl9~16 (dgain) */
            for (i2 = 0; i2 < 8; i2++) {
                regVal = ((GainStep*i) << 24) | ((GainStep*i) << 16) | 
                         ((GainStep*i) << 8) | (GainStep*i);
                ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL,
                    ACM_BASE + offsetof(struct acm, acm_ctrl17) + 0x4 * i2, regVal);
                if (ret != TK8710_OK) return ret;
                ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL,
                    ACM_BASE + offsetof(struct acm, acm_ctrl9) + 0x4 * i2, regVal);
                if (ret != TK8710_OK) return ret;
            }
            
            /* 触发ACM */
            tk8710_acm_trig();
            
            /* 读取SNR */
            tk8710_acm_get_snr(TmpSNR, 0);
            
            /* 累加SNR */
            for (i2 = 0; i2 < 8; i2++) {
                SumSNR_1[i2 + 8*0] += TmpSNR[i2 + 8*0];
                SumSNR_1[i2 + 8*1] += TmpSNR[i2 + 8*1];
                SumSNR_1[i2 + 8*2] += TmpSNR[i2 + 8*2];
                SumSNR_1[i2 + 8*3] += TmpSNR[i2 + 8*3];
            }
            
            /* 只复位状态机 */
            TK8710SpiReset(1);
        }
        
        /* 比较并更新最佳增益 */
        for (i1 = 0; i1 < 8; i1++) {
            /* again0 */
            if (SumSNR_1[i1+8*0] > SumSNR_0[i1+8*0] && SumSNR_1[i1+8*0] > MaxSNR[i1+8*0] + 4) {
                if (i1 == 0) {
                    g_acm_again0[0] = g_acm_again0[1] = g_acm_again0[3] = g_acm_again0[5] = (GainStep*i);
                    g_acm_again0[7] = g_acm_again0[9] = g_acm_again0[11] = g_acm_again0[13] = (GainStep*i);
                } else {
                    g_acm_again0[i1*2] = (GainStep*i);
                }
                MaxSNR[i1+8*0] = SumSNR_1[i1+8*0];
            }
            /* dgain0 */
            if (SumSNR_1[i1+8*1] > SumSNR_0[i1+8*1] && SumSNR_1[i1+8*1] > MaxSNR[i1+8*1] + 4) {
                if (i1 == 0) {
                    g_acm_dgain0[0] = g_acm_dgain0[1] = g_acm_dgain0[3] = g_acm_dgain0[5] = (GainStep*i);
                    g_acm_dgain0[7] = g_acm_dgain0[9] = g_acm_dgain0[11] = g_acm_dgain0[13] = (GainStep*i);
                } else {
                    g_acm_dgain0[i1*2] = (GainStep*i);
                }
                MaxSNR[i1+8*1] = SumSNR_1[i1+8*1];
            }
            /* again1 */
            if (SumSNR_1[i1+8*2] > SumSNR_0[i1+8*2] && SumSNR_1[i1+8*2] > MaxSNR[i1+8*2] + 4) {
                if (i1 == 0) {
                    g_acm_again1[0] = g_acm_again1[1] = g_acm_again1[3] = g_acm_again1[5] = (GainStep*i);
                    g_acm_again1[7] = g_acm_again1[9] = g_acm_again1[11] = g_acm_again1[13] = g_acm_again1[15] = (GainStep*i);
                } else {
                    g_acm_again1[i1*2] = (GainStep*i);
                }
                MaxSNR[i1+8*2] = SumSNR_1[i1+8*2];
            }
            /* dgain1 */
            if (SumSNR_1[i1+8*3] > SumSNR_0[i1+8*3] && SumSNR_1[i1+8*3] > MaxSNR[i1+8*3] + 4) {
                if (i1 == 0) {
                    g_acm_dgain1[0] = g_acm_dgain1[1] = g_acm_dgain1[3] = g_acm_dgain1[5] = (GainStep*i);
                    g_acm_dgain1[7] = g_acm_dgain1[9] = g_acm_dgain1[11] = g_acm_dgain1[13] = g_acm_dgain1[15] = (GainStep*i);
                } else {
                    g_acm_dgain1[i1*2] = (GainStep*i);
                }
                MaxSNR[i1+8*3] = SumSNR_1[i1+8*3];
            }
            
            /* 保存当前SNR作为下一轮比较基准 */
            SumSNR_0[i1+8*0] = SumSNR_1[i1+8*0];
            SumSNR_0[i1+8*1] = SumSNR_1[i1+8*1];
            SumSNR_0[i1+8*2] = SumSNR_1[i1+8*2];
            SumSNR_0[i1+8*3] = SumSNR_1[i1+8*3];
        }
    }
    
    /* 计算最终增益 (dgain和again的平均值) */
    for (i = 0; i < 15; i++) {
        uint16_t Tmp0, Tmp1;
        if (i == 12) {  /* i==2*6 */
            Tmp0 = (g_acm_dgain0[i] + g_acm_again0[i] * 0);
            Tmp1 = (g_acm_dgain1[i] + g_acm_again1[i]) / 2;
        } else if (i == 14) {  /* i==2*7 */
            Tmp0 = (g_acm_dgain0[i] + g_acm_again0[i]) / 2;
            Tmp1 = (g_acm_dgain1[i] + g_acm_again1[i] * 0);
        } else {
            Tmp0 = (g_acm_dgain0[i] + g_acm_again0[i]) / 2;
            Tmp1 = (g_acm_dgain1[i] + g_acm_again1[i]) / 2;
        }
        g_acm_dgain0[i] = Tmp0;
        g_acm_again0[i] = Tmp0;
        g_acm_dgain1[i] = Tmp1;
        g_acm_again1[i] = Tmp1;
    }
    
    /* 步骤10: 增益更新 (写入acm_ctrl19和acm_ctrl17) */
    for (i = 0; i < 8; i++) {
        regVal = ((uint32_t)g_acm_dgain0[i*2] << 16) | g_acm_dgain0[i*2+1] |
                 ((uint32_t)g_acm_dgain1[i*2] << 24) | ((uint32_t)g_acm_dgain1[i*2+1] << 8);
        ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL,
            ACM_BASE + offsetof(struct acm, acm_ctrl19) + 0x4 * i, regVal);
        if (ret != TK8710_OK) return ret;
    }
    for (i = 0; i < 8; i++) {
        regVal = ((uint32_t)g_acm_again0[i*2] << 16) | g_acm_again0[i*2+1] |
                 ((uint32_t)g_acm_again1[i*2] << 24) | ((uint32_t)g_acm_again1[i*2+1] << 8);
        ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL,
            ACM_BASE + offsetof(struct acm, acm_ctrl17) + 0x4 * i, regVal);
        if (ret != TK8710_OK) return ret;
    }
    
    /* 关闭ACM中断使能 (irq_ctrl0.acm_irq_mask = 1) */
    ret = TK8710ReadReg(TK8710_REG_TYPE_GLOBAL,
        MAC_BASE + offsetof(struct mac, irq_ctrl0), &regVal);
    if (ret != TK8710_OK) return ret;
    irqCtrl0.data = regVal;
    irqCtrl0.b.acm_irq_mask = 1;
    ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL,
        MAC_BASE + offsetof(struct mac, irq_ctrl0), irqCtrl0.data);
    if (ret != TK8710_OK) return ret;
    
    return TK8710_OK;
}

/**
 * @brief ACM校准步骤2: 获取校准信号SNR
 * @param TmpSNR SNR输出缓冲区 (32个int)
 * @param snrThreshold SNR门限值
 * @return SNR大于门限的天线个数
 */
static int tk8710_acm_get_snr(int* TmpSNR, uint8_t snrThreshold)
{
    s_irq_res irq_res;
    s_irq_ctrl1 irq_ctrl1;
    uint32_t value_tmp1, value_tmp2, value_tmp3, value_tmp4;
    uint32_t value_tmp5, value_tmp6, value_tmp7, value_tmp8;
    int8_t SNR_tx0[8], SNR_rx0[8], SNR_tx1[8], SNR_rx1[8];
    int i;
    int SNRNum = 0;
    
    /* 等待ACM中断 (bit9) */
    TK8710ReadReg(TK8710_REG_TYPE_GLOBAL,
        MAC_BASE + offsetof(struct mac, irq_res), &irq_res.data);
    while ((irq_res.b.irq_type & (1 << TK8710_IRQ_ACM)) == 0) {
        TK8710ReadReg(TK8710_REG_TYPE_GLOBAL,
            MAC_BASE + offsetof(struct mac, irq_res), &irq_res.data);
    }
    
    /* 清除ACM中断 */
    irq_ctrl1.data = 0;
    irq_ctrl1.b.acm_irq_clr = 1;
    TK8710WriteReg(TK8710_REG_TYPE_GLOBAL,
        MAC_BASE + offsetof(struct mac, irq_ctrl1), irq_ctrl1.data);
    
    // /* 延时30ms */
    // TK8710DelayMs(30);
    
    /* 读取acm_obv17~24获取SNR值 */
    TK8710ReadReg(TK8710_REG_TYPE_GLOBAL,
        ACM_BASE + offsetof(struct acm, acm_obv18), &value_tmp1);
    TK8710ReadReg(TK8710_REG_TYPE_GLOBAL,
        ACM_BASE + offsetof(struct acm, acm_obv17), &value_tmp2);
    TK8710ReadReg(TK8710_REG_TYPE_GLOBAL,
        ACM_BASE + offsetof(struct acm, acm_obv20), &value_tmp3);
    TK8710ReadReg(TK8710_REG_TYPE_GLOBAL,
        ACM_BASE + offsetof(struct acm, acm_obv19), &value_tmp4);
    TK8710ReadReg(TK8710_REG_TYPE_GLOBAL,
        ACM_BASE + offsetof(struct acm, acm_obv22), &value_tmp5);
    TK8710ReadReg(TK8710_REG_TYPE_GLOBAL,
        ACM_BASE + offsetof(struct acm, acm_obv21), &value_tmp6);
    TK8710ReadReg(TK8710_REG_TYPE_GLOBAL,
        ACM_BASE + offsetof(struct acm, acm_obv24), &value_tmp7);
    TK8710ReadReg(TK8710_REG_TYPE_GLOBAL,
        ACM_BASE + offsetof(struct acm, acm_obv23), &value_tmp8);
    
    /* 解析SNR值到各个天线 */
    for (i = 0; i < 4; i++) {
        SNR_tx0[i]   = ((value_tmp1 >> (i*8)) & 0xff);
        SNR_tx0[i+4] = ((value_tmp2 >> (i*8)) & 0xff);
        SNR_rx0[i]   = ((value_tmp3 >> (i*8)) & 0xff);
        SNR_rx0[i+4] = ((value_tmp4 >> (i*8)) & 0xff);
        SNR_tx1[i]   = ((value_tmp5 >> (i*8)) & 0xff);
        SNR_tx1[i+4] = ((value_tmp6 >> (i*8)) & 0xff);
        SNR_rx1[i]   = ((value_tmp7 >> (i*8)) & 0xff);
        SNR_rx1[i+4] = ((value_tmp8 >> (i*8)) & 0xff);
        
        /* 填充TmpSNR数组 */
        TmpSNR[i+8*0] = SNR_tx0[i];
        TmpSNR[i+4+8*0] = SNR_tx0[i+4];
        TmpSNR[i+8*1] = SNR_rx0[i];
        TmpSNR[i+4+8*1] = SNR_rx0[i+4];
        TmpSNR[i+8*2] = SNR_tx1[i];
        TmpSNR[i+4+8*2] = SNR_tx1[i+4];
        TmpSNR[i+8*3] = SNR_rx1[i];
        TmpSNR[i+4+8*3] = SNR_rx1[i+4];
    }
    
    /* 比较SNR与门限，统计通过个数 */
    for (i = 0; i < 8; i++) {
        if ((SNR_tx0[i] >= snrThreshold) && (SNR_tx0[i] < 60)) {
            SNRNum++;
        }
        if ((SNR_rx0[i] >= snrThreshold) && (SNR_rx0[i] < 60)) {
            SNRNum++;
        }
        if ((SNR_tx1[i] >= snrThreshold) && (SNR_tx1[i] < 60)) {
            SNRNum++;
        }
        if ((SNR_rx1[i] >= snrThreshold) && (SNR_rx1[i] < 60)) {
            SNRNum++;
        }
    }
    
    return SNRNum;
}

/**
 * @brief ACM校准步骤3: 执行校准流程
 * @param calibCount 连续校准次数
 * @param snrThreshold SNR门限值
 * @return 有效校准次数
 */
static int tk8710_acm_calibrate(uint8_t calibCount, uint8_t snrThreshold)
{
    int ret;
    int i;
    uint32_t regVal;
    s_irq_ctrl0 irqCtrl0;
    int TmpSNR[32];
    int TmpSNRNum;
    int validCount = 0;
    
    /* 校准信号频点和相位配置数据 (与tk8710_acm_get_gain相同) */
    uint16_t freqPose[16] = {0x7d,0x7e,0x7f,0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0};
    uint16_t dphase[16] = {0xa6,0x1a4,0x1c6,0x3c4,0x32e,0x2a4,0xa4,0x3de,0x1ec,0x368,0x182,0x3ec,0xe,0x358,0x3c4,0};
    uint16_t aphase[16] = {0xa6,0x1a4,0x1c6,0x3c4,0x32e,0x2a4,0xa4,0x3de,0x1ec,0x368,0x182,0x3ec,0xe,0x358,0x3c4,0};
    uint16_t dtone_en[8] = {0x2aab,0x4,0x10,0x40,0x100,0x400,0x1000,0x4000};
    uint16_t atone_en[8] = {0x3,0,0,0,0,0,0,0};
    
    /* 步骤1: 关闭PA (mac.init_10, 值1<<3) */
    ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL,
        MAC_BASE + offsetof(struct mac, init_10), (1 << 3));
    if (ret != TK8710_OK) return ret;
    
    /* 步骤2: 关闭LNA (0x9478=0x10100010, mac.init_10=0x1a) */
    ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, 0x9478, 0x10100010);
    if (ret != TK8710_OK) return ret;
    ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL,
        MAC_BASE + offsetof(struct mac, init_10), 0x1a);
    if (ret != TK8710_OK) return ret;
    
    /* 步骤3: 设置校准信号频率 (acm_ctrl11) */
    for (i = 0; i < 8; i++) {
        regVal = (freqPose[i*2] << 16) | freqPose[i*2+1];
        ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL,
            ACM_BASE + offsetof(struct acm, acm_ctrl11) + 0x4 * i, regVal);
        if (ret != TK8710_OK) return ret;
    }
    
    /* 步骤4: 设置数据通道校准信号相位 (acm_ctrl25) */
    for (i = 0; i < 8; i++) {
        regVal = (dphase[i*2] << 16) | dphase[i*2+1];
        ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL,
            ACM_BASE + offsetof(struct acm, acm_ctrl25) + 0x4 * i, regVal);
        if (ret != TK8710_OK) return ret;
    }
    
    /* 步骤5: 设置数据通道校准信号tone使能 (acm_ctrl41) */
    for (i = 0; i < 4; i++) {
        regVal = (dtone_en[i*2] << 16) | dtone_en[i*2+1];
        ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL,
            ACM_BASE + offsetof(struct acm, acm_ctrl41) + 0x4 * i, regVal);
        if (ret != TK8710_OK) return ret;
    }
    
    /* 步骤6: 设置ACM通道校准信号相位 (acm_ctrl33) */
    for (i = 0; i < 8; i++) {
        regVal = (aphase[i*2] << 16) | aphase[i*2+1];
        ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL,
            ACM_BASE + offsetof(struct acm, acm_ctrl33) + 0x4 * i, regVal);
        if (ret != TK8710_OK) return ret;
    }
    
    /* 步骤7: 设置ACM通道校准信号tone使能 (acm_ctrl41) */
    for (i = 0; i < 4; i++) {
        regVal = (atone_en[i*2] << 16) | atone_en[i*2+1];
        ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL,
            ACM_BASE + offsetof(struct acm, acm_ctrl41) + 0x4 * i, regVal);
        if (ret != TK8710_OK) return ret;
    }
    
    /* 步骤8: 设置ACM中断使能 (irq_ctrl0.acm_irq_mask = 0) */
    ret = TK8710ReadReg(TK8710_REG_TYPE_GLOBAL,
        MAC_BASE + offsetof(struct mac, irq_ctrl0), &regVal);
    if (ret != TK8710_OK) return ret;
    irqCtrl0.data = regVal;
    irqCtrl0.b.acm_irq_mask = 0;
    ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL,
        MAC_BASE + offsetof(struct mac, irq_ctrl0), irqCtrl0.data);
    if (ret != TK8710_OK) return ret;
    
    /* 循环执行校准 */
    for (i = 0; i < calibCount; i++) {
        /* 步骤9: 复位状态机 */
        TK8710SpiReset(1);
        
        /* 步骤10: 触发校准、获取SNR */
        tk8710_acm_trig();
        TmpSNRNum = tk8710_acm_get_snr(TmpSNR, snrThreshold);
        
        /* 步骤11: 判断满足SNR门限个数是否大于25 */
        if (TmpSNRNum > 25) {
            validCount++;
        }
    }
    
    /* 关闭ACM中断使能 */
    ret = TK8710ReadReg(TK8710_REG_TYPE_GLOBAL,
        MAC_BASE + offsetof(struct mac, irq_ctrl0), &regVal);
    if (ret != TK8710_OK) return ret;
    irqCtrl0.data = regVal;
    irqCtrl0.b.acm_irq_mask = 1;
    ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL,
        MAC_BASE + offsetof(struct mac, irq_ctrl0), irqCtrl0.data);
    if (ret != TK8710_OK) return ret;
    
    return validCount;
}

/*============================================================================
 * 全局配置访问函数
 *============================================================================*/

/**
 * @brief 获取时隙配置
 * @return 时隙配置指针
 */
const slotCfg_t* TK8710GetSlotCfg(void)
{
    return &g_slotCfg;
}

/**
 * @brief 获取下行发送模式
 * @return 0=自动发送, 1=指定信息发送
 */
uint8_t TK8710GetTxAutoMode(void)
{
    return g_slotCfg.txAutoMode;
}

/*============================================================================
 * 配置接口实现
 *============================================================================*/

/**
 * @brief 设置芯片配置
 * @param type 配置类型:
 *        - TK8710_CFG_TYPE_CHIP_INFO: 配置芯片参数 (ChipConfig)
 *        - TK8710_CFG_TYPE_SLOT_CFG: 配置时隙参数 (slotCfg_t)
 *        - TK8710_CFG_TYPE_ADDTL: 配置附加位参数 (addtlCfg_u)
 * @param params 配置参数指针
 * @return 0-成功, 1-失败, 2-超时
 */
int TK8710SetConfig(TK8710ConfigType type, const void* params)
{
    if (params == NULL) {
        TK8710_LOG_CONFIG_ERROR("Config params is NULL for type %d", type);
        return TK8710_ERR;
    }
    
    switch (type) {
        case TK8710_CFG_TYPE_CHIP_INFO:
            /* 配置芯片参数，调用TK8710Init */
            return TK8710Init((const ChipConfig*)params);
        
        case TK8710_CFG_TYPE_SLOT_CFG:
        {
            /* 配置时隙参数 */
            slotCfg_t* slotCfg = (slotCfg_t*)params;
            s_mac_config_0 macConfig0;
            s_mac_config_1 macConfig1;
            int ret;
            
            if (slotCfg == NULL) {
                return TK8710_ERR;
            }
            
            /* 配置mac_config_0 (0x00): s0_cfg, s1_cfg, mode */
            macConfig0.data = 0;
            macConfig0.b.s0_cfg = 0;  /* BCN时隙 */
            macConfig0.b.s1_cfg = slotCfg->s1Cfg[0].byteLen & 0x3FF;  /* 使用第一个速率模式 */
            macConfig0.b.mode = slotCfg->rateModes[0] & 0xF;  /* 使用第一个速率模式 */
            ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, 
                MAC_BASE + offsetof(struct mac, mac_config_0), macConfig0.data);
            if (ret != TK8710_OK) return ret;
            
            /* 配置mac_config_1 (0x04): s2_cfg, s3_cfg, pl_crc_en */
            macConfig1.data = 0;
            macConfig1.b.s2_cfg = slotCfg->s2Cfg[0].byteLen & 0x3FF;  /* 使用第一个速率模式 */
            macConfig1.b.s3_cfg = slotCfg->s3Cfg[0].byteLen & 0x3FF;  /* 使用第一个速率模式 */
            macConfig1.b.pl_crc_en = slotCfg->plCrcEn & 0x01;
            ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, 
                MAC_BASE + offsetof(struct mac, mac_config_1), macConfig1.data);
            if (ret != TK8710_OK) return ret;
            
            /* 配置 init_1: md_agc, rx_delay */
            {
                s_init_1 init1;
                init1.data = 0;
                init1.b.md_agc = slotCfg->md_agc & 0xFFF;  /* DATA AGC长度, 12bit最大值4095 */
                init1.b.rx_delay = slotCfg->rx_delay & 0xFFFFF;
                ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, 
                    MAC_BASE + offsetof(struct mac, init_1), init1.data);
                if (ret != TK8710_OK) return ret;
            }
            
            /* 配置 init_2: da1_m */
            {
                s_init_2 init2;
                init2.data = 0;
                init2.b.da1_m = slotCfg->s1Cfg[0].da_m & 0xFFFFFF;  /* da1_m from S1[0] */
                ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, 
                    MAC_BASE + offsetof(struct mac, init_2), init2.data);
                if (ret != TK8710_OK) return ret;
            }
            
            /* 配置 init_3: da2_m, tx_fix_freq */
            {
                s_init_3 init3;
                init3.data = 0;
                init3.b.da2_m = slotCfg->s2Cfg[0].da_m & 0xFFFFFF;  /* da2_m from S2[0] */
                init3.b.tx_fix_freq = slotCfg->txAutoMode & 0x01;     /* tx_fix_freq */
                ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, 
                    MAC_BASE + offsetof(struct mac, init_3), init3.data);
                if (ret != TK8710_OK) return ret;
            }
            
            /* 配置 init_4: da3_m */
            {
                s_init_4 init4;
                init4.data = 0;
                init4.b.da3_m = slotCfg->s3Cfg[0].da_m & 0xFFFFFF;  /* da3_m from S3[0] */
                ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, 
                    MAC_BASE + offsetof(struct mac, init_4), init4.data);
                if (ret != TK8710_OK) return ret;
            }
            
            /* 配置 init_18: da0_m (完整24位) */
            {
                s_init_18 init18;
                init18.data = 0;
                init18.b.da0_m = slotCfg->s0Cfg[0].da_m & 0xFFFFFF;  /* da0_m完整24位 from S0[0] */
                ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, 
                    MAC_BASE + offsetof(struct mac, init_18), init18.data);
                if (ret != TK8710_OK) return ret;
            }
            
            /* 注：主从模式由TK8710Startwork配置，此处不更新 */
            
            /* 保存时隙配置到全局变量 (包含brdUserNum) */
            memcpy(&g_slotCfg, slotCfg, sizeof(slotCfg_t));
            
            TK8710_LOG_CONFIG_INFO("Slot config updated: rateCount=%d, rateModes[0]=%d, brdUserNum=%d, txAutoMode=%d", 
                                 slotCfg->rateCount, slotCfg->rateModes[0], slotCfg->brdUserNum, slotCfg->txAutoMode);
            
            /* 从寄存器读取时隙时间长度并更新 */
            {
                s_obv_1 obv1;
                s_obv_2 obv2;
                s_obv_3 obv3;
                s_obv_4 obv4;
                uint32_t regVal;
                
                TK8710ReadReg(TK8710_REG_TYPE_GLOBAL,
                    MAC_BASE + offsetof(struct mac, obv_1), &regVal);
                obv1.data = regVal;
                g_slotCfg.s0Cfg[0].timeLen = obv1.b.s0_len;
                
                TK8710ReadReg(TK8710_REG_TYPE_GLOBAL,
                    MAC_BASE + offsetof(struct mac, obv_2), &regVal);
                obv2.data = regVal;
                g_slotCfg.s1Cfg[0].timeLen = obv2.b.s1_len;
                
                TK8710ReadReg(TK8710_REG_TYPE_GLOBAL,
                    MAC_BASE + offsetof(struct mac, obv_3), &regVal);
                obv3.data = regVal;
                g_slotCfg.s2Cfg[0].timeLen = obv3.b.s2_len;
                
                TK8710ReadReg(TK8710_REG_TYPE_GLOBAL,
                    MAC_BASE + offsetof(struct mac, obv_4), &regVal);
                obv4.data = regVal;
                g_slotCfg.s3Cfg[0].timeLen = obv4.b.s3_len;
                
                /* 计算帧总时间 */
                g_slotCfg.frameTimeLen = g_slotCfg.s0Cfg[0].timeLen + 
                    g_slotCfg.s1Cfg[0].timeLen + g_slotCfg.s2Cfg[0].timeLen + 
                    g_slotCfg.s3Cfg[0].timeLen;
                
                /* 打印每个slot的时间长度和帧总时间 */
                TK8710_LOG_CONFIG_INFO("Slot time lengths - S0:%d, S1:%d, S2:%d, S3:%d (us)",
                    g_slotCfg.s0Cfg[0].timeLen, g_slotCfg.s1Cfg[0].timeLen, 
                    g_slotCfg.s2Cfg[0].timeLen, g_slotCfg.s3Cfg[0].timeLen);
                TK8710_LOG_CONFIG_INFO("Frame total time length: %d (us)", g_slotCfg.frameTimeLen);
                    
            }
            
            return TK8710_OK;
        }
        
        case TK8710_CFG_TYPE_ADDTL:
            /* TODO: 待实现 */
            return TK8710_OK;
        
        default:
            return TK8710_ERR;
    }
}

/**
 * @brief 获取芯片配置
 * @param type 配置类型:
 *        - TK8710_CFG_TYPE_CHIP_INFO: 获取芯片参数 (ChipConfig)
 *        - TK8710_CFG_TYPE_SLOT_CFG: 获取时隙参数 (slotCfg_t)
 *        - TK8710_CFG_TYPE_ADDTL: 获取附加位参数 (addtlCfg_u)
 * @param params 配置参数输出指针
 * @return 0-成功, 1-失败, 2-超时
 */
int TK8710GetConfig(TK8710ConfigType type, void* params)
{
    int ret;
    
    if (params == NULL) {
        return TK8710_ERR;
    }
    
    switch (type) {
        case TK8710_CFG_TYPE_CHIP_INFO:
        {
            ChipConfig* cfg = (ChipConfig*)params;
            s_init_0 init0;
            s_init_1 init1;
            s_init_2 init2;
            s_init_3 init3;
            s_init_4 init4;
            s_init_5 init5;
            s_init_9 init9;
            s_init_11 init11;
            s_irq_ctrl0 irqCtrl0;
            s_irq_ctrl1 irqCtrl1;
            
            /* 读取 init_0 */
            ret = TK8710ReadReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, init_0), &init0.data);
            if (ret != TK8710_OK) return ret;
            cfg->bcn_agc = init0.b.bcn_agc;
            cfg->interval = init0.b.interval;
            cfg->tx_dly = init0.b.tx_freq_dly;  /* 保存到cfg */
            
            /* 读取 init_1 */
            ret = TK8710ReadReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, init_1), &init1.data);
            if (ret != TK8710_OK) return ret;
            /* md_agc字段已移至slotCfg_t */
            /* rx_delay字段已移至slotCfg_t */
            
            /* init_2, init_3, init_4, init_18 配置已移至slotCfg_t */
            
            /* 读取 init_5 */
            ret = TK8710ReadReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, init_5), &init5.data);
            if (ret != TK8710_OK) return ret;
            cfg->offset_adj = init5.b.offset_adj;
            cfg->tx_pre = init5.b.tx_pre;
            cfg->conti_mode = init5.b.conti_mode;
            cfg->bcn_scan = init5.b.conti_scan;
            
            /* 读取 init_9 */
            ret = TK8710ReadReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, init_9), &init9.data);
            if (ret != TK8710_OK) return ret;
            cfg->ant_en = init9.b.ant_en;
            cfg->rf_sel = init9.b.rf_sel;
            cfg->tx_bcn_en = init9.b.tx_bcn_en;
            
            /* 读取 init_11 */
            ret = TK8710ReadReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, init_11), &init11.data);
            if (ret != TK8710_OK) return ret;
            cfg->rf_model = init11.b.rf_type;
            
            /* 读取 irq_ctrl1 */
            ret = TK8710ReadReg(TK8710_REG_TYPE_GLOBAL, MAC_BASE + offsetof(struct mac, irq_ctrl1), &irqCtrl1.data);
            if (ret != TK8710_OK) return ret;
            cfg->irq_ctrl1 = irqCtrl1.data;
            
            return TK8710_OK;
        }
        
        case TK8710_CFG_TYPE_SLOT_CFG:
        {
            /* 获取时隙配置 */
            slotCfg_t* slotCfg = (slotCfg_t*)params;
            s_mac_config_0 macConfig0;
            s_mac_config_1 macConfig1;
            s_obv_1 obv1;
            s_obv_2 obv2;
            s_obv_3 obv3;
            s_obv_4 obv4;
            uint32_t regVal;
            
            if (slotCfg == NULL) {
                return TK8710_ERR;
            }
            
            /* 读取mac_config_0 (0x00): s0_cfg, s1_cfg, mode */
            ret = TK8710ReadReg(TK8710_REG_TYPE_GLOBAL, 
                MAC_BASE + offsetof(struct mac, mac_config_0), &regVal);
            if (ret != TK8710_OK) return ret;
            macConfig0.data = regVal;
            slotCfg->brdUserNum = TK8710GetBrdUserNum();  /* 从全局变量获取广播用户数 */
            slotCfg->s1Cfg[0].byteLen = macConfig0.b.s1_cfg;
            slotCfg->rateModes[0] = macConfig0.b.mode & 0xF;
            
            /* 读取mac_config_1 (0x04): s2_cfg, s3_cfg, pl_crc_en */
            ret = TK8710ReadReg(TK8710_REG_TYPE_GLOBAL, 
                MAC_BASE + offsetof(struct mac, mac_config_1), &regVal);
            if (ret != TK8710_OK) return ret;
            macConfig1.data = regVal;
            slotCfg->s2Cfg[0].byteLen = macConfig1.b.s2_cfg;
            slotCfg->s3Cfg[0].byteLen = macConfig1.b.s3_cfg;
            slotCfg->plCrcEn = macConfig1.b.pl_crc_en;
            
            /* 读取obv_1 (0xf4): s0_len - BCN时隙时间 */
            ret = TK8710ReadReg(TK8710_REG_TYPE_GLOBAL, 
                MAC_BASE + offsetof(struct mac, obv_1), &regVal);
            if (ret != TK8710_OK) return ret;
            obv1.data = regVal;
            /* s0为BCN时隙，暂不存储 */
            
            /* 读取obv_2 (0xf8): s1_len - 时隙1时间 */
            ret = TK8710ReadReg(TK8710_REG_TYPE_GLOBAL, 
                MAC_BASE + offsetof(struct mac, obv_2), &regVal);
            if (ret != TK8710_OK) return ret;
            obv2.data = regVal;
            slotCfg->s1Cfg[0].timeLen = obv2.b.s1_len;
            
            /* 读取obv_3 (0xfc): s2_len - 时隙2时间 */
            ret = TK8710ReadReg(TK8710_REG_TYPE_GLOBAL, 
                MAC_BASE + offsetof(struct mac, obv_3), &regVal);
            if (ret != TK8710_OK) return ret;
            obv3.data = regVal;
            slotCfg->s2Cfg[0].timeLen = obv3.b.s2_len;
            
            /* 读取obv_4 (0x100): s3_len - 时隙3时间 */
            ret = TK8710ReadReg(TK8710_REG_TYPE_GLOBAL, 
                MAC_BASE + offsetof(struct mac, obv_4), &regVal);
            if (ret != TK8710_OK) return ret;
            obv4.data = regVal;
            slotCfg->s3Cfg[0].timeLen = obv4.b.s3_len;
            
            /* 从全局状态获取主从模式 */
            slotCfg->msMode = TK8710GetWorkType();
            
            return TK8710_OK;
        }
        
        case TK8710_CFG_TYPE_ADDTL:
            /* TODO: 待实现 */
            return TK8710_OK;
        
        default:
            return TK8710_ERR;
    }
}

/**
 * @brief 芯片控制命令
 * @param type 控制类型:
 *        - TK8710_CTRL_TYPE_ACM_START: 触发ACM校准 (acmParam_t)
 *        - TK8710_CTRL_TYPE_SEND_WAKEUP: 发送唤醒信号 (wakeUpParam_t)
 * @param params 控制参数指针
 * @return 0-成功, 非0-失败 (ACM时返回值非0表示异常天线位)
 */
int TK8710Ctrl(TK8710CtrlType type, const void* params)
{
    switch (type) {
        case TK8710_CTRL_TYPE_ACM_START:
        {
            acmParam_t* acmParam = (acmParam_t*)params;
            int ret;
            
            if (acmParam == NULL) {
                return TK8710_ERR;
            }
            
            /* 步骤1: 自动获取校准增益 */
            ret = tk8710_acm_get_gain();
            if (ret != TK8710_OK) {
                return ret;
            }
            
            /* 步骤2: 执行校准流程 */
            ret = tk8710_acm_calibrate(acmParam->calibCount, acmParam->snrThreshold);
            return ret;
        }
        
        case TK8710_CTRL_TYPE_SEND_WAKEUP:
        {
            wakeUpParam_t* wakeUpParam = (wakeUpParam_t*)params;
            
            if (wakeUpParam == NULL) {
                return TK8710_ERR;
            }
            
            /* TODO: 实现唤醒信号发送逻辑 */
            (void)wakeUpParam;
            return TK8710_OK;
        }
        
        default:
            return TK8710_ERR;
    }
}

/**
 * @brief 调试控制接口
 * @param ctrlType 控制类型:
 *        - TK8710_DBG_TYPE_FFT_OUT: 获取FFT运算输出 (仅Get)
 *        - TK8710_DBG_TYPE_CAPTURE_DATA: 获取原始采样数据 (仅Get)
 *        - TK8710_DBG_TYPE_ACM_CAL_FACTOR: 获取ACM校准因子 (仅Get)
 *        - TK8710_DBG_TYPE_ACM_SNR: 获取ACM信噪比 (仅Get)
 *        - TK8710_DBG_TYPE_TX_TONE: 配置并发送单Tone信号 (仅Set)
 * @param optType 操作类型: 0=Set, 1=Get, 2=Exe
 * @param inputParams 输入参数指针 (Set/Exe时使用)
 * @param outputParams 输出参数指针 (Get时使用)
 * @return 0-成功, 1-失败, 2-超时
 */
int TK8710DebugCtrl(TK8710DebugCtrlType ctrlType, CtrlOptType optType,
                    const void* inputParams, void* outputParams)
{
    (void)outputParams;
    
    switch (ctrlType) {
        case TK8710_DBG_TYPE_TX_TONE:
        {
            if (optType != TK8710_DBG_OPT_SET) {
                return TK8710_ERR;
            }
            
            TxToneConfig* txTone = (TxToneConfig*)inputParams;
            int i;
            int ret;
            uint32_t toneFreq;
            uint32_t toneGain;
            
            if (txTone == NULL) {
                return TK8710_ERR;
            }
            
            toneFreq = txTone->freq;
            toneGain = txTone->gain;
            
            /* 配置寄存器 0xc030 = 0x8 */
            ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, 0xc030, 0x8);
            if (ret != TK8710_OK) return ret;
            
            /* 配置寄存器 0x9478 = 0x10100010 */
            ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, 0x9478, 0x10100010);
            if (ret != TK8710_OK) return ret;
            
            /* 配置寄存器 0xc030 = 0x4 */
            ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL, 0xc030, 0x4);
            if (ret != TK8710_OK) return ret;
            
            /* 循环配置8个天线的TX Tone */
            for (i = 0; i < 8; i++) {
                /* 配置tx_config_31 (0xd0): Tone频率 */
                ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL,
                    TX_FE_BASE + 0xd0 + 0x1000 * i,
                    toneFreq + 167772 * i);
                if (ret != TK8710_OK) return ret;
                
                /* 配置tx_config_33 (0xd8): Tone增益 */
                ret = TK8710WriteReg(TK8710_REG_TYPE_GLOBAL,
                    TX_FE_BASE + 0xd8 + 0x1000 * i,
                    toneGain);
                if (ret != TK8710_OK) return ret;
            }
            
            return TK8710_OK;
        }
        
        case TK8710_DBG_TYPE_FFT_OUT:
        case TK8710_DBG_TYPE_CAPTURE_DATA:
        case TK8710_DBG_TYPE_ACM_CAL_FACTOR:
        case TK8710_DBG_TYPE_ACM_SNR:
            /* TODO: 待实现 */
            (void)inputParams;
            return TK8710_OK;
            
        default:
            return TK8710_ERR;
    }
}
