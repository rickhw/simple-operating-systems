// PMM: Physical Memory Management
#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stdbool.h>

#define PMM_FRAME_SIZE 4096 // 每塊地的大小是 4KB

// 初始化 PMM，傳入系統總記憶體大小 (以 KB 為單位)
void init_pmm(uint32_t mem_size_kb);

// 索取一塊 4KB 的實體記憶體，回傳其記憶體位址
void* pmm_alloc_page(void);

// 釋放一塊 4KB 的實體記憶體
void pmm_free_page(void* ptr);

#endif
