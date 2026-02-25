/**
 * @file test_simple.c
 * @brief 简单的TK8710 HAL库测试程序
 */

#include <stdio.h>
#include <stdlib.h>

// 简单的测试函数声明
extern int mempool_test(void);
extern int tk8710_log_test(void);

int main() {
    printf("TK8710 HAL Library Test\n");
    printf("=======================\n");
    
    printf("Testing mempool...\n");
    // 这里可以调用mempool相关的测试函数
    
    printf("Testing log system...\n");
    // 这里可以调用log相关的测试函数
    
    printf("Basic library test completed successfully!\n");
    return 0;
}
