/**
 * @file mempool.c
 * @brief 内存池管理实现
 * @version 1.0
 * @date 2026-02-09
 */

#include "common/mempool.h"
#include <string.h>

int MemPool_Init(MemPool* pool, uint8_t* buffer, uint32_t blockSize, uint32_t blockCount)
{
    if (pool == NULL || buffer == NULL || blockSize == 0 || blockCount == 0) {
        return -1;
    }
    
    if (blockCount > 32) {
        return -1;  /* 位图最多支持32块 */
    }
    
    pool->buffer = buffer;
    pool->blockSize = blockSize;
    pool->blockCount = blockCount;
    pool->usedMask = 0;
    pool->allocCount = 0;
    pool->freeCount = 0;
    
    return 0;
}

void* MemPool_Alloc(MemPool* pool)
{
    if (pool == NULL) {
        return NULL;
    }
    
    /* 查找空闲块 */
    for (uint32_t i = 0; i < pool->blockCount; i++) {
        if ((pool->usedMask & (1U << i)) == 0) {
            pool->usedMask |= (1U << i);
            pool->allocCount++;
            return pool->buffer + (i * pool->blockSize);
        }
    }
    
    return NULL;  /* 无空闲块 */
}

void MemPool_Free(MemPool* pool, void* ptr)
{
    if (pool == NULL || ptr == NULL) {
        return;
    }
    
    /* 计算块索引 */
    uint8_t* p = (uint8_t*)ptr;
    if (p < pool->buffer) {
        return;
    }
    
    uint32_t offset = (uint32_t)(p - pool->buffer);
    uint32_t index = offset / pool->blockSize;
    
    if (index >= pool->blockCount) {
        return;
    }
    
    /* 检查是否对齐 */
    if (offset % pool->blockSize != 0) {
        return;
    }
    
    /* 释放块 */
    if (pool->usedMask & (1U << index)) {
        pool->usedMask &= ~(1U << index);
        pool->freeCount++;
    }
}

int MemPool_GetFreeCount(MemPool* pool)
{
    if (pool == NULL) {
        return 0;
    }
    
    int count = 0;
    for (uint32_t i = 0; i < pool->blockCount; i++) {
        if ((pool->usedMask & (1U << i)) == 0) {
            count++;
        }
    }
    
    return count;
}

void MemPool_Reset(MemPool* pool)
{
    if (pool == NULL) {
        return;
    }
    
    pool->usedMask = 0;
    pool->allocCount = 0;
    pool->freeCount = 0;
}
