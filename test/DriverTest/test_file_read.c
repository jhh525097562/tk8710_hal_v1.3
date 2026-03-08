#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int test_file_reading(void)
{
    char filePath[256];
    FILE *fp;
    char lineBuffer[4096];
    int dataCount;
    
    printf("=== 测试文件读取功能 ===\n");
    
    /* 1. 测试 ANoise 数据 */
    snprintf(filePath, sizeof(filePath), 
        "D:\\data\\K2301\\K2301PHY_CaseList_Data\\Simulation_Database\\class3\\case2\\ANoise.txt");
    fp = fopen(filePath, "r");
    if (fp == NULL) {
        printf("无法打开文件: %s\n", filePath);
        return -1;
    }
    
    if (fgets(lineBuffer, sizeof(lineBuffer), fp) != NULL) {
        uint16_t anoiseData[8];
        dataCount = sscanf(lineBuffer, "%hu %hu %hu %hu %hu %hu %hu %hu",
                          &anoiseData[0], &anoiseData[1], &anoiseData[2], &anoiseData[3],
                          &anoiseData[4], &anoiseData[5], &anoiseData[6], &anoiseData[7]);
        
        if (dataCount == 8) {
            printf("ANoise 数据读取成功: ");
            for (int i = 0; i < 8; i++) {
                printf("%u ", anoiseData[i]);
            }
            printf("\n");
        }
    }
    fclose(fp);
    
    /* 2. 测试 GWRXAH 数据 - 读取所有内容 */
    snprintf(filePath, sizeof(filePath), 
        "D:\\data\\K2301\\K2301PHY_CaseList_Data\\Simulation_Database\\class3\\case2\\GWRXAH.txt");
    fp = fopen(filePath, "r");
    if (fp == NULL) {
        printf("无法打开文件: %s\n", filePath);
        return -1;
    }
    
    uint16_t gwrxahData[1000];  // 增加缓冲区大小
    dataCount = 0;
    
    while (fgets(lineBuffer, sizeof(lineBuffer), fp) != NULL && dataCount < 1000) {
        char *ptr = lineBuffer;
        while (*ptr != '\0' && dataCount < 1000) {
            uint16_t value;
            if (sscanf(ptr, "%hu", &value) == 1) {
                gwrxahData[dataCount++] = value;
                while (*ptr != '\0' && *ptr != ' ' && *ptr != '\t' && *ptr != '\n') ptr++;
                while (*ptr != '\0' && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n')) ptr++;
            } else {
                break;
            }
        }
    }
    fclose(fp);
    
    if (dataCount > 0) {
        printf("GWRXAH 总共读取 %d 个数据: ", dataCount);
        for (int i = 0; i < dataCount && i < 20; i++) {
            printf("%u ", gwrxahData[i]);
        }
        printf("...\n");
    }
    
    /* 3. 测试 TxPower 数据 - 读取所有内容 */
    snprintf(filePath, sizeof(filePath), 
        "D:\\data\\K2301\\K2301PHY_CaseList_Data\\Simulation_Database\\class3\\case2\\TxPower.txt");
    fp = fopen(filePath, "r");
    if (fp == NULL) {
        printf("无法打开文件: %s\n", filePath);
        return -1;
    }
    
    uint16_t txPowerData[500];  // 增加缓冲区大小
    dataCount = 0;
    
    while (fgets(lineBuffer, sizeof(lineBuffer), fp) != NULL && dataCount < 500) {
        char *ptr = lineBuffer;
        while (*ptr != '\0' && dataCount < 500) {
            uint16_t value;
            if (sscanf(ptr, "%hu", &value) == 1) {
                txPowerData[dataCount++] = value;
                while (*ptr != '\0' && *ptr != ' ' && *ptr != '\t' && *ptr != '\n') ptr++;
                while (*ptr != '\0' && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n')) ptr++;
            } else {
                break;
            }
        }
    }
    fclose(fp);
    
    if (dataCount > 0) {
        printf("TxPower 总共读取 %d 个数据: ", dataCount);
        for (int i = 0; i < dataCount && i < 20; i++) {
            printf("%u ", txPowerData[i]);
        }
        printf("...\n");
    }
    
    /* 4. 测试 GWRxPilotPower 数据 - 读取所有内容 */
    snprintf(filePath, sizeof(filePath), 
        "D:\\data\\K2301\\K2301PHY_CaseList_Data\\Simulation_Database\\class3\\case2\\GWRxPilotPower.txt");
    fp = fopen(filePath, "r");
    if (fp == NULL) {
        printf("无法打开文件: %s\n", filePath);
        return -1;
    }
    
    uint16_t pilotPowerData[500];  // 增加缓冲区大小
    dataCount = 0;
    
    while (fgets(lineBuffer, sizeof(lineBuffer), fp) != NULL && dataCount < 500) {
        char *ptr = lineBuffer;
        while (*ptr != '\0' && dataCount < 500) {
            uint16_t value;
            if (sscanf(ptr, "%hu", &value) == 1) {
                pilotPowerData[dataCount++] = value;
                while (*ptr != '\0' && *ptr != ' ' && *ptr != '\t' && *ptr != '\n') ptr++;
                while (*ptr != '\0' && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n')) ptr++;
            } else {
                break;
            }
        }
    }
    fclose(fp);
    
    if (dataCount > 0) {
        printf("GWRxPilotPower 总共读取 %d 个数据: ", dataCount);
        for (int i = 0; i < dataCount && i < 20; i++) {
            printf("%u ", pilotPowerData[i]);
        }
        printf("...\n");
    }
    
    /* 5. 测试 TxFreq 数据 - 读取所有内容 */
    snprintf(filePath, sizeof(filePath), 
        "D:\\data\\K2301\\K2301PHY_CaseList_Data\\Simulation_Database\\class3\\case2\\TxFreq.txt");
    fp = fopen(filePath, "r");
    if (fp == NULL) {
        printf("无法打开文件: %s\n", filePath);
        return -1;
    }
    
    uint32_t txFreqData[500];  // 增加缓冲区大小
    dataCount = 0;
    
    while (fgets(lineBuffer, sizeof(lineBuffer), fp) != NULL && dataCount < 500) {
        char *ptr = lineBuffer;
        while (*ptr != '\0' && dataCount < 500) {
            uint32_t value;
            if (sscanf(ptr, "%u", &value) == 1) {
                txFreqData[dataCount++] = value;
                while (*ptr != '\0' && *ptr != ' ' && *ptr != '\t' && *ptr != '\n') ptr++;
                while (*ptr != '\0' && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n')) ptr++;
            } else {
                break;
            }
        }
    }
    fclose(fp);
    
    if (dataCount > 0) {
        printf("TxFreq 总共读取 %d 个数据: ", dataCount);
        for (int i = 0; i < dataCount && i < 20; i++) {
            printf("%u ", txFreqData[i]);
        }
        printf("...\n");
    }
    
    printf("=== 文件读取测试完成 ===\n");
    return 0;
}

int main()
{
    return test_file_reading();
}
