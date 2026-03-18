/**
 * @file trm_mac_parser.c
 * @brief TRM MAC协议帧解析实现
 * @note 此文件包含TurMass™ Link MAC协议帧的解析函数实现
 */

#include "trm_mac_parser.h"
#include "trm_log.h"

/* =============================================================================
 * MAC协议帧解析辅助函数实现
 * ============================================================================= */

int TRM_ParseMacMhdr(const uint8_t* data, uint16_t len, TrmMacMhdr* mhdr)
{
    if (data == NULL || mhdr == NULL || len < 3) {
        TRM_LOG_ERROR("TRM: Invalid parameters for MHDR parsing");
        return -1;
    }
    
    /* 解析第一个字节 */
    mhdr->frameType = (data[TRM_MHDR_BYTE1_OFFSET] >> TRM_MHDR_FRAMETYPE_SHIFT) & TRM_MHDR_FRAMETYPE_MASK;
    mhdr->devType   = (data[TRM_MHDR_BYTE1_OFFSET] >> TRM_MHDR_DEVTYPE_SHIFT) & TRM_MHDR_DEVTYPE_MASK;
    mhdr->version   = (data[TRM_MHDR_BYTE1_OFFSET] >> TRM_MHDR_VERSION_SHIFT) & TRM_MHDR_VERSION_MASK;
    
    /* 解析第二个字节 (QoS相关) */
    mhdr->qosPri = (data[TRM_MHDR_BYTE2_OFFSET] >> TRM_MHDR_QOSPRI_SHIFT) & TRM_MHDR_QOSPRI_MASK;
    mhdr->qosTtl = (data[TRM_MHDR_BYTE2_OFFSET] >> TRM_MHDR_QOSTTL_SHIFT) & TRM_MHDR_QOSTTL_MASK;
    mhdr->hopNum = (data[TRM_MHDR_BYTE2_OFFSET] >> TRM_MHDR_HOPNUM_SHIFT) & TRM_MHDR_HOPNUM_MASK;
    mhdr->hopCnt = (data[TRM_MHDR_BYTE2_OFFSET] >> TRM_MHDR_HOPCNT_SHIFT) & TRM_MHDR_HOPCNT_MASK;
    
    /* 解析第三个字节 */
    mhdr->securityMode = (data[TRM_MHDR_BYTE3_OFFSET] >> TRM_MHDR_SECURITYMODE_SHIFT) & TRM_MHDR_SECURITYMODE_MASK;
    mhdr->addrMode      = (data[TRM_MHDR_BYTE3_OFFSET] >> TRM_MHDR_ADDRMODE_SHIFT) & TRM_MHDR_ADDRMODE_MASK;
    mhdr->nwkMode       = (data[TRM_MHDR_BYTE3_OFFSET] >> TRM_MHDR_NWKMODE_SHIFT) & TRM_MHDR_NWKMODE_MASK;
    mhdr->powerCtrlType = (data[TRM_MHDR_BYTE3_OFFSET] >> TRM_MHDR_POWERCTRL_SHIFT) & TRM_MHDR_POWERCTRL_MASK;
    mhdr->rfu           = (data[TRM_MHDR_BYTE3_OFFSET] >> TRM_MHDR_RFU_SHIFT) & TRM_MHDR_RFU_MASK;
    
    TRM_LOG_DEBUG("TRM: MHDR parsed - FrameType=0x%01X, DevType=%d, QosPri=%d, TTL=%d, AddrMode=%d", 
                  mhdr->frameType, mhdr->devType, mhdr->qosPri, mhdr->qosTtl, mhdr->addrMode);
    
    return 0;
}

