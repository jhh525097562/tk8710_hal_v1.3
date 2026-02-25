/**
 * @file test_rk3506_hal.c
 * @brief TK8710 RK3506 平台 SPI 功能验证测试程序
 * @note 用于验证 tk8710_rk3506.c 移植层的 SPI 读写和中断功�? * 
 * 编译方法:
 *   arm-buildroot-linux-gnueabihf-gcc -I../inc -I../port test_rk3506_hal.c \
 *       ../port/tk8710_rk3506.c -o test_rk3506_hal -lgpiod -lpthread
 * 
 * 运行方法:
 *   ./test_rk3506_hal        交互式菜�? *   ./test_rk3506_hal -a     运行全部测试
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include "port/tk8710_hal.h"

/*============================================================================
 * 外部函数声明 (来自 tk8710_rk3506.c)
 *============================================================================*/

extern int TK8710SpiReset(uint8_t resetConfig);
extern int TK8710SpiWriteReg(uint16_t addr, const uint32_t* data, uint8_t regCount);
extern int TK8710SpiReadReg(uint16_t addr, uint32_t* data, uint8_t regCount);
extern int TK8710SpiWriteBuffer(uint8_t bufferIndex, const uint8_t* data, uint16_t len);
extern int TK8710SpiReadBuffer(uint8_t bufferIndex, uint8_t* data, uint16_t len);
extern int TK8710SpiSetInfo(uint8_t infoType, const uint8_t* data, uint16_t len);
extern int TK8710SpiGetInfo(uint8_t infoType, uint8_t* data, uint16_t len);
extern void TK8710Rk3506Cleanup(void);

/*============================================================================
 * 测试配置
 *============================================================================*/

#define TEST_SPI_SPEED          16000000    /* 测试SPI速度 16MHz */
#define TEST_IRQ_PIN            20          /* 中断引脚 (RM_IO20) */
#define TEST_TIMEOUT_SEC        5           /* 中断等待超时时间 */

/*============================================================================
 * 测试用全局变量
 *============================================================================*/

static volatile int g_irqCount = 0;
static volatile uint32_t g_lastIrqTime = 0;

/*============================================================================
 * 中断处理函数
 *============================================================================*/

/**
 * @brief TK8710 外部中断处理函数 (�?tk8710_rk3506.c 中的回调调用)
 */
void TK8710_IRQHandler(void)
{
    g_irqCount++;
    g_lastIrqTime = TK8710GetTickMs();
    printf("[IRQ] Triggered! Count: %d, Time: %u ms\n", g_irqCount, g_lastIrqTime);
}

/*============================================================================
 * 测试辅助函数
 *============================================================================*/

static void print_separator(void)
{
    printf("========================================\n");
}

static void print_test_header(const char* testName)
{
    printf("\n");
    print_separator();
    printf("Test: %s\n", testName);
    print_separator();
}

static void print_result(const char* item, int result)
{
    printf("  %-30s: %s\n", item, result == 0 ? "PASS" : "FAIL");
}

static void print_hex_data(const char* prefix, const uint8_t* data, uint16_t len)
{
    uint16_t i;
    printf("%s: ", prefix);
    for (i = 0; i < len && i < 32; i++) {
        printf("%02X ", data[i]);
    }
    if (len > 32) {
        printf("...");
    }
    printf("\n");
}

/*============================================================================
 * 测试用例
 *============================================================================*/

/**
 * @brief 测试1: SPI初始�? */
int test_spi_init(void)
{
    SpiConfig cfg;
    int ret;
    
    print_test_header("SPI Init");
    
    cfg.speed = TEST_SPI_SPEED;
    cfg.mode = 0;
    cfg.bits = 8;
    cfg.lsb_first = 0;
    cfg.cs_pin = 0;
    
    ret = TK8710SpiInit(&cfg);
    print_result("SPI Init (16MHz, Mode0)", ret);
    
    return ret;
}

/**
 * @brief 测试2: 芯片复位
 */
int test_chip_reset(void)
{
    int ret;
    
    print_test_header("Chip Reset");
    
    ret = TK8710SpiReset(2);  /* SM+REG 复位 */
    print_result("Send Reset CMD (SM+REG)", ret);
    
    TK8710DelayMs(10);
    
    return ret;
}

/**
 * @brief 测试3: 单个寄存器读�? */
