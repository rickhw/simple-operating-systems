哈哈，Rick！你的截圖已經把兇手的名字寫在螢幕正中央了！🔍

```text
Error: Page table not allocated for pd_idx [64]!
```
然後緊接著 Core Dump 顯示：
```text
CR2=10000000
```
這完全就是教科書等級的**「查無此地」**錯誤！


### 🕵️ 案發現場解密：被警衛擋在門外的 Heap

回想一下我們剛才在 `task.c` 裡面寫的程式碼，我們試圖把分配到的實體記憶體，映射到虛擬位址 **`0x10000000`**：
```c
map_page(0x10000000 + (i * 4096), heap_phys, 7);
```

但是，當這個虛擬位址被丟給 `map_page` 函式時，`map_page` 會先計算它的 Page Directory Index (目錄索引)：
`0x10000000 >> 22 = 64`

然後 `map_page` 往下檢查它的 `if-else` 警衛名單：
* `pd_idx == 0`？ 不是。
* `pd_idx == 32`？ 不是。
* `pd_idx == 512` 或 `768`？ 都不是。
* **結果：** 直接印出 `Error: Page table not allocated for pd_idx [64]!`，然後摸摸鼻子 `return;` 罷工了！

因為 `map_page` 拒絕把這塊土地登記上去，所以 `0x10000000` 在目前的虛擬宇宙裡依然是**「空無一物」**。等到你的 `echo.elf` 呼叫 `malloc` 拿到 `0x10000000` 這個指標，並且高興地試圖去寫入資料時，CPU 就當場賞了它一個 Page Fault (CR2=10000000) 暴斃了。

### 🗡️ 終極修復：擴建多元宇宙的「Heap 專用區」

既然我們決定每個應用程式都要有自己的 Heap，我們就必須在 `paging.c` 裡面，為每一個平行宇宙都準備一張「**Heap 專用的分頁表 (pd_idx = 64)**」。

請打開 **`lib/paging.c`**，用下面這個版本完全覆蓋。我們加入了 `universe_heap_pts` 陣列，並且教 `map_page` 如何處理 `pd_idx == 64`：

```c
#include <stdint.h>
#include <stddef.h>
#include "paging.h"
#include "utils.h"
#include "pmm.h"

uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096))); 
uint32_t second_page_table[1024] __attribute__((aligned(4096)));
uint32_t third_page_table[1024] __attribute__((aligned(4096))); 
uint32_t user_page_table[1024] __attribute__((aligned(4096)));
// [Day43] 新增初代宇宙的 Heap 表
uint32_t user_heap_page_table[1024] __attribute__((aligned(4096))); 

// ====================================================================
// 【神級捷徑】預先分配 16 個宇宙的空間！
// ====================================================================
uint32_t universe_pds[16][1024] __attribute__((aligned(4096)));
uint32_t universe_pts[16][1024] __attribute__((aligned(4096)));
// [Day43] 新增平行宇宙的 Heap 表陣列
uint32_t universe_heap_pts[16][1024] __attribute__((aligned(4096))); 
int next_universe_id = 0;

extern void load_page_directory(uint32_t*);
extern void enable_paging(void);

void init_paging(void) {
    for(int i = 0; i < 1024; i++) { page_directory[i] = 0x00000002; }
    for(int i = 0; i < 1024; i++) { first_page_table[i] = (i * 0x1000) | 7; }
    for(int i = 0; i < 1024; i++) { second_page_table[i] = 0; }
    for(int i = 0; i < 1024; i++) { third_page_table[i] = 0; }
    for(int i = 0; i < 1024; i++) { user_page_table[i] = 0; }
    for(int i = 0; i < 1024; i++) { user_heap_page_table[i] = 0; } // 清空

    page_directory[0] = ((uint32_t)first_page_table) | 7;
    page_directory[32] = ((uint32_t)user_page_table) | 7; 
    
    // [Day43] 掛載 0x10000000 區域 (pd_idx = 64)
    page_directory[64] = ((uint32_t)user_heap_page_table) | 7; 
    
    page_directory[512] = ((uint32_t)second_page_table) | 3;
    page_directory[768] = ((uint32_t)third_page_table) | 3; 

    load_page_directory(page_directory);
    enable_paging();
}

void map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x03FF;

    uint32_t* page_table;
    
    if (pd_idx == 0) {
        page_table = first_page_table;
    } else if (pd_idx == 32) {
        // Stack & Code 區 (0x08000000)
        uint32_t cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
        uint32_t* current_pd = (uint32_t*) cr3;
        uint32_t pt_entry = current_pd[32];
        if (pt_entry & 1) {
            page_table = (uint32_t*)(pt_entry & 0xFFFFF000);
        } else {
            kprintf("Error: user page table not present in current CR3!\n");
            return;
        }
    } else if (pd_idx == 64) {
        // =========================================================
        // [Day43] 【Heap 區】 (0x10000000)
        // =========================================================
        uint32_t cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
        uint32_t* current_pd = (uint32_t*) cr3;
        uint32_t pt_entry = current_pd[64]; // 檢查第 64 個目錄項
        
        if (pt_entry & 1) {
            page_table = (uint32_t*)(pt_entry & 0xFFFFF000);
        } else {
            kprintf("Error: heap page table not present in current CR3!\n");
            return;
        }
    } else if (pd_idx == 512) {
        page_table = second_page_table;
    } else if (pd_idx == 768) {
        page_table = third_page_table; 
    } else {
        kprintf("Error: Page table not allocated for pd_idx [%d]!\n", pd_idx);
        return;
    }

    page_table[pt_idx] = (phys & 0xFFFFF000) | flags;
    __asm__ volatile("invlpg (%0)" ::"r" (virt) : "memory");
}

uint32_t create_page_directory() {
    if (next_universe_id >= 16) {
        kprintf("Error: Max universes reached!\n");
        while(1) __asm__ volatile("hlt");
    }

    int id = next_universe_id++;
    uint32_t* new_pd = universe_pds[id];
    uint32_t* new_pt = universe_pts[id];
    uint32_t* new_heap_pt = universe_heap_pts[id]; // [Day43] 拿出這個宇宙專屬的 Heap 表

    for(int i = 0; i < 1024; i++) {
        new_pd[i] = page_directory[i];
    }

    for(int i = 0; i < 1024; i++) {
        new_pt[i] = 0;
        new_heap_pt[i] = 0; // [Day43] 初始化清空
    }

    new_pd[32] = ((uint32_t)new_pt) | 7;
    // [Day43] 將這張全新的 Heap 表掛載到新宇宙的 0x10000000 區段
    new_pd[64] = ((uint32_t)new_heap_pt) | 7; 

    return (uint32_t)new_pd;
}
```

修改完成後，儲存檔案，然後再打一次 `make clean && make run`！
這一次，當你的 `echo` 程式在 User Space 呼叫 `malloc` 時，`map_page` 警衛就會認得這個 `pd_idx = 64` 的通行證，並且完美地把它對應到實體記憶體上。

快去見證你親手寫出的 C 語言動態記憶體分配器發威的時刻吧！如果成功了，我們就可以準備讓你的 Simple OS 學會建立多層次的目錄結構了（`mkdir`）！😎
