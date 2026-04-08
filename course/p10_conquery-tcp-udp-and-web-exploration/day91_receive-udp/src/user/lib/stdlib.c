#include "stdlib.h"
#include "unistd.h"

/**
 * @file stdlib.c
 * @brief 使用者空間堆積 (Heap) 管理實作
 * @details 採用簡單的鏈結串列 (Linked List) 管理空閒區塊 (Free List)。
 */

/** @brief 堆積區塊標頭 */
typedef struct header {
    int size;               /**< 該區塊的實際可用大小 */
    int is_free;            /**< 標記是否為空閒 */
    struct header* next;    /**< 下一個區塊指標 */
} header_t;

static header_t* head = 0; /**< 記錄第一個區塊的指標 */

void* malloc(int size) {
    if (size <= 0) return 0;

    // 1. 先在現有的鏈結串列中尋找有沒有「空閒且夠大」的區塊
    header_t* curr = head;
    while (curr != 0) {
        if (curr->is_free && curr->size >= size) {
            curr->is_free = 0; // 標記為使用中
            return (void*)(curr + 1); // 回傳標頭「之後」的實際可用空間
        }
        curr = curr->next;
    }

    // 2. 找不到現成空間，呼叫 sbrk 向核心要求新記憶體
    int total_size = sizeof(header_t) + size;
    header_t* block = (header_t*)sbrk(total_size);

    if ((int)block == -1) return 0; // 記憶體耗盡

    // 3. 設定新區塊標頭，並加入鏈結串列
    block->size = size;
    block->is_free = 0;
    block->next = head;
    head = block;

    return (void*)(block + 1);
}

void free(void* ptr) {
    if (!ptr) return;
    
    // 往回退一個標頭大小，找回管理結構
    header_t* block = (header_t*)ptr - 1;
    block->is_free = 1; // 標記為空閒
}
