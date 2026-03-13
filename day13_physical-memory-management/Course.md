歡迎來到 **Day 13：圈地為王 — 實體記憶體管理 (Physical Memory Management, PMM)**！

在前幾天，我們成功開啟了虛擬記憶體 (Paging)，並且用 1:1 對等映射（Identity Mapping）把前 4MB 的空間隨便分配給了系統。但這就像是一個沒有戶政系統的荒島，大家隨便找空地蓋房子，總有一天會發生搶地盤的衝突。

如果未來我們要建立新的 Task（例如啟動一個使用者程式），我們必須給它配發「乾淨、沒人使用的實體記憶體 (RAM)」來作為它的堆疊和程式碼空間。

因此，今天的目標非常明確：**打造一個地政事務所，精準掌握系統中每一塊 4KB 記憶體的使用狀態！**

---

### Day 13 授課目標

1. **理解頁框 (Page Frame)：** 將實體 RAM 切割成無數個 4KB 的標準單位。
2. **實作 Bitmap (位元圖)：** 使用最節省記憶體的方式，記錄哪一塊地被佔用（`1`），哪一塊地是空的（`0`）。
3. **打造分配與釋放機制：** 實作 `pmm_alloc_page()` 與 `pmm_free_page()`，讓核心可以動態索取記憶體。

---

### 理論補充：為什麼用 Bitmap？

假設我們的電腦有 4GB 的 RAM。
如果每一個 4KB 的區塊（Frame）算作一塊地，總共有 `4GB / 4KB = 1,048,576` 塊地。

如果我們用一個整數陣列 `int frames[1048576]` 來記錄，光是這個陣列本身就要消耗 4MB 的記憶體！這太浪費了。
但如果我們用 **Bit (位元)** 來記錄呢？
一個位元只能是 `0` (Free) 或 `1` (Used)。
`1,048,576 bits / 8 = 131,072 bytes`。
我們只需要區區 **128 KB** 的空間，就能精準監控整台 4GB 電腦的每一寸實體記憶體！這就是底層系統工程師的極致浪漫。

---

### 實作步驟：建立地政事務所

我們將在 `lib/` 建立新的實體記憶體管理器模組。

#### 1. 宣告 PMM 介面 (`lib/pmm.h`)

```c
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

```

#### 2. 實作 Bitmap 邏輯 (`lib/pmm.c`)

這段程式碼充滿了 C 語言的「位元運算 (Bitwise Operations)」，這是系統底層最常見的操作。

```c
#include "pmm.h"
#include "utils.h"
#include "tty.h"

// 假設我們最多支援 4GB RAM (總共 1,048,576 個 Frames)
// 1048576 bits / 32 bits (一個 uint32_t) = 32768 個陣列元素
#define BITMAP_SIZE 32768
uint32_t memory_bitmap[BITMAP_SIZE];

uint32_t max_frames = 0; // 系統實際擁有的可用 Frame 數量

// --- 內部輔助函式：操作特定的 Bit ---

// 將第 frame_idx 個 bit 設為 1 (代表已使用)
static void bitmap_set(uint32_t frame_idx) {
    uint32_t array_idx = frame_idx / 32;
    uint32_t bit_offset = frame_idx % 32;
    memory_bitmap[array_idx] |= (1 << bit_offset);
}

// 將第 frame_idx 個 bit 設為 0 (代表釋放)
static void bitmap_clear(uint32_t frame_idx) {
    uint32_t array_idx = frame_idx / 32;
    uint32_t bit_offset = frame_idx % 32;
    memory_bitmap[array_idx] &= ~(1 << bit_offset);
}

// 檢查第 frame_idx 個 bit 是否為 1
static bool bitmap_test(uint32_t frame_idx) {
    uint32_t array_idx = frame_idx / 32;
    uint32_t bit_offset = frame_idx % 32;
    return (memory_bitmap[array_idx] & (1 << bit_offset)) != 0;
}

// 尋找第一個為 0 的 bit (第一塊空地)
static int bitmap_find_first_free() {
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        // 如果這個整數不是 0xFFFFFFFF (代表裡面至少有一個 bit 是 0)
        if (memory_bitmap[i] != 0xFFFFFFFF) {
            for (int j = 0; j < 32; j++) {
                int frame_idx = i * 32 + j;
                if (frame_idx >= max_frames) return -1; // 已經超出實際記憶體容量
                
                if (!bitmap_test(frame_idx)) {
                    return frame_idx;
                }
            }
        }
    }
    return -1; // 記憶體全滿 (Out of Memory)
}

// --- 公開 API ---

void init_pmm(uint32_t mem_size_kb) {
    // 總 KB 數 / 4 = 總共有多少個 4KB 的 Frames
    max_frames = mem_size_kb / 4;
    
    // 一開始先把所有的記憶體都標記為「已使用」(填滿 1)
    // 這是為了安全，避免我們不小心分配到不存在的硬體空間
    memset(memory_bitmap, 0xFF, sizeof(memory_bitmap));

    // 然後，我們只把「真實存在」的記憶體標記為「可用」(設為 0)
    for (uint32_t i = 0; i < max_frames; i++) {
        bitmap_clear(i);
    }

    // [極度重要] 保留系統前 4MB (0 ~ 1024 個 Frames) 不被分配！
    // 因為這 4MB 已經被我們的 Kernel 程式碼、VGA 記憶體、IDT/GDT 給佔用了
    for (uint32_t i = 0; i < 1024; i++) {
        bitmap_set(i);
    }
}

void* pmm_alloc_page(void) {
    int free_frame = bitmap_find_first_free();
    if (free_frame == -1) {
        kprintf("PANIC: Out of Physical Memory!\n");
        return NULL; // OOM
    }

    // 標記為已使用
    bitmap_set(free_frame);
    
    // 計算出實際的物理位址 (Frame 索引 * 4096)
    uint32_t phys_addr = free_frame * PMM_FRAME_SIZE;
    return (void*)phys_addr;
}

void pmm_free_page(void* ptr) {
    uint32_t phys_addr = (uint32_t)ptr;
    uint32_t frame_idx = phys_addr / PMM_FRAME_SIZE;
    
    bitmap_clear(frame_idx);
}

```

