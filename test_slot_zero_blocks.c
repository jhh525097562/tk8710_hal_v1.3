/**
 * @file test_slot_zero_blocks.c
 * @brief 测试当包块数为0时时隙长度计算是否正确
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "inc/trm/trm_api.h"

int main() {
    printf("测试包块数为0时的时隙长度计算\n");
    printf("================================\n\n");
    
    // 测试用例1: 所有包块数都为0
    TRM_SlotCalcInput input1 = {
        .rateMode = 8,
        .brdBlockNum = 0,
        .ulBlockNum = 0,
        .dlBlockNum = 0,
        .superFrameNum = 1
    };
    
    TRM_SlotCalcOutput output1;
    memset(&output1, 0, sizeof(output1));
    
    int result1 = trm_calc_slot_config(&input1, &output1);
    
    printf("测试用例1: 所有包块数都为0\n");
    printf("输入: rateMode=%d, brdBlockNum=%d, ulBlockNum=%d, dlBlockNum=%d\n",
           input1.rateMode, input1.brdBlockNum, input1.ulBlockNum, input1.dlBlockNum);
    printf("结果: %s\n", result1 == 0 ? "成功" : "失败");
    printf("输出时隙长度: BCN=%u, BRD=%u, UL=%u, DL=%u\n",
           output1.bcnSlotLen, output1.brdSlotLen, output1.ulSlotLen, output1.dlSlotLen);
    printf("预期: BRD=0, UL=0, DL=0\n");
    printf("实际: %s\n\n", 
           (output1.brdSlotLen == 0 && output1.ulSlotLen == 0 && output1.dlSlotLen == 0) ? "✅ 正确" : "❌ 错误");
    
    // 测试用例2: 部分包块数为0
    TRM_SlotCalcInput input2 = {
        .rateMode = 8,
        .brdBlockNum = 2,
        .ulBlockNum = 0,
        .dlBlockNum = 1,
        .superFrameNum = 1
    };
    
    TRM_SlotCalcOutput output2;
    memset(&output2, 0, sizeof(output2));
    
    int result2 = trm_calc_slot_config(&input2, &output2);
    
    printf("测试用例2: 部分包块数为0\n");
    printf("输入: rateMode=%d, brdBlockNum=%d, ulBlockNum=%d, dlBlockNum=%d\n",
           input2.rateMode, input2.brdBlockNum, input2.ulBlockNum, input2.dlBlockNum);
    printf("结果: %s\n", result2 == 0 ? "成功" : "失败");
    printf("输出时隙长度: BCN=%u, BRD=%u, UL=%u, DL=%u\n",
           output2.bcnSlotLen, output2.brdSlotLen, output2.ulSlotLen, output2.dlSlotLen);
    printf("预期: BRD>0, UL=0, DL>0\n");
    printf("实际: %s\n\n", 
           (output2.brdSlotLen > 0 && output2.ulSlotLen == 0 && output2.dlSlotLen > 0) ? "✅ 正确" : "❌ 错误");
    
    // 测试用例3: 所有包块数都大于0（正常情况）
    TRM_SlotCalcInput input3 = {
        .rateMode = 8,
        .brdBlockNum = 1,
        .ulBlockNum = 2,
        .dlBlockNum = 1,
        .superFrameNum = 1
    };
    
    TRM_SlotCalcOutput output3;
    memset(&output3, 0, sizeof(output3));
    
    int result3 = trm_calc_slot_config(&input3, &output3);
    
    printf("测试用例3: 所有包块数都大于0（正常情况）\n");
    printf("输入: rateMode=%d, brdBlockNum=%d, ulBlockNum=%d, dlBlockNum=%d\n",
           input3.rateMode, input3.brdBlockNum, input3.ulBlockNum, input3.dlBlockNum);
    printf("结果: %s\n", result3 == 0 ? "成功" : "失败");
    printf("输出时隙长度: BCN=%u, BRD=%u, UL=%u, DL=%u\n",
           output3.bcnSlotLen, output3.brdSlotLen, output3.ulSlotLen, output3.dlSlotLen);
    printf("预期: 所有时隙长度都>0\n");
    printf("实际: %s\n\n", 
           (output3.brdSlotLen > 0 && output3.ulSlotLen > 0 && output3.dlSlotLen > 0) ? "✅ 正确" : "❌ 错误");
    
    printf("测试完成！\n");
    return 0;
}
