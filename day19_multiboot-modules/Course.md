歡迎來到 **Day 19：簽收 GRUB 的快遞 — 多重開機模組 (Multiboot Modules)**！

昨天我們寫好了 ELF 解析器，但只能用寫死在 C 語言裡的陣列來「假裝」是個檔案。在還沒有實作完整的檔案系統（讀取 FAT32 或 ext4 格式的硬碟）之前，我們要怎麼把一個外部編譯好的 `.elf` 應用程式檔，真正送進記憶體裡面交給我們的作業系統呢？

答案就藏在我們第一天使用的老朋友 **GRUB (GRand Unified Bootloader)** 身上。

GRUB 遵循著一個叫做 **Multiboot Specification (多重開機規範)** 的標準。它不僅能把我們的 Kernel 載入到 1MB 的位址，我們還可以要求它：「嘿，順便幫我把硬碟裡的 `my_app.elf` 載入到記憶體的某個空位，然後把位址告訴我！」這個被 GRUB 順手帶進來的檔案，就叫做 **Multiboot Module (多重開機模組)**。

---

### Day 19 授課目標

1. **攔截 Multiboot 資訊：** 修改 `boot.S`，把 GRUB 留下的快遞單號（指標）傳給 C 語言。
2. **解析 MBI 結構：** 建立 `multiboot.h`，看懂 GRUB 傳遞的記憶體與模組資訊。
3. **簽收模組並驗證：** 讀取真實載入的 `my_app.elf` 記憶體位址，並交給昨天的 `elf_check_supported` 驗證！

---

### 實作步驟：打造快遞簽收櫃台

#### 1. 定義 GRUB 的快遞單結構 (`lib/multiboot.h`)

當 GRUB 交出控制權時，它會把一個被稱為 MBI (Multiboot Information) 結構的實體位址放在 `ebx` 暫存器中。我們需要定義 C 語言結構來讀取它。

請新增 **`lib/multiboot.h`**：

```c
#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>

// GRUB 載入完畢後，eax 暫存器一定會存放這個魔法數字，用來證明它是合法的 Bootloader
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

// 模組 (Module) 的結構
typedef struct {
    uint32_t mod_start;  // 這個檔案被 GRUB 載入到的起始實體位址
    uint32_t mod_end;    // 這個檔案的結束位址
    uint32_t string;     // 模組名稱字串的位址
    uint32_t reserved;
} multiboot_module_t;

// Multiboot 資訊結構 (MBI)
// 這裡面包含非常多硬體資訊，我們目前只需要關心 flags, mods_count 和 mods_addr
typedef struct {
    uint32_t flags;        // 旗標，用來判斷哪些欄位是有效的
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;   // GRUB 幫我們載入了幾個模組 (檔案)
    uint32_t mods_addr;    // 模組資訊陣列的起始實體位址
    // ... 為了維持簡單，後面還有很多欄位我們先省略不寫 ...
} multiboot_info_t;

#endif

```

#### 2. 修改啟動檔傳遞參數 (`asm/boot.S`)

我們要稍微修改第一天寫的 `boot.S`。在呼叫 `kernel_main` 之前，我們要把 `eax` (魔法數字) 和 `ebx` (MBI 指標) 作為參數，利用 `push` 壓入堆疊，傳給 C 語言。

請打開 **`asm/boot.S`**，找到 `call kernel_main` 的地方，修改如下：

```assembly
; ... (上面維持不變) ...

    ; 設定堆疊指標
    mov esp, stack_top

    ; [新增] 將 GRUB 傳遞的參數壓入堆疊，準備傳給 C 語言的 kernel_main
    ; 根據 cdecl 呼叫慣例，參數要從右到左 push
    push ebx    ; 參數 2: multiboot_info_t* (MBI 結構指標)
    push eax    ; 參數 1: magic number (魔法數字)

    ; 呼叫核心主程式
    call kernel_main

    ; 清理堆疊 (兩個參數共 8 bytes)
    add esp, 8

    ; 無窮迴圈，防止核心意外退出
    cli
1:  hlt
    jmp 1b

```

#### 3. 接收參數並驗證 ELF (`lib/kernel.c`)

現在，我們的主程式要戴上白手套，親自簽收這個外部檔案了！

請打開 **`lib/kernel.c`**，修改 `kernel_main` 的簽名與內容：