int TRM_ExtractUserIdFromMacFrame(const uint8_t* data, uint16_t len, uint32_t* userId)
{
    if (data == NULL || userId == NULL || len < 3) {
        TRM_LOG_ERROR("TRM: Invalid parameters for user ID extraction");
        return -1;
    }
    
    /* 解析MHDR获取地址模式 */
    TrmMacMhdr mhdr;
    if (TRM_ParseMacMhdr(data, len, &mhdr) != 0) {
        return -1;
    }
    
    /* FHDR从MHDR后开始，偏移3字节 */
    uint8_t fhdrOffset = 3;
    
    if (mhdr.addrMode == 0) {
        /* 短地址模式 (2字节) */
        if (len >= fhdrOffset + 2) {
            *userId = (data[fhdrOffset] << 8) | data[fhdrOffset + 1];
            TRM_LOG_DEBUG("TRM: Extracted user ID from short address: 0x%04X", *userId);
            return 0;
        } else {
            TRM_LOG_WARN("TRM: Data too short for short address extraction");
            return -1;
        }
    } else {
        /* 长地址模式 (4字节: NwkID + NwkAddr) */
        if (len >= fhdrOffset + 4) {
            *userId = (data[fhdrOffset] << 24) | (data[fhdrOffset + 1] << 16) | 
                      (data[fhdrOffset + 2] << 8) | data[fhdrOffset + 3];
            TRM_LOG_DEBUG("TRM: Extracted user ID from long address: 0x%08X", *userId);
            return 0;
        } else {
            TRM_LOG_WARN("TRM: Data too short for long address extraction");
            return -1;
        }
    }
}

int TRM_GetMacFrameQosInfo(const uint8_t* data, uint16_t len, uint8_t* qosPri, uint8_t* qosTtl)
{
    if (data == NULL || qosPri == NULL || qosTtl == NULL || len < 2) {
        TRM_LOG_ERROR("TRM: Invalid parameters for QoS info extraction");
        return -1;
    }
    
    /* 从MHDR第二个字节提取QoS信息 */
    uint8_t mhdrByte2 = data[TRM_MHDR_BYTE2_OFFSET];
    *qosPri = (mhdrByte2 & TRM_MHDR_QOSPRI_MASK) >> TRM_MHDR_QOSPRI_SHIFT;
    *qosTtl = (mhdrByte2 & TRM_MHDR_QOSTTL_MASK) >> TRM_MHDR_QOSTTL_SHIFT;
    
    TRM_LOG_DEBUG("TRM: Extracted QoS info - Pri=%d, TTL=%d", *qosPri, *qosTtl);
    return 0;
}

int TRM_BuildMacMhdr(const TrmMacMhdr* mhdr, uint8_t* data, uint16_t maxLen)
{
    if (mhdr == NULL || data == NULL || maxLen < 3) {
        TRM_LOG_ERROR("TRM: Invalid parameters for MHDR building");
        return -1;
    }
    
    uint16_t pos = 0;
    
    /* 构建第一个字节 */
    data[pos] = ((mhdr->frameType & TRM_MHDR_FRAMETYPE_MASK) << TRM_MHDR_FRAMETYPE_SHIFT) |
               ((mhdr->devType   & TRM_MHDR_DEVTYPE_MASK)   << TRM_MHDR_DEVTYPE_SHIFT)   |
               ((mhdr->version   & TRM_MHDR_VERSION_MASK)   << TRM_MHDR_VERSION_SHIFT);
    pos++;
    
    /* 构建第二个字节 (QoS相关) */
    data[pos] = ((mhdr->qosPri & TRM_MHDR_QOSPRI_MASK) << TRM_MHDR_QOSPRI_SHIFT) |
               ((mhdr->qosTtl & TRM_MHDR_QOSTTL_MASK) << TRM_MHDR_QOSTTL_SHIFT) |
               ((mhdr->hopNum & TRM_MHDR_HOPNUM_MASK) << TRM_MHDR_HOPNUM_SHIFT) |
               ((mhdr->hopCnt & TRM_MHDR_HOPCNT_MASK) << TRM_MHDR_HOPCNT_SHIFT);
    pos++;
    
    /* 构建第三个字节 */
    data[pos] = ((mhdr->securityMode & TRM_MHDR_SECURITYMODE_MASK) << TRM_MHDR_SECURITYMODE_SHIFT) |
               ((mhdr->addrMode      & TRM_MHDR_ADDRMODE_MASK)      << TRM_MHDR_ADDRMODE_SHIFT)     |
               ((mhdr->nwkMode       & TRM_MHDR_NWKMODE_MASK)       << TRM_MHDR_NWKMODE_SHIFT)      |
               ((mhdr->powerCtrlType & TRM_MHDR_POWERCTRL_MASK)     << TRM_MHDR_POWERCTRL_SHIFT)    |
               ((mhdr->rfu           & TRM_MHDR_RFU_MASK)           << TRM_MHDR_RFU_SHIFT);
    pos++;
    
    TRM_LOG_DEBUG("TRM: MHDR built - FrameType=0x%01X, DevType=%d, QosPri=%d, TTL=%d", 
                  mhdr->frameType, mhdr->devType, mhdr->qosPri, mhdr->qosTtl);
    
    return 3; /* MHDR固定3字节 */
}

