#include "kheap.h"
#include "pmm.h"
#include "paging.h"
#include "utils.h"
#include "tty.h"

block_header_t* first_block = 0;

// --- 公開 API ---

void init_kheap() {
    kprintf("[Heap] Initializing Kernel Heap at 0xC0000000...\n");

    // 從 32 個分頁擴建到 512 個分頁 (512 * 4KB = 2 MB)！
    for (int i = 0; i < 512; i++) {
        // 加上 (uint32_t) 強制轉型，消除編譯警告
        uint32_t phys_addr = (uint32_t) pmm_alloc_page();
        map_page(0xC0000000 + (i * 4096), phys_addr, 3);
    }

    // 在這塊巨大的平原最開頭，插上第一塊地契
    first_block = (block_header_t*) 0xC0000000;
    first_block->size = (512 * 4096) - sizeof(block_header_t); // 總空間扣掉地契本身
    first_block->is_free = 1;
    first_block->next = 0;
}

void* kmalloc(uint32_t size) {
    // 記憶體對齊：為了硬體效能，申請的大小必須是 4 的倍數
    uint32_t aligned_size = (size + 3) & ~3;

    block_header_t* current = first_block;

    while (current != 0) {
        if (current->is_free && current->size >= aligned_size) {
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