int test_single_register_rw(void)
{
    int ret;
    uint32_t writeData;
    uint32_t readData = 0;
    uint16_t testAddr = 0xC020;
    int i;
    int passCount = 0;
    int failCount = 0;
    
    uint32_t testPatterns[] = {
        0x12345678,
        0xAAAAAAAA,
        0x55555555,
        0xFFFFFFFF,
        0x00000000,
        0xDEADBEEF,
        0xA5A5A5A5,
        0x5A5A5A5A
    };
    int numTests = sizeof(testPatterns) / sizeof(testPatterns[0]);
    
    print_test_header("Single Register R/W (Multi-Test)");
    printf("  Test Address: 0x%04X\n", testAddr);
    printf("  Running %d write-read cycles...\n\n", numTests);
    
    for (i = 0; i < numTests; i++) {
        writeData = testPatterns[i];
        readData = 0;
        
        ret = TK8710SpiWriteReg(testAddr, &writeData, 1);
        if (ret != 0) {
            printf("  [%d] Write 0x%08X: FAIL (write error)\n", i + 1, writeData);
            failCount++;
            continue;
        }
        
        ret = TK8710SpiReadReg(testAddr, &readData, 1);
        if (ret != 0) {
            printf("  [%d] Write 0x%08X: FAIL (read error)\n", i + 1, writeData);
            failCount++;
            continue;
        }
        
        if (readData == writeData) {
            printf("  [%d] Write 0x%08X, Read 0x%08X: PASS\n", i + 1, writeData, readData);
            passCount++;
        } else {
            printf("  [%d] Write 0x%08X, Read 0x%08X: FAIL\n", i + 1, writeData, readData);
            failCount++;
        }
    }
    
    printf("\n  ========== Summary ==========\n");
    printf("  Total: %d, Pass: %d, Fail: %d\n", numTests, passCount, failCount);
    printf("  Result: %s\n", (failCount == 0) ? "ALL PASS" : "HAS FAILURES");
    
    return (failCount == 0) ? 0 : -1;
}

/**
 * @brief 测试4: 批量寄存器读�?(0x9024-0x9060)
 */
int test_multi_register_rw(void)
{
    int ret;
    uint16_t startAddr = 0x9024;
    uint16_t endAddr = 0x9060;
    int regCount = (endAddr - startAddr) / 4;
    uint32_t writeData[16];
    uint32_t readData[16];
    int i;
    int passCount = 0;
    int failCount = 0;
    
    print_test_header("Multi Register R/W (0x9024-0x9060)");
    printf("  Address Range: 0x%04X - 0x%04X\n", startAddr, endAddr);
    printf("  Register Count: %d\n\n", regCount);
    
    for (i = 0; i < regCount; i++) {
        writeData[i] = 0xA5000000 | (i << 16) | (startAddr + i * 4);
    }
    
    printf("  Writing %d registers...\n", regCount);
    ret = TK8710SpiWriteReg(startAddr, writeData, regCount);
    print_result("Write Multi Registers", ret);
    if (ret != 0) {
        return ret;
    }
    
    memset(readData, 0, sizeof(readData));
    ret = TK8710SpiReadReg(startAddr, readData, regCount);
    print_result("Read Multi Registers", ret);
    if (ret != 0) {
        return ret;
    }
    
    printf("\n  Verify Results:\n");
    for (i = 0; i < regCount; i++) {
        uint16_t addr = startAddr + i * 4;
        if (readData[i] == writeData[i]) {
            printf("  [0x%04X] W:0x%08X R:0x%08X OK\n", addr, writeData[i], readData[i]);
            passCount++;
        } else {
            printf("  [0x%04X] W:0x%08X R:0x%08X ERR\n", addr, writeData[i], readData[i]);
            failCount++;
        }
    }
    
    printf("\n  ========== Summary ==========\n");
    printf("  Total: %d, Pass: %d, Fail: %d\n", regCount, passCount, failCount);
    printf("  Result: %s\n", (failCount == 0) ? "ALL PASS" : "HAS FAILURES");
    
    return (failCount == 0) ? 0 : -1;
}

/**
 * @brief 测试5: GPIO中断
 */
int test_gpio_interrupt(void)
{
    int ret;
    int timeout;
    
    print_test_header("GPIO Interrupt Test");
    
    g_irqCount = 0;
    ret = TK8710GpioInit(TEST_IRQ_PIN, TK8710_GPIO_EDGE_FALLING, NULL, NULL);
    print_result("GPIO IRQ Init", ret);
    
    if (ret != 0) {
        printf("  Skip IRQ test (init failed)\n");
        return ret;
    }
    
    ret = TK8710GpioIrqEnable(TEST_IRQ_PIN, 1);
    print_result("Enable IRQ", ret);
    
    printf("\n  Waiting for IRQ (press chip IRQ pin or wait timeout)...\n");
    printf("  (If chip not connected, this test will timeout)\n");
    
    timeout = 50;  /* 5秒超�?*/
    while (g_irqCount == 0 && timeout > 0) {
        TK8710DelayMs(100);
        timeout--;
        printf(".");
        fflush(stdout);
    }
    printf("\n");
    
    if (g_irqCount > 0) {
        printf("  IRQ Test: PASS (received %d interrupts)\n", g_irqCount);
    } else {
        printf("  IRQ Test: TIMEOUT (no interrupt, chip may not connected)\n");
    }
    
    TK8710GpioIrqEnable(TEST_IRQ_PIN, 0);
    
    return 0;
}

