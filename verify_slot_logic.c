/**
 * @file verify_slot_logic.c
 * @brief 验证时隙长度计算逻辑（x86版本，用于本地测试）
 */

#include <stdio.h>
#include <stdint.h>

// 模拟常量定义
#define INTERVAL_US     1024

// 模拟的常量数组（简化版本，只包含mode 8）
static const uint32_t g_bcnSlotLen[] = {
    [8] = 10732,
};

static const uint32_t g_brdBaseBody[] = {
    [8] = 16384,
};

static const uint32_t g_brdBaseGap[] = {
    [8] = 5600,
};

static const uint32_t g_ulBaseBody[] = {
    [8] = 16384,
};

static const uint32_t g_ulBaseGap[] = {
    [8] = 5600,
};

static const uint32_t g_dlBaseBody[] = {
    [8] = 16384,
};

static const uint32_t g_dlBaseGap[] = {
    [8] = 5600,
};

// 模拟的输入输出结构
typedef struct {
    uint8_t  rateMode;
    uint8_t  brdBlockNum;
    uint8_t  ulBlockNum;
    uint8_t  dlBlockNum;
    uint8_t  superFrameNum;
} TestInput;

typedef struct {
    uint32_t bcnSlotLen;
    uint32_t brdSlotLen;
    uint32_t ulSlotLen;
    uint32_t dlSlotLen;
} TestOutput;

// 修改后的时隙计算逻辑
void test_calc_slot_config(const TestInput* input, TestOutput* output) {
    uint8_t mode = input->rateMode;
    
    /* 计算各时隙长度 */
    output->bcnSlotLen = g_bcnSlotLen[mode];
    
    /* 如果包块数为0，对应时隙长度为0 */
    if (input->brdBlockNum == 0) {
        output->brdSlotLen = 0;
    } else {
        output->brdSlotLen = INTERVAL_US + g_brdBaseBody[mode] * (input->brdBlockNum * 2 + 1) + g_brdBaseGap[mode];
    }
    
    if (input->ulBlockNum == 0) {
        output->ulSlotLen = 0;
    } else {
        output->ulSlotLen = INTERVAL_US + g_ulBaseBody[mode] * (input->ulBlockNum * 2 + 1) + g_ulBaseGap[mode];
    }
    
    if (input->dlBlockNum == 0) {
        output->dlSlotLen = 0;
    } else {
        output->dlSlotLen = INTERVAL_US + g_dlBaseBody[mode] * (input->dlBlockNum * 2 + 1) + g_dlBaseGap[mode];
    }
}

int main() {
    printf("验证时隙长度计算逻辑\n");
    printf("====================\n\n");
    
    // 测试用例1: 所有包块数都为0
    TestInput input1 = {8, 0, 0, 0, 1};
    TestOutput output1;
    
    test_calc_slot_config(&input1, &output1);
    
    printf("测试用例1: 所有包块数都为0\n");
    printf("输入: rateMode=%d, brdBlockNum=%d, ulBlockNum=%d, dlBlockNum=%d\n",
           input1.rateMode, input1.brdBlockNum, input1.ulBlockNum, input1.dlBlockNum);
    printf("输出时隙长度: BCN=%u, BRD=%u, UL=%u, DL=%u\n",
           output1.bcnSlotLen, output1.brdSlotLen, output1.ulSlotLen, output1.dlSlotLen);
    printf("预期: BRD=0, UL=0, DL=0\n");
    printf("实际: %s\n\n", 
           (output1.brdSlotLen == 0 && output1.ulSlotLen == 0 && output1.dlSlotLen == 0) ? "✅ 正确" : "❌ 错误");
    
    // 测试用例2: 部分包块数为0
    TestInput input2 = {8, 2, 0, 1, 1};
    TestOutput output2;
    
    test_calc_slot_config(&input2, &output2);
    
    printf("测试用例2: 部分包块数为0\n");
    printf("输入: rateMode=%d, brdBlockNum=%d, ulBlockNum=%d, dlBlockNum=%d\n",
           input2.rateMode, input2.brdBlockNum, input2.ulBlockNum, input2.dlBlockNum);
    printf("输出时隙长度: BCN=%u, BRD=%u, UL=%u, DL=%u\n",
           output2.bcnSlotLen, output2.brdSlotLen, output2.ulSlotLen, output2.dlSlotLen);
    printf("预期: BRD>0, UL=0, DL>0\n");
    printf("实际: %s\n\n", 
           (output2.brdSlotLen > 0 && output2.ulSlotLen == 0 && output2.dlSlotLen > 0) ? "✅ 正确" : "❌ 错误");
    
    // 测试用例3: 所有包块数都大于0（正常情况）
    TestInput input3 = {8, 1, 2, 1, 1};
    TestOutput output3;
    
    test_calc_slot_config(&input3, &output3);
    
    printf("测试用例3: 所有包块数都大于0（正常情况）\n");
    printf("输入: rateMode=%d, brdBlockNum=%d, ulBlockNum=%d, dlBlockNum=%d\n",
           input3.rateMode, input3.brdBlockNum, input3.ulBlockNum, input3.dlBlockNum);
    printf("输出时隙长度: BCN=%u, BRD=%u, UL=%u, DL=%u\n",
           output3.bcnSlotLen, output3.brdSlotLen, output3.ulSlotLen, output3.dlSlotLen);
    printf("预期: 所有时隙长度都>0\n");
    printf("实际: %s\n\n", 
           (output3.brdSlotLen > 0 && output3.ulSlotLen > 0 && output3.dlSlotLen > 0) ? "✅ 正确" : "❌ 错误");
    
    // 手动计算验证测试用例2
    printf("手动计算验证测试用例2:\n");
    printf("BRD时隙长度 (2个包块): %u + %u * (2*2+1) + %u = %u\n",
           INTERVAL_US, g_brdBaseBody[8], g_brdBaseGap[8],
           INTERVAL_US + g_brdBaseBody[8] * (2*2+1) + g_brdBaseGap[8]);
    printf("UL时隙长度 (0个包块): 0\n");
    printf("DL时隙长度 (1个包块): %u + %u * (1*2+1) + %u = %u\n",
           INTERVAL_US, g_dlBaseBody[8], g_dlBaseGap[8],
           INTERVAL_US + g_dlBaseBody[8] * (1*2+1) + g_dlBaseGap[8]);
    
    printf("\n验证完成！\n");
    return 0;
}
