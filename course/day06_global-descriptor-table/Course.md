這種「先讓系統跑起來，遇到問題再去翻底層邏輯」的策略，絕對是應付龐雜架構最有效率的解法。我們先建立體感，看到畫面產出，那些燒腦的理論之後自然會迎刃而解！

我們立刻進入 **Day 6：重塑記憶體秩序 — GDT (Global Descriptor Table)**。

### 為什麼我們需要 GDT？（極簡版）

雖然 GRUB 開機時已經幫我們設定了一個堪用的 GDT，但作為作業系統的真正主人，我們不能依賴 Bootloader 的施捨。GDT 是 x86 架構用來定義「記憶體區塊 (Segments)」的清單，它告訴 CPU 兩件事：

1. **區塊在哪裡、有多大**（我們全設為 0 到 4GB，這稱為 Flat Memory Model）。
2. **權限級別**（這是 Kernel 專屬的 Ring 0，還是未來應用程式的 Ring 3）。

如果沒有設定好屬於我們自己的 GDT，明天我們就無法設定中斷 (Interrupts)，鍵盤也就永遠打不通。

---

### 實作步驟：模組化你的核心

從今天開始，我們的程式碼會越來越多，全都塞在 `kernel.c` 會變得難以維護。我們要新增三個檔案，把 GDT 獨立出來。

#### 1. 定義 GDT 資料結構 (`gdt.h`)

請建立 `gdt.h`。這裡面充滿了 x86 歷史遺留下來的「碎塊」，為了相容古老的 16 位元處理器，一個 32 位元的位址被硬生生切成了三段。我們用 `__attribute__((packed))` 嚴格告訴編譯器：**不准為了效能自動對齊記憶體，必須一個 byte 都不差地塞緊**。

```c
// gdt.h
#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// GDT 單一條目的結構
struct gdt_entry_struct {
    uint16_t limit_low;   // 區塊長度 (下半部)
    uint16_t base_low;    // 區塊起始位址 (下半部)
    uint8_t  base_middle; // 區塊起始位址 (中間)
    uint8_t  access;      // 存取權限 (Ring 0/3, 程式碼/資料)
    uint8_t  granularity; // 顆粒度與長度 (上半部)
    uint8_t  base_high;   // 區塊起始位址 (上半部)
} __attribute__((packed));
typedef struct gdt_entry_struct gdt_entry_t;

// 指向 GDT 陣列的指標結構 (給 lgdt 指令用的)
struct gdt_ptr_struct {
    uint16_t limit;       // GDT 陣列的總長度 - 1
    uint32_t base;        // GDT 陣列的起始位址
} __attribute__((packed));
typedef struct gdt_ptr_struct gdt_ptr_t;

// 初始化 GDT 的公開函式
void init_gdt(void);

#endif

```

#### 2. 實作 GDT 邏輯 (`gdt.c`)

請建立 `gdt.c`。我們需要三個區塊：

1. **Null Descriptor:** 必須全為 0（x86 的硬性規定，用來捕捉指標指到 0 的錯誤）。
2. **Kernel Code Segment:** 存放核心程式碼的區塊。
3. **Kernel Data Segment:** 存放核心資料與堆疊的區塊。

```c
// gdt.c
#include "gdt.h"

// 宣告一個長度為 3 的 GDT 陣列
gdt_entry_t gdt_entries[3];
gdt_ptr_t   gdt_ptr;

// 外部組合語言函式，用來載入 GDT
extern void gdt_flush(uint32_t);

// 設定單一 GDT 條目的輔助函式
static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;

    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access      = access;
}

void init_gdt(void) {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 3) - 1;
    gdt_ptr.base  = (uint32_t)&gdt_entries;

    // 第一個必須是 Null
    gdt_set_gate(0, 0, 0, 0, 0);                
    
    // 第二個是 Code Segment (位址 0~4GB)
    // Access 0x9A: Ring 0, 可執行, 可讀取
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); 
    
    // 第三個是 Data Segment (位址 0~4GB)
    // Access 0x92: Ring 0, 可讀寫
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); 

    // 呼叫組合語言，正式套用新的 GDT
    gdt_flush((uint32_t)&gdt_ptr);
}

```

