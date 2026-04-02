/**
 * @file src/user/lib/stdlib.c
 * @brief Main logic and program flow for stdlib.c.
 *
 * This file handles the operations and logic associated with stdlib.c.
 */

#include "stdlib.h"
#include "unistd.h"

// 每個動態分配的區塊前面，都會隱藏這個標頭來記錄大小與狀態
typedef struct header {
    int size;
    int is_free;
    struct header* next;
} header_t;

header_t* head = 0; // 記錄第一個區塊的指標

void* malloc(int size) {
    if (size <= 0) return 0;

    // 1. 先在現有的鏈結串列中尋找有沒有「空閒且夠大」的區塊
    header_t* curr = head;
    while (curr != 0) {
        if (curr->is_free && curr->size >= size) {
            curr->is_free = 0; // 找到了！標記為使用中
            return (void*)(curr + 1); // 回傳標頭「之後」的實際可用空間
        }
        curr = curr->next;
    }

    // 2. 找不到現成空間，只好呼叫 sbrk 向作業系統批發新記憶體！
    int total_size = sizeof(header_t) + size;
    header_t* block = (header_t*)sbrk(total_size);

    if ((int)block == -1) return 0; // OS 記憶體耗盡

    // 3. 設定新區塊的標頭，並把它加進鏈結串列中
    block->size = size;
    block->is_free = 0;
    block->next = head;
    head = block;

    return (void*)(block + 1);
}

void free(void* ptr) {
    if (!ptr) return;
    // 往回退一個標頭的大小，找回這個區塊的管理結構
    header_t* block = (header_t*)ptr - 1;
    block->is_free = 1; // 標記為空閒，下次 malloc 就可以重複利用！
}