#### 3. 在主程式進行驗證 (`lib/kernel.c`)

讓我們把地政事務所開張，並試著向它要兩塊地看看。

```c
#include "tty.h"
#include "utils.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "timer.h"
#include "pmm.h" // [新增]

void kernel_main(void) {
    terminal_initialize();
    kprintf("=== OS Kernel Booting ===\n");
    
    init_gdt();
    init_idt();
    init_paging();
    
    // 假設 GRUB 告訴我們系統有 16MB 的 RAM (16 * 1024 KB)
    // 實務上 GRUB 會透過 Multitool header 傳遞真實記憶體大小，這裡我們先 Hardcode
    init_pmm(16384); 
    kprintf("Physical Memory Manager initialized.\n");

    // 測試：向系統索取兩塊 4KB 的記憶體
    void* page1 = pmm_alloc_page();
    void* page2 = pmm_alloc_page();
    
    // 印出它們的實體位址
    kprintf("Allocated Page 1 at: 0x%x\n", (uint32_t)page1);
    kprintf("Allocated Page 2 at: 0x%x\n", (uint32_t)page2);
    
    // 釋放第一塊，然後再索取一塊，看看會不會拿到剛剛釋放的！
    pmm_free_page(page1);
    kprintf("Freed Page 1.\n");
    
    void* page3 = pmm_alloc_page();
    kprintf("Allocated Page 3 at: 0x%x\n", (uint32_t)page3);

    // 我們先關閉 Timer 以免打擾畫面觀看
    // init_timer(100); 
    
    __asm__ volatile ("sti");
    kprintf("System is ready.\n> ");

    while (1) { __asm__ volatile ("hlt"); }
}

```

---

### 執行與驗證

不需要修改 `Makefile`，直接執行：

```bash
make clean
make
qemu-system-i386 -cdrom myos.iso

```

**預期結果：**
畫面會印出三行記憶體位址的配發結果。
你會發現：

1. `Page 1` 的位址應該會是 `0x400000` (也就是 4MB 整的位置！因為前 4MB 被我們設定為保留區了)。
2. `Page 2` 的位址會緊接在後面：`0x401000` (加了 4096 bytes)。
3. 當你釋放 `Page 1` 後，再次要一塊地 `Page 3`，地政事務所會聰明地把剛剛空出來的 `0x400000` 再次配發給你！

---

### 🤔 Day 13 思考題

太棒了，現在我們有能力動態取得實體的 4KB 記憶體空間了（這相當於 C 語言標準函式庫裡面 `malloc` 最底層的骨架）！

但請注意，我們現在配發出來的位址（例如 `0x400000`）是**「實體位址 (Physical Address)」**。
在 Day 10 我們啟動了分頁機制 (Paging)，CPU 執行時只認得「虛擬位址 (Virtual Address)」。如果我們現在直接把資料寫入 `0x400000`，這剛好是因為我們前面做了 1:1 的對等映射所以沒出事。

**如果未來我們分配了一塊實體記憶體 `0x500000`，但我們想要讓「使用者程式 A」以為它是在虛擬位址 `0x8000000` 執行，我們該如何把這塊剛剛要到的實體地皮，登記到使用者程式 A 專屬的那本「分頁字典 (Page Table)」裡呢？**

這個把「虛擬位址」與「實體位址」縫合的過程，就是虛擬記憶體管理的終極奧義。跑出結果後，跟我分享你的想法吧！