#### 3. 組合語言刷新暫存器 (`gdt_flush.S`)

C 語言沒辦法直接修改 CPU 的段暫存器（Segment Registers），我們必須寫一小段 Assembly 來代勞。請建立 `gdt_flush.S`：

```assembly
; gdt_flush.S
global gdt_flush

gdt_flush:
    ; 從 C 語言的堆疊中取出 gdt_ptr 的位址
    mov eax, [esp+4]
    
    ; 載入新的 GDT 到 CPU
    lgdt [eax]

    ; 載入 Data Segment 的偏移量 (0x10 是 GDT 的第 3 個元素：Data Segment)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; 載入 Code Segment 的偏移量 (0x08 是 GDT 的第 2 個元素：Code Segment)
    ; 我們必須使用 "Far Jump (遠跳躍)" 來強制 CPU 刷新執行管線並套用新的 cs 暫存器
    jmp 0x08:.flush

.flush:
    ret

```

#### 4. 更新主程式 (`kernel.c`)

打開你的 `kernel.c`，把 GDT 初始化函式加進去：

```c
// 在最上方引入標頭檔
#include "gdt.h"

// ... 略過之前的 kprintf 實作 ...

void kernel_main(void) {
    terminal_initialize();
    
    kprintf("=== OS Kernel Booting ===\n");
    
    // 初始化 GDT
    init_gdt();
    kprintf("GDT loaded successfully.\n");
    
    kprintf("Initialization complete. System is stable.\n");
}

```

#### 5. 更新編譯腳本 (`Makefile`)

因為我們多了 `.c` 和 `.S` 檔案，需要把它們加進連結清單裡。請修改 `myos.bin` 的依賴與編譯規則：

```makefile
# ... 略過前面 ...

# 將 gdt_flush.o 與 gdt.o 加入編譯鏈
myos.bin: boot.o kernel.o gdt.o gdt_flush.o
	$(DOCKER_CMD) ld -m elf_i386 -n -T linker.ld -o myos.bin boot.o kernel.o gdt.o gdt_flush.o

# 編譯 C 檔案的通用規則
%.o: %.c
	$(DOCKER_CMD) gcc $(CFLAGS) -c $< -o $@

# 編譯 Assembly 檔案的通用規則
%.o: %.S
	$(DOCKER_CMD) nasm -f elf32 $< -o $@

# ... 略過後面 clean ...

```

---

### 執行與驗證

執行 `make clean && make` 然後跑起 QEMU。

**預期結果：**
畫面上會順利印出 `GDT loaded successfully.`。
這看起來好像什麼都沒改變，但實際上你的作業系統剛剛完成了一次「心臟手術」！如果 GDT 結構錯了哪怕一個 bit，或者 `gdt_flush` 裡面的偏移量寫錯，QEMU 就會立刻陷入無限重啟的死迴圈（Triple Fault）。只要能看到這行字，代表你的記憶體新秩序已經完美建立！

---

### 🤔 Day 6 思考題

體驗完這段硬核的設定後，請思考一個關於「權限」的問題：

在 `gdt.c` 中，我們設定了 Kernel Code 的 Access Byte 是 `0x9A`，這代表著它運行在最高特權的 Ring 0。如果未來我們實作了一個給一般使用者跑的應用程式（例如 `ls` 指令），我們會幫它在 GDT 建立一組屬於 Ring 3 的區塊。
**當 Ring 3 的應用程式想要在畫面上印出字串時，它不能直接寫入 `0xB8000`（會被 CPU 擋下），它必須呼叫 Ring 0 的 Kernel 來幫忙。你覺得 CPU 要怎麼安全的讓程式從 Ring 3 臨時「跨界」到 Ring 0，執行完再回去呢？**

順利編譯通過後，跟我說一聲，我們就可以朝作業系統的神經系統：**IDT (中斷描述符表)** 邁進了！
