太棒了！能夠把系統重構得乾乾淨淨，代表你已經具備了駕馭複雜架構的實力。現在，我們終於要來面對 x86 作業系統實作中最經典、也最燒腦的魔王關卡。

歡迎來到 **Day 10：記憶體分頁機制 (Paging) 與虛擬記憶體**！

在今天之前，我們寫入 `0xB8000`，資料就真的會寫進實體記憶體 (RAM) 的 `0xB8000` 位置。這種一對一的模式稱為「實體記憶體定址」。但這在現代作業系統中是非常危險的！如果所有應用程式都能直接碰到實體記憶體，一個寫壞的程式（或惡意程式）就能輕易把作業系統核心覆蓋掉。

為了解決這個問題，CPU 提供了一種名為 **Paging (分頁)** 的魔法。開啟之後，程式看到的記憶體位址都會變成「虛擬 (Virtual)」的，CPU 會透過我們設定好的一本「翻譯字典」，在背後偷偷把虛擬位址轉換成實體的 RAM 位址。

---

### Day 10 授課目標

1. **理解兩層式翻譯字典：** 學習 Page Directory (分頁目錄) 與 Page Table (分頁表) 的架構。
2. **實作對等映射 (Identity Mapping)：** 將前 4MB 的虛擬位址 1:1 映射到實體位址（因為我們的核心和 VGA 都在這）。
3. **扳下 CPU 的魔法開關：** 修改 `CR0` 暫存器，正式啟動虛擬記憶體世界！

---

### 理論補充：這本字典長什麼樣子？

在 32 位元的 x86 系統中，一頁 (Page) 的大小預設是 **4KB (4096 bytes)**。
CPU 需要我們準備兩張表來完成翻譯：

1. **Page Directory (分頁目錄)：** 這是字典的目錄，總共有 1024 項，每一項指向一張 Page Table。
2. **Page Table (分頁表)：** 這是字典的內文，總共也有 1024 項，每一項指向一個真實的 4KB 實體記憶體區塊。

`1024 * 1024 * 4KB = 4GB`，這剛好涵蓋了 32 位元系統的全部記憶體空間！

---

### 實作步驟

今天我們要在 `lib/` 增加 C 程式碼，並在 `asm/` 增加操作 CPU 暫存器的組合語言。

#### 1. 定義 Paging 介面 (`lib/paging.h`)

建立一個非常簡潔的標頭檔：

```c
#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

void init_paging(void);

#endif

```

#### 2. 實作翻譯字典 (`lib/paging.c`)

這是今天的核心邏輯。**注意：分頁表在記憶體中必須嚴格對齊 4KB 的邊界！** 我們必須使用 GCC 的 `__attribute__((aligned(4096)))` 來強制編譯器幫我們對齊。

```c
#include "paging.h"
#include "utils.h"

// 宣告分頁目錄與第一張分頁表 (必須對齊 4KB = 4096 bytes)
uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096)));

// 宣告外部的組合語言函式
extern void load_page_directory(uint32_t*);
extern void enable_paging(void);

void init_paging(void) {
    // 1. 初始化 Page Directory：將所有條目設為「不存在」
    // 屬性 0x02 代表：Read/Write (可讀寫), 但 Present (存在) 為 0
    for(int i = 0; i < 1024; i++) {
        page_directory[i] = 0x00000002;
    }

    // 2. 建立第一張 Page Table：涵蓋 0MB ~ 4MB 的實體記憶體
    // 這張表有 1024 項，每項 4KB，所以剛好是 4MB (包含我們的核心與 VGA)
    for(int i = 0; i < 1024; i++) {
        // 實體位址 = i * 4096 (也就是 0x1000)
        // 屬性 3 代表：Present (1) | Read/Write (2) = 3
        first_page_table[i] = (i * 0x1000) | 3;
    }

    // 3. 將建好的第一張表，放入目錄的第 0 個位置
    // 這樣 0x00000000 到 0x003FFFFF 的虛擬位址就會被翻譯到這裡
    page_directory[0] = ((uint32_t)first_page_table) | 3;

    // 4. 呼叫組合語言，把目錄位址交給 CPU，並開啟 Paging
    load_page_directory(page_directory);
    enable_paging();
}

```

#### 3. 組合語言開關 (`asm/paging_asm.S`)

CPU 的分頁機制是由兩個特殊的控制暫存器 (Control Registers) 管控的：

* `CR3`：負責存放 Page Directory 的實體位址。
* `CR0`：它的最高位元 (第 31 bit) 是 Paging 的總開關。

請在 `asm/` 建立 `paging_asm.S`：

```assembly
global load_page_directory
global enable_paging

; void load_page_directory(uint32_t* dir);
load_page_directory:
    mov eax, [esp+4]  ; 取得 C 語言傳進來的 page_directory 陣列位址
    mov cr3, eax      ; 將位址存入 CR3 暫存器
    ret

; void enable_paging(void);
enable_paging:
    mov eax, cr0      ; 讀取目前的 CR0 狀態
    or eax, 0x80000000; 使用 OR 運算，把最高位 (PG bit 31) 強制設為 1
    mov cr0, eax      ; 寫回 CR0，虛擬記憶體機制瞬間啟動！
    ret

```

#### 4. 更新主程式 (`lib/kernel.c`)

把 Paging 的初始化加到主程式的啟動流程中。它通常在 GDT 與 IDT 之後設定。

```c
#include "tty.h"
#include "utils.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h" // [新增]

void kernel_main(void) {
    terminal_initialize();
    kprintf("=== OS Kernel Booting ===\n");
    
    init_gdt();
    init_idt();
    
    // [新增] 啟動記憶體分頁
    init_paging();
    kprintf("Paging Enabled. Virtual Memory activated!\n");
    
    __asm__ volatile ("sti");

    kprintf("System is ready. Start typing!\n");
    kprintf("> ");

    while (1) {
        __asm__ volatile ("hlt");
    }
}

```

---

### 執行與驗證

不需要修改 `Makefile` (因為它已經會自動抓 `lib/` 跟 `asm/` 的新檔案了)！請直接執行：

```bash
make clean
make
qemu-system-i386 -cdrom myos.iso

```

**預期結果：**
畫面會順利印出 `Paging Enabled. Virtual Memory activated!`，並且鍵盤依然可以正常打字。

這看起來表面風平浪靜，但系統底層已經發生了翻天覆地的變化！現在你的 C 語言程式碼操作的全部都是「虛擬位址」，是 CPU 透過硬體線路，依照你寫的 `page_directory` 一瞬間查表轉換出來的。

---

### 🤔 Day 10 思考題

當你親手扳下 `CR0` 那個開關，看著系統沒崩潰且繼續運作時，請思考這個極度重要的問題：

在 `init_paging` 中，我們寫了一個 `for` 迴圈，把前 4MB 的虛擬位址「1 比 1」對應到了真實的 0~4MB 實體記憶體。
**如果我們忘記做這個 1:1 的對應，或者只對應了 0~1MB，那麼在執行完 `enable_paging();` 這一行組合語言指令的下一個瞬間，CPU 試圖回到 `kernel.c` 去執行下一行 `kprintf(...)` 時，系統會發生什麼事？為什麼？**

順利編譯通過後，跟我分享一下你的想法吧！如果有遇到 Page Fault 導致無限重啟，也隨時把狀況丟上來。