int TRM_GetMacFrameType(const uint8_t* data, uint16_t len, uint8_t* frameType)
{
    if (data == NULL || frameType == NULL || len < 1) {
        TRM_LOG_ERROR("TRM: Invalid parameters for frame type detection");
        return -1;
    }
    
    /* 从第一个字节的高4位提取帧类型 */
    *frameType = (data[0] >> TRM_MHDR_FRAMETYPE_SHIFT) & TRM_MHDR_FRAMETYPE_MASK;
    
    TRM_LOG_DEBUG("TRM: Detected frame type: 0x%01X", *frameType);
    return 0;
}

int TRM_ParseMacFrame(const uint8_t* data, uint16_t len, TrmMacFrame* frame)
{
    if (data == NULL || frame == NULL || len < 3) {
        TRM_LOG_ERROR("TRM: Invalid parameters for MAC frame parsing");
        return -1;
    }
    
    /* 解析MHDR */
    if (TRM_ParseMacMhdr(data, len, &frame->mhdr) != 0) {
        return -1;
    }
    
    /* 计算FHDR偏移 */
    uint16_t fhdrOffset = 3;
    uint16_t remainingLen = len - fhdrOffset;
    
    if (remainingLen < 2) {
        TRM_LOG_WARN("TRM: Frame too short for FHDR parsing");
        return -1;
    }
    
    /* 解析FHDR - 源地址 */
    uint8_t addrLen = (frame->mhdr.addrMode == 0) ? 2 : 4;
    if (remainingLen < addrLen) {
        TRM_LOG_WARN("TRM: Frame too short for source address");
        return -1;
    }
    
    /* 复制源地址 */
    if (addrLen == 2) {
        frame->payload.fhdr.srcAddr[0] = data[fhdrOffset];
        frame->payload.fhdr.srcAddr[1] = data[fhdrOffset + 1];
        frame->payload.fhdr.srcAddr[2] = 0;
        frame->payload.fhdr.srcAddr[3] = 0;
    } else {
        frame->payload.fhdr.srcAddr[0] = data[fhdrOffset];
        frame->payload.fhdr.srcAddr[1] = data[fhdrOffset + 1];
        frame->payload.fhdr.srcAddr[2] = data[fhdrOffset + 2];
        frame->payload.fhdr.srcAddr[3] = data[fhdrOffset + 3];
    }
    
    /* 构建FCtrl字段 */
    if (remainingLen >= 1) {
        frame->payload.fhdr.fctrl.ack = (data[fhdrOffset + addrLen] >> 7) & 0x01;
        frame->payload.fhdr.fctrl.foptsNum = (data[fhdrOffset + addrLen] >> 3) & 0x0F;
        frame->payload.fhdr.fctrl.retranPkg = (data[fhdrOffset + addrLen] >> 2) & 0x01;
        frame->payload.fhdr.fctrl.subPkg = (data[fhdrOffset + addrLen] >> 1) & 0x01;
        frame->payload.fhdr.fctrl.rfu = data[fhdrOffset + addrLen] & 0x01;
    }
    
    /* 解析可选字段 */
    if (frame->payload.fhdr.fctrl.subPkg && remainingLen >= 2) {
        frame->payload.fhdr.subPkg.subPkgIdx = (data[fhdrOffset + addrLen + 1] >> 4) & 0x0F;
        frame->payload.fhdr.subPkg.subPkgNum = data[fhdrOffset + addrLen + 1] & 0x0F;
    }
    
    if (frame->payload.fhdr.fctrl.retranPkg && remainingLen >= 2) {
        frame->payload.fhdr.retrans.retransIdx = (data[fhdrOffset + addrLen + 1] >> 4) & 0x0F;
        frame->payload.fhdr.retrans.retransNum = data[fhdrOffset + addrLen + 1] & 0x0F;
    }
    
    /* 解析FOpts */
    uint8_t foptsLen = frame->payload.fhdr.fctrl.foptsNum;
    if (foptsLen > 0 && remainingLen >= foptsLen + addrLen + 1) {
        if (foptsLen > 15) foptsLen = 15; /* 限制最大长度 */
        for (uint8_t i = 0; i < foptsLen; i++) {
            frame->payload.fhdr.fopts[i] = data[fhdrOffset + addrLen + 1 + i];
        }
    }
    
    /* 解析功率控制字段 */
    if (frame->mhdr.nwkMode == TRM_MAC_NWKMODE_WAN && remainingLen >= addrLen + 1 + foptsLen) {
        frame->payload.fhdr.txPowerIdx = data[fhdrOffset + addrLen + 1 + foptsLen];
    }
    
    /* 解析FPort和负载 */
    if (remainingLen >= addrLen + 1 + foptsLen + 1) {
        frame->payload.fport = data[fhdrOffset + addrLen + 1 + foptsLen + 1];
    }
    
    /* WAN模式有负载长度字段 */
    if (frame->mhdr.nwkMode == TRM_MAC_NWKMODE_WAN && remainingLen >= addrLen + 1 + foptsLen + 2) {
        frame->payload.payloadLen = (data[fhdrOffset + addrLen + 1 + foptsLen + 1] << 8) | 
                                    data[fhdrOffset + addrLen + 1 + foptsLen + 2];
    } else {
        frame->payload.payloadLen = 0;
    }
    
    /* 解析负载数据 */
    if (remainingLen > addrLen + 1 + foptsLen + 2) {
        frame->payload.payload = (uint8_t*)&data[fhdrOffset + addrLen + 1 + foptsLen + 2];
        frame->payload.payloadSize = remainingLen - addrLen - 1 - foptsLen - 2;
    } else {
        frame->payload.payload = NULL;
        frame->payload.payloadSize = 0;
    }
    
    /* 解析MIC (如果存在) */
    if (frame->mhdr.securityMode != TRM_MAC_SECURITY_NONE && 
        frame->mhdr.securityMode != TRM_MAC_SECURITY_TEA_ONLY &&
        frame->mhdr.securityMode != TRM_MAC_SECURITY_AES_ONLY) {
        uint16_t micOffset = fhdrOffset + addrLen + 1 + foptsLen + 2 + frame->payload.payloadSize;
        if (len >= micOffset + 4) {
            frame->mic[0] = data[micOffset];
            frame->mic[1] = data[micOffset + 1];
            frame->mic[2] = data[micOffset + 2];
            frame->mic[3] = data[micOffset + 3];
        }
    }
    
    TRM_LOG_DEBUG("TRM: MAC frame parsed successfully - Type=0x%01X, PayloadLen=%d", 
                  frame->mhdr.frameType, frame->payload.payloadSize);
    
    return 0;
}

