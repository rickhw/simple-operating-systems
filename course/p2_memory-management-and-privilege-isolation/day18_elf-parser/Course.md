太棒了！這股一鼓作氣的氣勢正是開發作業系統最需要的。

我們已經有能力把 CPU 降級到 User Mode (Ring 3)，也擁有了 Syscall 讓平民可以呼叫核心服務。但目前我們的「使用者程式」是直接寫死 (Hardcoded) 在核心的 C 程式碼裡面的（也就是昨天的 `my_first_user_app` 函式）。

在真正的作業系統中，應用程式（像是 `ls`, `echo` 或你自己寫的 C 程式）是獨立編譯成一個個的**「執行檔 (Executable Files)」**，儲存在硬碟裡。當你點擊兩下或在終端機輸入指令時，作業系統會把這個檔案讀進記憶體，然後執行它。

歡迎來到 **Day 18：解剖二進位 — 認識 ELF 執行檔格式與解析器 (ELF Parser)**！

---

### Day 18 授課目標

1. **破除迷思：** 執行檔裡面不全都是機器碼 (Machine Code)。它包含了大量的「詮釋資料 (Metadata)」，告訴 OS 該怎麼載入它。
2. **認識 ELF 格式：** 學習 Linux/Unix 系統中最標準的執行檔格式 **ELF (Executable and Linkable Format)**。
3. **實作 ELF 解析器：** 撰寫 C 語言結構來讀取 ELF 標頭，找出最重要的兩塊資訊：「它是不是合法的執行檔？」以及「程式的進入點 (Entry Point) 在哪裡？」。

---

### 理論揭秘：作業系統如何看懂 `.exe` 或 `.elf`？

當你用 GCC 編譯好一個程式，產生的檔案內部其實被嚴格劃分成了幾個區塊：

1. **ELF Header (標頭)：** 檔案最開頭的幾十個 bytes。裡面包含了「魔法數字 (Magic Number)」證明自己的身分，以及程式第一行指令的記憶體位址（進入點）。
2. **Program Headers (程式標頭)：** 告訴作業系統：「請把檔案裡第 X 個 byte 到第 Y 個 byte 的資料，搬到記憶體的 `0x08048000` 這個位址，並且設定為可讀可執行（這是你的 `.text` 程式碼）。」
3. **Sections (區段)：** 包含實際的機器碼、全域變數資料 (`.data`) 等等。

今天，我們要先實作 **ELF Header** 的解析。

---

### 實作步驟：打造 ELF 檢驗站

我們將在 `lib/` 新增 `elf.h` 與 `elf.c`。

#### 1. 定義 ELF 標頭資料結構 (`lib/elf.h`)

這是一個標準的 32 位元 ELF 標頭結構。我們必須使用 `__attribute__((packed))` 來確保 C 語言不會為了記憶體對齊而偷偷塞入空白的 bytes，這樣才能與真實檔案完美貼合。

```c
#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#include <stdbool.h>

// ELF 檔案開頭永遠是這四個字元： 0x7F, 'E', 'L', 'F'
// 在 Little-Endian 系統中，這組合成一個 32-bit 的整數如下：
#define ELF_MAGIC 0x464C457F 

// 32 位元 ELF 標頭結構
typedef struct {
    uint32_t magic;      // 魔法數字 (0x7F 'E' 'L' 'F')
    uint8_t  ident[12];  // 架構資訊 (32/64 bit, Endianness 等)
    uint16_t type;       // 檔案類型 (1=Relocatable, 2=Executable)
    uint16_t machine;    // CPU 架構 (3=x86)
    uint32_t version;    // ELF 版本
    uint32_t entry;      // [極度重要] 程式的進入點 (Entry Point) 虛擬位址！
    uint32_t phoff;      // Program Header Table 的偏移量
    uint32_t shoff;      // Section Header Table 的偏移量
    uint32_t flags;      // 架構特定標籤
    uint16_t ehsize;     // 這個 ELF Header 本身的大小
    uint16_t phentsize;  // 每個 Program Header 的大小
    uint16_t phnum;      // Program Header 的數量
    uint16_t shentsize;  // 每個 Section Header 的大小
    uint16_t shnum;      // Section Header 的數量
    uint16_t shstrndx;   // 字串表索引
} __attribute__((packed)) elf32_ehdr_t;

// 宣告我們的解析函式
bool elf_check_supported(elf32_ehdr_t* header);

#endif

```

#### 2. 實作檢驗邏輯 (`lib/elf.c`)

作業系統在載入任何檔案之前，都必須先檢查它是不是「自己人」。如果載入了一個純文字檔並試圖把它當成機器碼執行，系統絕對會大崩潰。

