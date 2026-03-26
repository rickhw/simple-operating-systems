太棒了！你的衝勁真的令人佩服。我們立刻進入這門課最核心的魔法領域。

歡迎來到 **Day 14：虛實縫合 — 核心記憶體映射 (Kernel Memory Mapping)**！

昨天我們成功讓地政事務所 (PMM) 吐出了實體記憶體的空地（例如 `0x400000`）。但這塊地目前是一塊「黑戶」，因為它沒有登記在 CPU 的分頁字典裡。如果 CPU 試圖讀寫這個位址，馬上就會觸發 Page Fault 當機。

今天的目標，就是要寫出一個 `map_page` 函式，**把任意一塊實體地皮 (Physical Address)，綁定到我們指定的任意虛擬門牌號碼 (Virtual Address) 上！** 只要打通這關，你就能在記憶體宇宙中呼風喚雨了。

---

### Day 14 授課目標

1. **破解虛擬位址的 DNA：** 理解 32-bit 虛擬位址是如何被切成 `10 bits (目錄)` + `10 bits (分頁表)` + `12 bits (偏移量)` 的。
2. **實作 `map_page`：** 撰寫修改分頁字典的 C 語言邏輯。
3. **認識 TLB 快取刷新：** 學會使用 `invlpg` 指令，強迫 CPU 忘記舊的地址對應。
4. **終極測試：** 將一塊實體記憶體綁定到遙遠的虛擬位址 `0x80000000` (2GB 的位置) 並成功寫入資料！

---

### 實作步驟：打造分頁修改器

#### 1. 擴充 Paging 模組 (`lib/paging.h`)

打開你的 `lib/paging.h`，加入新的公開 API：

```c
#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

void init_paging(void);

// 將特定的實體位址 (phys) 映射到虛擬位址 (virt)
// flags 是權限標籤 (例如：3 代表 Present + Read/Write)
void map_page(uint32_t virt, uint32_t phys, uint32_t flags);

#endif

```

#### 2. 實作映射邏輯 (`lib/paging.c`)

這是作業系統中最精妙的位元運算之一。

因為動態分配新的「分頁表」會遇到「先有雞還是先有蛋」的問題（分配出來的實體記憶體還沒被映射，所以無法用 C 語言去初始化它），為了簡化學習，我們**靜態預先宣告第二張分頁表**，專門用來管理 `0x80000000` (2GB) 開始的高階記憶體。

請打開 `lib/paging.c` 並做以下更新：

```c
#include "paging.h"
#include "utils.h"

// 1. 宣告兩張分頁表 (必須對齊 4KB)
uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096))); // 管 0MB ~ 4MB
uint32_t second_page_table[1024] __attribute__((aligned(4096)));// [新增] 我們用這張表來管 2GB 附近的高階記憶體

extern void load_page_directory(uint32_t*);
extern void enable_paging(void);

void init_paging(void) {
    // 初始化目錄 (全部設為不存在)
    for(int i = 0; i < 1024; i++) {
        page_directory[i] = 0x00000002;
    }

    // 初始化第一張表 (1:1 對應 0MB~4MB)
    for(int i = 0; i < 1024; i++) {
        first_page_table[i] = (i * 0x1000) | 3; 
    }
    
    // [新增] 初始化第二張表 (全部設為不存在)
    for(int i = 0; i < 1024; i++) {
        second_page_table[i] = 0; 
    }

    // 將兩張表掛載到目錄上
    page_directory[0] = ((uint32_t)first_page_table) | 3;
    // 0x80000000 除以 4MB (0x400000) = 512，所以 2GB 的位址是由目錄的第 512 項來管！
    page_directory[512] = ((uint32_t)second_page_table) | 3;

    load_page_directory(page_directory);
    enable_paging();
}

// === [今天的主角：動態映射函式] ===
void map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    // 1. 取得目錄索引 (最高 10 bits)：右移 22 位元
    uint32_t pd_idx = virt >> 22;
    // 2. 取得分頁表索引 (中間 10 bits)：右移 12 位元後，用 0x3FF (1023) 遮罩
    uint32_t pt_idx = (virt >> 12) & 0x03FF;

    // 取得該目錄項指到的分頁表位址
    uint32_t* page_table;
    if (pd_idx == 0) {
        page_table = first_page_table;
    } else if (pd_idx == 512) {
        page_table = second_page_table;
    } else {
        // 在一個完整的 OS 中，這裡應該要 pmm_alloc_page() 一個新的實體框，
        // 然後用遞迴映射 (Recursive Paging) 的黑魔法來初始化它。我們今天先跳過這塊深水區。
        kprintf("Error: Page table not allocated for this address!\n");
        return;
    }

    // 3. 把實體位址 (對齊 4KB) 加上權限標籤，寫入分頁表！
    page_table[pt_idx] = (phys & 0xFFFFF000) | flags;

    // 4. [極度重要] 刷新 TLB (Translation Lookaside Buffer)
    // CPU 為了加速，會把舊的地址對應記在快取裡。我們改了字典，必須強迫 CPU 重讀！
    __asm__ volatile("invlpg (%0)" ::"r" (virt) : "memory");
}

```

