歡迎來到 **Day 15：百川匯海 — 動態記憶體配置 (Heap 與 Kmalloc)**！

解答昨天思考題的時刻到了：如果我們只需要 10 bytes 的字串，卻每次都呼叫 `pmm_alloc_page()` 要一整塊 4KB 的地皮，這就像是為了停一腳踏車而買下整座停車場一樣荒謬。

為了解決這個問題，作業系統必須在我們做好的 PMM（實體分頁）與 Paging（虛擬映射）之上，再疊加一層**「Heap (堆積) 管理器」**。它的工作原理是：先向系統「批發」一整塊 4KB 的記憶體，然後像切蛋糕一樣，根據程式的需要，一小塊一小塊地「零售」出去。這就是 C 語言中大名鼎鼎的 `malloc` 與 `free` 的底層真面目！

---

### Day 15 授課目標

1. **理解 Metadata (詮釋資料)：** 記憶體切塊後，系統怎麼知道哪塊有多大、哪塊是空的？我們需要「標頭 (Header)」。
2. **實作 Linked List (鏈結串列) 分配器：** 寫出核心專用的 `kmalloc` 與 `kfree`。
3. **區塊切割 (Splitting)：** 當拿到的空地太大時，自動把它切成「使用中」與「剩餘空地」兩半。

---

### 實作步驟：打造你的切蛋糕機器

我們將在 `lib/` 新增 `kheap.h` 與 `kheap.c`。

#### 1. 定義區塊標頭 (`lib/kheap.h`)

每一個被切出來的記憶體區塊，前面都必須跟著一個「標頭」，用來記錄這塊地的大小與狀態。

```c
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

```

#### 2. 實作 Heap 管理邏輯 (`lib/kheap.c`)

這裡會用到指標的強型別轉換。我們會從一個 4KB 的大區塊開始，隨著 `kmalloc` 的呼叫，不斷把它切成更小的區塊，並用 Linked List 串起來。

```c
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

```

#### 3. 驗證你的 Malloc (`lib/kernel.c`)

把昨天寫入 `0x80000000` 的測試碼刪掉，我們來正式享受 `kmalloc` 帶來的便利！

```c
#include "tty.h"
#include "utils.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "timer.h"
#include "pmm.h"
#include "kheap.h" // [新增]

void kernel_main(void) {
    terminal_initialize();
    kprintf("=== OS Kernel Booting ===\n");
    
    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384); 
    
    // [新增] 初始化 Kernel Heap
    init_kheap();
    kprintf("Kernel Heap initialized at 0xC0000000.\n");

    // 測試 1：要一個長度為 16 bytes 的字串空間
    char* str1 = (char*)kmalloc(16);
    // 測試 2：再要一個長度為 32 bytes 的字串空間
    char* str2 = (char*)kmalloc(32);
    
    kprintf("Allocated str1 at: 0x%x\n", (uint32_t)str1);
    kprintf("Allocated str2 at: 0x%x\n", (uint32_t)str2);
    
    // 寫入資料測試
    memcpy(str1, "Hello kmalloc!", 15);
    str1[15] = '\0';
    kprintf("Data in str1: %s\n", str1);

    // 釋放記憶體
    kfree(str1);
    kfree(str2);
    kprintf("Memory freed successfully.\n");

    __asm__ volatile ("sti");
    kprintf("\nSystem is ready.\n> ");

    while (1) { __asm__ volatile ("hlt"); }
}

```

---

### 執行與驗證

編譯並啟動 QEMU：

```bash
make clean
make
qemu-system-i386 -cdrom myos.iso

```

**預期結果：**
你會看到 `str1` 的位址大約落在 `0xc000000c` (因為 `0xC0000000` 加上 12 bytes 的 Header)。
而 `str2` 的位址會緊接在 `str1` 的資料區 (16 bytes) 和它自己的 Header (12 bytes) 之後！

從今天起，你在 C 語言寫 OS 就像在寫一般的 App 一樣舒服，隨時可以 `kmalloc` 建立各種動態資料結構（例如無上限的 Task Queue、視窗管理物件等）。

---

### 🤔 Day 15 思考題

這套 `kmalloc` 和 `kfree` 目前是跑在核心最高權限 (Ring 0) 裡。

如果你未來開發了一個在使用者空間 (Ring 3) 執行的 App，這個 App 想要呼叫 `malloc`。**你覺得它能直接呼叫我們寫好的這個 `kmalloc` 函式嗎？如果不行，那麼 User App 該怎麼安全地向 OS 要求更多的記憶體呢？**

這題的答案，將引導我們走向作業系統的最後一道高牆。完成測試後跟我分享你的見解，我們準備跨入下一個章節！