```c
#include "elf.h"
#include "tty.h"

// 檢查這是不是一個我們支援的 x86 32-bit ELF 執行檔
bool elf_check_supported(elf32_ehdr_t* header) {
    if (!header) return false;

    // 1. 檢查魔法數字 (Magic Number)
    if (header->magic != ELF_MAGIC) {
        kprintf("[ELF] Error: Invalid Magic Number. Not an ELF file.\n");
        return false;
    }

    // 2. 檢查是否為 32 位元 (ident[0] == 1 代表 32-bit)
    if (header->ident[0] != 1) {
        kprintf("[ELF] Error: Not a 32-bit ELF file.\n");
        return false;
    }

    // 3. 檢查是否為可執行檔 (Type 2 = Executable)
    if (header->type != 2) {
        kprintf("[ELF] Error: Not an executable file.\n");
        return false;
    }

    // 4. 檢查是否為 x86 架構 (Machine 3 = x86)
    if (header->machine != 3) {
        kprintf("[ELF] Error: Not compiled for x86 CPU.\n");
        return false;
    }

    kprintf("[ELF] Valid x86 32-bit Executable!\n");
    kprintf("[ELF] Entry Point is at: 0x%x\n", header->entry);
    
    return true;
}

```

#### 3. 測試我們的解析器 (`lib/kernel.c`)

因為我們現在還沒有實作檔案系統（無法真的從硬碟讀檔），我們用一個「作弊」的方法來測試：我們直接在 C 語言裡宣告一個 `byte 陣列`，並手動把 ELF 的魔法數字和進入點填進去，假裝這是一份剛從硬碟讀進記憶體的檔案！

請打開 `lib/kernel.c`，做以下修改：

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
#include "elf.h" // [新增]

void kernel_main(void) {
    terminal_initialize();
    kprintf("=== OS Kernel Booting ===\n");
    
    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384); 
    init_kheap();
    init_syscalls();
    
    uint32_t current_esp;
    __asm__ volatile ("mov %%esp, %0" : "=r" (current_esp));
    set_kernel_stack(current_esp);

    kprintf("Kernel subsystems initialized.\n\n");

    // === Day 18: 測試 ELF 解析器 ===
    kprintf("--- ELF Parser Test ---\n");

    // 偽造一份長度為 52 bytes 的假 ELF 檔案 (全部填 0)
    uint8_t fake_file[52] = {0}; 
    
    // 把這塊記憶體強制轉型成 ELF 標頭結構，方便我們修改
    elf32_ehdr_t* fake_elf = (elf32_ehdr_t*)fake_file;

    // 填寫合法的資訊騙過檢查器
    fake_elf->magic = ELF_MAGIC;    // 0x7F 'E' 'L' 'F'
    fake_elf->ident[0] = 1;         // 32-bit
    fake_elf->type = 2;             // Executable
    fake_elf->machine = 3;          // x86
    fake_elf->entry = 0x08048000;   // 假設這是應用程式的進入點 (Linux 的標準起點)

    // 把假檔案丟給解析器檢查！
    elf_check_supported(fake_elf);
    
    // 試試看故意搞破壞
    kprintf("\nNow corrupting the file...\n");
    fake_elf->machine = 8; // 故意改成 8 (MIPS 架構)
    elf_check_supported(fake_elf);

    __asm__ volatile ("sti");
    kprintf("\nSystem is ready.\n> ");

    while (1) { __asm__ volatile ("hlt"); }
}

```

---

### 執行與驗證

直接編譯並啟動 QEMU：

```bash
make clean
make
qemu-system-i386 -cdrom myos.iso

```

**預期結果：**
你會在畫面上看到我們寫好的檢驗站成功攔截與解析的結果：

```text
--- ELF Parser Test ---
[ELF] Valid x86 32-bit Executable!
[ELF] Entry Point is at: 0x8048000

Now corrupting the file...
[ELF] Error: Not compiled for x86 CPU.

```

這就代表你的 OS 已經擁有了「辨識」二進位執行檔的能力！當我們把合法的位元組交給解析器，它馬上就能準確地萃取出最重要的資訊：**「如果我要執行這支程式，我必須跳躍到 `0x08048000` 這個位址！」**

---

### 🤔 Day 18 思考題

太棒了，我們現在知道怎麼看懂 ELF 的大門了。

但正如前面提到的，我們今天是用一個「偽造的 C 陣列」來假裝檔案。在真正的系統中，沒有檔案系統的話，我們要怎麼把外部編譯好的 `my_app.elf` 送進記憶體裡交給 OS 呢？

其實，我們的老朋友 **GRUB (開機載入程式)** 除了能載入 Kernel，它還提供了一個叫做 **Multiboot Modules (多重開機模組)** 的超強功能。我們可以要求 GRUB：「嘿！開機的時候順便幫我把 `my_app.elf` 這個檔案整包塞進實體記憶體裡的某個角落，然後把位址告訴我！」
這種將檔案直接載入記憶體的技術，常常被稱為 **Initrd (Initial RAM Disk)** 的雛形。

你把這段 ELF 檢驗邏輯跑出來之後，會想試試看在 Day 19 實作如何**「接收 GRUB 快遞來的 Multiboot 模組，並把真實的外部檔案解析出來」**嗎？
