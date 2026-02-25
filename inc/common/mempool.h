/**
 * @file mempool.h
 * @brief 内存池管理
 * @version 1.0
 * @date 2026-02-09
 */

#ifndef MEMPOOL_H
#define MEMPOOL_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 配置定义
 *============================================================================*/
#define MEMPOOL_RX_BLOCK_SIZE   1024    /* 接收块大小(字节) */
#define MEMPOOL_RX_BLOCK_COUNT  16      /* 接收块数量 */
#define MEMPOOL_TX_BLOCK_SIZE   512     /* 发送块大小(字节) */
#define MEMPOOL_TX_BLOCK_COUNT  32      /* 发送块数量 */

/*==============================================================================
 * 类型定义
 *============================================================================*/

/* 内存池结构 */
typedef struct {
    uint8_t* buffer;            /* 预分配缓冲区 */
    uint32_t blockSize;         /* 块大小 */
    uint32_t blockCount;        /* 块数量 */
    uint32_t usedMask;          /* 使用位图(最多支持32块) */
    uint32_t allocCount;        /* 分配计数 */
    uint32_t freeCount;         /* 释放计数 */
} MemPool;

/*==============================================================================
 * API
 *============================================================================*/

/**
 * @brief 初始化内存池
 * @param pool 内存池结构
 * @param buffer 预分配缓冲区
 * @param blockSize 块大小
 * @param blockCount 块数量(最大32)
 * @return 0成功，其他失败
 */
int MemPool_Init(MemPool* pool, uint8_t* buffer, uint32_t blockSize, uint32_t blockCount);

/**
 * @brief 从内存池分配
 * @param pool 内存池
 * @return 内存指针，失败返回NULL
 */
void* MemPool_Alloc(MemPool* pool);

/**
 * @brief 释放到内存池
 * @param pool 内存池
 * @param ptr 内存指针
 */
void MemPool_Free(MemPool* pool, void* ptr);

/**
 * @brief 获取空闲块数量
 * @param pool 内存池
 * @return 空闲块数量
 */
int MemPool_GetFreeCount(MemPool* pool);

/**
 * @brief 重置内存池
 * @param pool 内存池
 */
void MemPool_Reset(MemPool* pool);

#ifdef __cplusplus
}
#endif

#endif /* MEMPOOL_H */