int TRM_BuildMacFrame(const TrmMacFrame* frame, uint8_t* data, uint16_t maxLen)
{
    if (frame == NULL || data == NULL || maxLen < 3) {
        TRM_LOG_ERROR("TRM: Invalid parameters for MAC frame building");
        return -1;
    }
    
    uint16_t pos = 0;
    
    /* 构建MHDR */
    int mhdrLen = TRM_BuildMacMhdr(&frame->mhdr, &data[pos], maxLen - pos);
    if (mhdrLen < 0) {
        return -1;
    }
    pos += mhdrLen;
    
    /* 构建FHDR */
    uint8_t addrLen = (frame->mhdr.addrMode == 0) ? 2 : 4;
    
    /* 源地址 */
    if (pos + addrLen > maxLen) {
        TRM_LOG_ERROR("TRM: Buffer too small for source address");
        return -1;
    }
    for (uint8_t i = 0; i < addrLen; i++) {
        data[pos + i] = frame->payload.fhdr.srcAddr[i];
    }
    pos += addrLen;
    
    /* FCtrl */
    if (pos + 1 > maxLen) {
        TRM_LOG_ERROR("TRM: Buffer too small for FCtrl");
        return -1;
    }
    data[pos] = ((frame->payload.fhdr.fctrl.ack & 0x01) << 7) |
               ((frame->payload.fhdr.fctrl.foptsNum & 0x0F) << 3) |
               ((frame->payload.fhdr.fctrl.retranPkg & 0x01) << 2) |
               ((frame->payload.fhdr.fctrl.subPkg & 0x01) << 1) |
               ((frame->payload.fhdr.fctrl.rfu & 0x01));
    pos++;
    
    /* FCnt */
    if (pos + 1 > maxLen) {
        TRM_LOG_ERROR("TRM: Buffer too small for FCnt");
        return -1;
    }
    data[pos] = frame->payload.fhdr.fcnt;
    pos++;
    
    /* SubPkg */
    if (frame->payload.fhdr.fctrl.subPkg) {
        if (pos + 1 > maxLen) {
            TRM_LOG_ERROR("TRM: Buffer too small for SubPkg");
            return -1;
        }
        data[pos] = ((frame->payload.fhdr.subPkg.subPkgIdx & 0x0F) << 4) | 
                    (frame->payload.fhdr.subPkg.subPkgNum & 0x0F);
        pos++;
    }
    
    /* Retrans */
    if (frame->payload.fhdr.fctrl.retranPkg) {
        if (pos + 1 > maxLen) {
            TRM_LOG_ERROR("TRM: Buffer too small for Retrans");
            return -1;
        }
        data[pos] = ((frame->payload.fhdr.retrans.retransIdx & 0x0F) << 4) | 
                    (frame->payload.fhdr.retrans.retransNum & 0x0F);
        pos++;
    }
    
    /* FOpts */
    uint8_t foptsLen = frame->payload.fhdr.fctrl.foptsNum;
    if (foptsLen > 0) {
        if (pos + foptsLen > maxLen) {
            TRM_LOG_ERROR("TRM: Buffer too small for FOpts");
            return -1;
        }
        for (uint8_t i = 0; i < foptsLen; i++) {
            data[pos + i] = frame->payload.fhdr.fopts[i];
        }
        pos += foptsLen;
    }
    
    /* 功率控制 */
    if (frame->mhdr.nwkMode == TRM_MAC_NWKMODE_WAN) {
        if (pos + 1 > maxLen) {
            TRM_LOG_ERROR("TRM: Buffer too small for TxPowerIdx");
            return -1;
        }
        data[pos] = frame->payload.fhdr.txPowerIdx;
        pos++;
    }
    
    /* FPort */
    if (pos + 1 > maxLen) {
        TRM_LOG_ERROR("TRM: Buffer too small for FPort");
        return -1;
    }
    data[pos] = frame->payload.fport;
    pos++;
    
    /* 负载长度 (WAN模式) */
    if (frame->mhdr.nwkMode == TRM_MAC_NWKMODE_WAN) {
        if (pos + 2 > maxLen) {
            TRM_LOG_ERROR("TRM: Buffer too small for PayloadLen");
            return -1;
        }
        data[pos] = (frame->payload.payloadLen >> 8) & 0xFF;
        data[pos + 1] = frame->payload.payloadLen & 0xFF;
        pos += 2;
    }
    
    /* 负载数据 */
    if (frame->payload.payloadSize > 0) {
        if (pos + frame->payload.payloadSize > maxLen) {
            TRM_LOG_ERROR("TRM: Buffer too small for payload data");
            return -1;
        }
        for (uint16_t i = 0; i < frame->payload.payloadSize; i++) {
            data[pos + i] = frame->payload.payload[i];
        }
        pos += frame->payload.payloadSize;
    }
    
    /* MIC */
    if (frame->mhdr.securityMode != TRM_MAC_SECURITY_NONE && 
        frame->mhdr.securityMode != TRM_MAC_SECURITY_TEA_ONLY &&
        frame->mhdr.securityMode != TRM_MAC_SECURITY_AES_ONLY) {
        if (pos + 4 > maxLen) {
            TRM_LOG_ERROR("TRM: Buffer too small for MIC");
            return -1;
        }
        data[pos] = frame->mic[0];
        data[pos + 1] = frame->mic[1];
        data[pos + 2] = frame->mic[2];
        data[pos + 3] = frame->mic[3];
        pos += 4;
    }
    
    TRM_LOG_DEBUG("TRM: MAC frame built successfully - Total length=%d", pos);
    return pos;
}
