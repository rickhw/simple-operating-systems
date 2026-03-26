#include "kheap.h"
#include "pmm.h"
#include "paging.h"
#include "utils.h"
#include "tty.h"

// 記憶體區塊的標頭 (類似地契，記錄這塊地有多大、有沒有人住)
typedef struct block_header {
    uint32_t size;
    uint8_t is_free;
    struct block_header* next;
} block_header_t;

block_header_t* first_block = 0;

void init_kheap() {
    kprintf("[Heap] Initializing Kernel Heap at 0xC0000000...\n");

    // 1. 批發 32 個實體分頁 (32 * 4KB = 128 KB)，這對目前的 OS 來說是一塊超大平原！
    for (int i = 0; i < 32; i++) {
        uint32_t phys_addr = pmm_alloc_page();
        map_page(0xC0000000 + (i * 4096), phys_addr, 3);
    }

    // 2. 在這塊平原的最開頭，插上第一塊地契
    first_block = (block_header_t*) 0xC0000000;
    first_block->size = (32 * 4096) - sizeof(block_header_t); // 總空間扣掉地契本身的大小
    first_block->is_free = 1;
    first_block->next = 0;
}

void* kmalloc(uint32_t size) {
    // 記憶體對齊：為了硬體效能，申請的大小必須是 4 的倍數
    uint32_t aligned_size = (size + 3) & ~3;

    block_header_t* current = first_block;

    while (current != 0) {
        if (current->is_free && current->size >= aligned_size) {
            // 【關鍵修復：切割邏輯】
            // 如果這塊空地很大，我們不能全給他，要把剩下的切出來當新空地！
            if (current->size > aligned_size + sizeof(block_header_t) + 16) {
                // 計算新地契的位置
                block_header_t* new_block = (block_header_t*)((uint32_t)current + sizeof(block_header_t) + aligned_size);
                new_block->is_free = 1;
                new_block->size = current->size - aligned_size - sizeof(block_header_t);
                new_block->next = current->next;

                current->size = aligned_size;
                current->next = new_block;
            }

            // 把地契標記為使用中，並回傳可用空間的起始位址
            current->is_free = 0;
            return (void*)((uint32_t)current + sizeof(block_header_t));
        }
        current = current->next;
    }

    kprintf("PANIC: Kernel Heap Out of Memory! (Req: %d bytes)\n", size);
    return 0;
}

void kfree(void* ptr) {
    if (ptr == 0) return;

    // 往回退一點，找到這塊地的地契，把它標記為空閒
    block_header_t* block = (block_header_t*)((uint32_t)ptr - sizeof(block_header_t));
    block->is_free = 1;

    // (在簡單版 OS 中，我們暫時省略把相鄰空地合併的邏輯，128KB 夠我們隨便花了)
}
