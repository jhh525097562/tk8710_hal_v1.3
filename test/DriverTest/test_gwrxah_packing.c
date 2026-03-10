#include <stdio.h>
#include <stdint.h>

void test_gwrxah_packing(void)
{
    uint32_t gwrxahData[] = {
        6454, 9277, 9694, 3484, 2865, 2647, 1039272, 5056, 
        1045312, 4999, 1033527, 1048222, 1043941, 11566, 5877, 1046874
    };
    
    uint8_t spiDataBuffer[100];
    int byteIndex = 0;
    uint32_t bitBuffer = 0;
    int bitsInBuffer = 0;
    
    printf("GWRXAH 20bit数据打包过程:\n");
    
    for (int i = 0; i < 16; i++) {
        uint32_t ahValue = gwrxahData[i] & 0xFFFFF;  // 确保只取20bit
        printf("数据[%2d]: %6u -> 0x%05X\n", i, gwrxahData[i], ahValue);
        
        // 将20bit数据添加到位缓冲区
        bitBuffer = (bitBuffer << 20) | ahValue;
        bitsInBuffer += 20;
        
        printf("  位缓冲区: 0x%08X (%d位)\n", bitBuffer, bitsInBuffer);
        
        // 当缓冲区有8位或更多时，提取字节
        while (bitsInBuffer >= 8 && byteIndex < sizeof(spiDataBuffer) - 1) {
            uint8_t byte = (uint8_t)((bitBuffer >> (bitsInBuffer - 8)) & 0xFF);
            spiDataBuffer[byteIndex++] = byte;
            printf("  提取字节[%d]: 0x%02X\n", byteIndex-1, byte);
            bitsInBuffer -= 8;
        }
        printf("\n");
    }
    
    // 处理剩余的位
    if (bitsInBuffer > 0 && byteIndex < sizeof(spiDataBuffer)) {
        uint8_t byte = (uint8_t)((bitBuffer << (8 - bitsInBuffer)) & 0xFF);
        spiDataBuffer[byteIndex++] = byte;
        printf("剩余字节[%d]: 0x%02X\n", byteIndex-1, byte);
    }
    
    printf("\n最终SPI数据 (前%d字节): ", byteIndex);
    for (int i = 0; i < byteIndex; i++) {
        printf("0x%02X ", spiDataBuffer[i]);
    }
    printf("\n");
    
    printf("总字节数: %d\n", byteIndex);
}

int main()
{
    test_gwrxah_packing();
    return 0;
}
