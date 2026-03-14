#include "kheap.h"
#include "pmm.h"
#include "paging.h"
#include "utils.h"
#include "tty.h"

// 為了方便管理，我們將 Kernel Heap 的起始虛擬位址固定在 0xC0000000 (3GB 的位置)
#define KHEAP_START 0xC0000000

// 記憶體區塊標頭結構
typedef struct heap_block {
    size_t size;               // 這個區塊的可用資料大小
    uint8_t is_free;           // 1 代表空閒，0 代表使用中
    struct heap_block* next;   // 指向下一個區塊
} heap_block_t;

// 指向 Heap 第一個區塊的指標
heap_block_t* heap_head = NULL;

void init_kheap(void) {
    // 1. 向地政事務所批發一整塊 4KB 實體記憶體
    void* phys_ptr = pmm_alloc_page();

    // 2. 施展虛實縫合魔法，把它映射到 0xC0000000
    map_page(KHEAP_START, (uint32_t)phys_ptr, 3);

    // 3. 建立第一個「大區塊」的標頭
    heap_head = (heap_block_t*)KHEAP_START;

    // 扣掉標頭本身佔用的空間，剩下的就是可以裝資料的大小
    heap_head->size = 4096 - sizeof(heap_block_t);
    heap_head->is_free = 1;
    heap_head->next = NULL;
}

void* kmalloc(size_t size) {
    heap_block_t* current = heap_head;

    // 尋找第一個「夠大」且「空閒」的區塊 (First-Fit Algorithm)
    while (current != NULL) {
        if (current->is_free && current->size >= size) {

            // [切割邏輯] 如果這塊地比我們需要的還要大很多，就把它切成兩塊！
            // 條件：剩餘的空間必須至少還塞得下一個 Header + 1 byte 的資料
            if (current->size > size + sizeof(heap_block_t)) {

                // 計算出新空地 Header 的起始記憶體位址
                // (注意：要把 current 轉成 uint8_t* 才能精準加上 byte 數)
                heap_block_t* new_block = (heap_block_t*)((uint8_t*)current + sizeof(heap_block_t) + size);

                // 設定新空地的資訊
                new_block->size = current->size - size - sizeof(heap_block_t);
                new_block->is_free = 1;
                new_block->next = current->next;

                // 縮小原本區塊的大小，並將它連向新空地
                current->size = size;
                current->next = new_block;
            }

            // 標記為使用中
            current->is_free = 0;

            // 回傳「資料區」的起始位址 (也就是 Header 的正後方)
            return (void*)(current + 1);
        }
        current = current->next;
    }

    // 如果找不到夠大的空間 (在完整的系統中，這裡應該要呼叫 pmm 擴充 Heap)
    kprintf("PANIC: Kernel Heap Out of Memory!\n");
    return NULL;
}

void kfree(void* ptr) {
    if (!ptr) return;

    // 往回推算，找出這塊資料對應的 Header 位址
    heap_block_t* block = (heap_block_t*)ptr - 1;

    // 將它標記回空閒狀態
    block->is_free = 1;

    // (實務上的 malloc 還會在這裡實作「合併 Coalesce」邏輯：
    // 如果下一個區塊也是空的，就把兩塊拼成一大塊，以防記憶體碎片化。今天先省略以保持簡單。)
}