/**
 * @brief 测试6: 读取芯片信息
 */
int test_read_chip_info(void)
{
    int ret;
    uint32_t regValue = 0;
    
    print_test_header("Read Chip Info");
    
    ret = TK8710SpiReadReg(0xc110, &regValue, 1);
    printf("  Reg 0xc110: 0x%08X (ret=%d)\n", regValue, ret);
    
    ret = TK8710SpiReadReg(0xc114, &regValue, 1);
    printf("  Reg 0xc114: 0x%08X (ret=%d)\n", regValue, ret);
    
    ret = TK8710SpiReadReg(0xc118, &regValue, 1);
    printf("  Reg 0xc118: 0x%08X (ret=%d)\n", regValue, ret);
    
    return 0;
}

/**
 * @brief 测试7: Buffer读写
 */
int test_buffer_rw(void)
{
    uint8_t writeData[64];
    uint8_t readData[64];
    int i;
    int ret;
    int passCount = 0;
    
    print_test_header("Buffer R/W Test");
    
    /* 生成测试数据 */
    for (i = 0; i < 64; i++) {
        writeData[i] = (uint8_t)(i * 3 + 0x10);
    }
    
    printf("  Writing 64 bytes to Buffer[0]...\n");
    ret = TK8710SpiWriteBuffer(0, writeData, 64);
    print_result("Write Buffer", ret);
    if (ret != 0) {
        return ret;
    }
    
    memset(readData, 0, sizeof(readData));
    ret = TK8710SpiReadBuffer(0, readData, 64);
    print_result("Read Buffer", ret);
    if (ret != 0) {
        return ret;
    }
    
    print_hex_data("  Write Data", writeData, 16);
    print_hex_data("  Read Data ", readData, 16);
    
    for (i = 0; i < 64; i++) {
        if (writeData[i] == readData[i]) {
            passCount++;
        }
    }
    
    printf("  Data Match: %d/64\n", passCount);
    printf("  Result: %s\n", (passCount == 64) ? "ALL PASS" : "HAS FAILURES");
    
    return (passCount == 64) ? 0 : -1;
}

/*============================================================================
 * 交互式测试菜�? *============================================================================*/

void show_menu(void)
{
    printf("\n");
    print_separator();
    printf("TK8710 RK3506 Test Menu\n");
    print_separator();
    printf("1. Init SPI\n");
    printf("2. Chip Reset\n");
    printf("3. Single Register R/W Test\n");
    printf("4. Multi Register R/W Test\n");
    printf("5. Buffer R/W Test\n");
    printf("6. GPIO IRQ Test\n");
    printf("7. Read Chip Info\n");
    printf("8. Run All Tests\n");
    printf("0. Exit\n");
    print_separator();
    printf("Select: ");
}

void run_all_tests(void)
{
    printf("\n\n=== Run All Tests ===\n");
    
    if (test_spi_init() != 0) {
        printf("\nSPI init failed, cannot continue\n");
        return;
    }
    
    test_chip_reset();
    test_single_register_rw();
    test_multi_register_rw();
    test_buffer_rw();
    test_read_chip_info();
    test_gpio_interrupt();
    
    printf("\n=== Tests Complete ===\n");
}

/*============================================================================
 * 主函�? *============================================================================*/

int main(int argc, char* argv[])
{
    int choice;
    int running = 1;
    
    printf("\n");
    printf("+--------------------------------------+\n");
    printf("|   TK8710 RK3506 SPI Test Program     |\n");
    printf("|   Version: 1.0                       |\n");
    printf("+--------------------------------------+\n");
    
    /* 检查命令行参数 */
    if (argc > 1 && strcmp(argv[1], "-a") == 0) {
        if (test_spi_init() == 0) {
            run_all_tests();
        }
        TK8710Rk3506Cleanup();
        return 0;
    }
    
    /* 交互式菜�?*/
    while (running) {
        show_menu();
        
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');  /* 清除输入缓冲 */
            continue;
        }
        
        switch (choice) {
            case 1: test_spi_init(); break;
            case 2: test_chip_reset(); break;
            case 3: test_single_register_rw(); break;
            case 4: test_multi_register_rw(); break;
            case 5: test_buffer_rw(); break;
            case 6: test_gpio_interrupt(); break;
            case 7: test_read_chip_info(); break;
            case 8: run_all_tests(); break;
            case 0:
                running = 0;
                break;
            default:
                printf("Invalid choice\n");
                break;
        }
    }
    
    /* 清理资源 */
    TK8710Rk3506Cleanup();
    printf("\nTest program finished\n");
    
    return 0;
}
