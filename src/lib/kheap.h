#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>
#include <stddef.h>

// 記憶體區塊的標頭 (類似地契，記錄這塊地有多大、有沒有人住)
typedef struct block_header {
    uint32_t size;
    uint8_t is_free;
    struct block_header* next;
} block_header_t;

// 初始化 Kernel Heap
void init_kheap(void);

// 動態配置指定大小的記憶體 (類似 malloc)
void* kmalloc(size_t size);

// 釋放記憶體 (類似 free)
void kfree(void* ptr);

#endif