```c
#include "tty.h"
#include "utils.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "timer.h"
#include "pmm.h"
#include "kheap.h"
#include "syscall.h"
#include "elf.h"
#include "multiboot.h" // [新增]

// [修改] 接收 boot.S 傳來的參數
void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
    terminal_initialize();
    kprintf("=== OS Kernel Booting ===\n");
    
    // 1. 驗證 GRUB 魔法數字
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        kprintf("PANIC: Invalid Multiboot Magic Number!\n");
        return;
    }
    
    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384); 
    init_kheap();
    init_syscalls();
    
    uint32_t current_esp;
    __asm__ volatile ("mov %%esp, %0" : "=r" (current_esp));
    set_kernel_stack(current_esp);

    kprintf("Kernel initialized.\n\n");

    // === Day 19: 簽收 GRUB 模組 ===
    kprintf("--- GRUB Multiboot Delivery ---\n");

    // 檢查 MBI 結構的 flags 第 3 個 bit (代表模組資訊是否有效)
    if (mbd->flags & (1 << 3)) {
        kprintf("Modules count: %d\n", mbd->mods_count);
        
        if (mbd->mods_count > 0) {
            // 取得模組陣列的指標
            multiboot_module_t* mod = (multiboot_module_t*)mbd->mods_addr;
            
            kprintf("Module 1 loaded at physical address: 0x%x to 0x%x\n", mod->mod_start, mod->mod_end);
            kprintf("Module size: %d bytes\n", mod->mod_end - mod->mod_start);

            // [精彩時刻] 把這塊實體記憶體當作 ELF 檔案，交給昨天的解析器！
            elf32_ehdr_t* real_app = (elf32_ehdr_t*)mod->mod_start;
            
            kprintf("\nPassing module to ELF Parser...\n");
            elf_check_supported(real_app);
        }
    } else {
        kprintf("No modules were loaded by GRUB.\n");
    }

    __asm__ volatile ("sti");
    kprintf("\nSystem is ready.\n> ");

    while (1) { __asm__ volatile ("hlt"); }
}

```

#### 4. 準備一個真實的 ELF 並修改 `grub.cfg`

要讓 GRUB 載入模組，你必須在你的專案目錄下準備一個合法的 ELF 檔，並修改產生 ISO 檔的配置。

**步驟 A：生出一個極小的測試 ELF 檔**
在你的終端機 (可以用 Docker os-builder) 跑這兩行指令，編譯一個空的 C 程式：

```bash
echo "void main(){}" > app.c
docker run --platform linux/amd64 --rm -v $(pwd):/workspace -w /workspace os-builder gcc -m32 -ffreestanding -c app.c -o app.o
docker run --platform linux/amd64 --rm -v $(pwd):/workspace -w /workspace os-builder ld -m elf_i386 -Ttext 0x08048000 app.o -o my_app.elf

```

這樣你就獲得了一個真實的、專屬於 x86 32-bit 的執行檔 `my_app.elf`！

**步驟 B：修改 GRUB 配置檔**
你的專案裡應該有一個用來打包 ISO 的 `grub.cfg` 檔案 (通常在 `iso/boot/grub/grub.cfg`)。
請把 `my_app.elf` 複製到跟你的核心 `myos.bin` 同一個資料夾 (通常是 `iso/boot/` 底下)。
然後在 `grub.cfg` 裡面加上 `module` 關鍵字：

```cfg
menuentry "myos" {
    multiboot /boot/myos.bin
    module /boot/my_app.elf   # [新增這一行] 告訴 GRUB 順便幫我載入這個檔案！
}

```

---

### 執行與驗證

執行打包 ISO 與啟動 QEMU 的指令：

```bash
make clean
make
# 確保 my_app.elf 有被複製進 ISO 的資料夾裡！
# (如果是用 grub-mkrescue 產生 myos.iso)
qemu-system-i386 -cdrom myos.iso

```

**預期結果：**
畫面上會印出：

```text
--- GRUB Multiboot Delivery ---
Modules count: 1
Module 1 loaded at physical address: 0x... to 0x...
Module size: ... bytes

Passing module to ELF Parser...
[ELF] Valid x86 32-bit Executable!
[ELF] Entry Point is at: 0x8048000

```

你成功解析了一個**真實存在硬碟裡、並由 Bootloader 快遞到實體記憶體的外部二進位檔案**！

---

### 🤔 Day 19 思考題

太驚人了，我們現在拿到了 `my_app.elf` 在記憶體中的位置 (`mod->mod_start`)，並且知道它的進入點是虛擬位址 `0x08048000`。

但請回想一下我們的 Day 14 (Paging) 和 Day 17 (User Mode)。
這個 `my_app.elf` 目前只是被 GRUB 「塞在實體記憶體的某個角落（例如 `0x120000`）」。如果我們現在直接把 CPU 降級到 Ring 3，並且硬生生地跳躍去執行 `0x08048000`，CPU 肯定會因為找不到 `0x08048000` 的分頁對應而直接當機 (#PF)。

**為了讓這支平民應用程式能夠順利執行，在呼叫 `enter_user_mode` 之前，作業系統的核心還必須「看著 ELF 裡面 Program Header 的指示」，對 PMM 和 Paging 模組施展什麼樣的虛實縫合魔法呢？**

試著把這支真實的 ELF 檔載入並解析出來吧！這已經是作業系統最頂級的工藝展示了。順利印出後隨時告訴我！