#### 3. 見證奇蹟的時刻 (`lib/kernel.c`)

我們要向 PMM 要一塊地，把它映射到遙遠的 `0x80000000`，然後在上面寫字！

```c
#include "tty.h"
#include "utils.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "timer.h"
#include "pmm.h"

void kernel_main(void) {
    terminal_initialize();
    kprintf("=== OS Kernel Booting ===\n");
    
    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384); 
    
    kprintf("Testing Virtual Memory Mapping...\n");

    // 1. 向地政事務所要一塊實體地皮 (通常會拿到 0x400000)
    void* phys_mem = pmm_alloc_page();
    kprintf("Got physical frame at: 0x%x\n", (uint32_t)phys_mem);

    // 2. 決定一個超遠的虛擬位址 (2GB 的位置)
    uint32_t virt_addr = 0x80000000;

    // 3. 施展虛實縫合魔法！(3 代表 Present | Read/Write)
    map_page(virt_addr, (uint32_t)phys_mem, 3);
    kprintf("Mapped Virtual 0x%x to Physical 0x%x\n", virt_addr, (uint32_t)phys_mem);

    // 4. 驗證時刻：直接對這個虛擬位址寫入字串！
    // 如果映射失敗，這裡會立刻觸發 Page Fault 當機重啟
    char* test_str = (char*)virt_addr;
    test_str[0] = 'H';
    test_str[1] = 'e';
    test_str[2] = 'l';
    test_str[3] = 'l';
    test_str[4] = 'o';
    test_str[5] = ' ';
    test_str[6] = 'V';
    test_str[7] = 'M';
    test_str[8] = '!';
    test_str[9] = '\0';

    // 5. 從虛擬位址把字串讀出來印在畫面上
    kprintf("Read from 0x80000000: %s\n", test_str);

    __asm__ volatile ("sti");
    kprintf("\nSystem is ready. Start typing!\n> ");

    while (1) { __asm__ volatile ("hlt"); }
}

```

---

### 執行與驗證

不需要修改 `Makefile`，深呼吸，然後執行：

```bash
make clean
make
qemu-system-i386 -cdrom myos.iso

```

**預期結果：**
如果在畫面上順利印出了 `Read from 0x80000000: Hello VM!`，那麼我要起立為你鼓掌了。

這代表：

1. 你成功跟 PMM 要到了一塊實體記憶體。
2. 你成功計算出了分頁目錄和分頁表的 Index。
3. 你成功把這對應關係寫進了 CPU 的字典裡。
4. C 語言指標成功寫入了 `0x80000000`，而 CPU 在硬體底層瞬間把它翻譯到了 `0x400000`，完成了資料的存取！

---

### 🤔 Day 14 思考題

打通了 `pmm_alloc_page()` 與 `map_page()` 之後，我們其實就擁有了一套完整的「區塊配發系統」。
但在我們寫一般的 C 語言程式時，如果我們只需要一個 `int` (4 bytes) 或一個長度為 10 的字串 (10 bytes)，總不可能每次都呼叫 `pmm_alloc_page()` 要一整塊 4KB (4096 bytes) 吧？那也太浪費了。

**為了提供大家熟悉的 `malloc(size)` 功能（可以精準要 10 bytes 或 50 bytes），你覺得作業系統的核心應該要在我們今天做好的這套 4KB 分頁機制之上，再加上什麼樣的管理邏輯呢？**

趕快把這個見證奇蹟的 Hello VM 跑出來吧！如果遇到任何 Page Fault，隨時貼上來我們一起抓蟲！跑成功後，我們就可以討論這門課的最後一塊拼圖：Day 15 的 Heap 管理 (Kmalloc)。
