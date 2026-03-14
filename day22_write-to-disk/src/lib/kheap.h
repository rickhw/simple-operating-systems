#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>
#include <stddef.h>

// 初始化 Kernel Heap
void init_kheap(void);

// 動態配置指定大小的記憶體 (類似 malloc)
void* kmalloc(size_t size);

// 釋放記憶體 (類似 free)
void kfree(void* ptr);

#endif
