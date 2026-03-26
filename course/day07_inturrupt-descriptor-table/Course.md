歡迎來到 **Day 7：賦予系統知覺 — IDT (中斷描述符表)**！

前幾天我們一直處於「主動」狀態：主動寫入 VGA 記憶體、主動算數學、主動設定 GDT。但一個真正的作業系統必須具備「被動」反應的能力。當鍵盤被按下、當時鐘滴答作響、或者當程式發生「除以零」的嚴重錯誤時，CPU 必須立刻放下手邊的工作，轉而去處理這些突發事件。

這套機制的核心，就是 **IDT (Interrupt Descriptor Table)**。你可以把它想像成作業系統的「神經系統」與「緊急聯絡簿」。

---

### Day 7 授課目標

1. **理解中斷機制：** 認識例外 (Exceptions)、硬體中斷 (IRQs) 與軟體中斷 (Software Interrupts)。
2. **建立 IDT 結構：** 設定 256 個中斷向量 (Interrupt Vectors)。
3. **實作第一個 ISR (Interrupt Service Routine)：** 捕捉「除以零 (Divide by Zero)」錯誤，防止系統無聲無息地崩潰。

---

### 實作步驟：打造緊急聯絡簿

在 x86 架構中，CPU 支援 256 種中斷（編號 0~255）。前 32 個 (0~31) 是 CPU 保留給**內部例外錯誤**使用的（例如：0 是除以零，14 是 Page Fault 記憶體分頁錯誤）。

#### 1. 定義 IDT 結構 (`idt.h`)

請建立 `idt.h`。與 GDT 類似，IDT 的每一個條目 (Entry) 也是 8 bytes，用來記錄「當某個中斷發生時，該跳去執行哪一段記憶體位址的程式碼」。

```c
// idt.h
#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// IDT 單一條目的結構
struct idt_entry_struct {
    uint16_t base_lo; // 中斷處理程式 (ISR) 位址的下半部
    uint16_t sel;     // GDT 裡的 Kernel Code Segment 選擇子 (通常是 0x08)
    uint8_t  always0; // 必須為 0
    uint8_t  flags;   // 屬性與權限標籤
    uint16_t base_hi; // 中斷處理程式 (ISR) 位址的上半部
} __attribute__((packed));
typedef struct idt_entry_struct idt_entry_t;

// 指向 IDT 陣列的指標結構 (給 lidt 指令用的)
struct idt_ptr_struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));
typedef struct idt_ptr_struct idt_ptr_t;

void init_idt(void);

#endif

```

#### 2. 實作 IDT 邏輯 (`idt.c`)

請建立 `idt.c`。我們要初始化這 256 個條目，並把「第 0 號中斷 (除以零)」綁定到我們專屬的處理函式。

```c
// idt.c
#include "idt.h"

// 由於我們的 kprintf 寫在 kernel.c 裡，這裡先宣告外部參照
extern void kprintf(const char* format, ...);

// 宣告一個長度為 256 的 IDT 陣列
idt_entry_t idt_entries[256];
idt_ptr_t   idt_ptr;

// 外部組合語言函式：載入 IDT 與中斷處理入口
extern void idt_flush(uint32_t);
extern void isr0(); // 第 0 號中斷的 Assembly 進入點

// 設定單一 IDT 條目的輔助函式
static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].base_lo = base & 0xFFFF;
    idt_entries[num].base_hi = (base >> 16) & 0xFFFF;
    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    // flags 0x8E 代表：這是一個 32-bit 的中斷閘 (Interrupt Gate)，運行在 Ring 0，且此條目有效(Present)
    idt_entries[num].flags   = flags;
}

// 這是當「除以零」發生時，實際會執行的 C 語言函式
void isr0_handler(void) {
    kprintf("\n[KERNEL PANIC] Exception 0: Divide by Zero!\n");
    kprintf("System Halted.\n");
    // 發生嚴重錯誤，直接把系統凍結
    __asm__ volatile ("cli; hlt");
}

void init_idt(void) {
    idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
    idt_ptr.base  = (uint32_t)&idt_entries;

    // 先把 256 個中斷全部清空 (避免指到未知的記憶體)
    // 這裡我們簡單用迴圈清零 (你也可以 include 昨天寫的 memset)
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    // 掛載第 0 號中斷：除以零
    // 0x08 是我們昨天在 GDT 設定的 Kernel Code Segment
    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);

    // 呼叫組合語言，正式套用新的 IDT
    idt_flush((uint32_t)&idt_ptr);
}

```

#### 3. 組合語言中斷跳板 (`interrupts.S`)

當 CPU 遇到錯誤時，它沒辦法直接聰明地呼叫 C 語言函式，我們必須寫一段 Assembly 作為「跳板 (Trampoline)」，負責儲存當前的暫存器狀態，呼叫 C 語言，然後再恢復狀態。

請建立 `interrupts.S`：

```assembly
; interrupts.S
global idt_flush
global isr0
extern isr0_handler ; 告訴組譯器，這個函式在 C 語言裡

; 載入 IDT (跟昨天的 lgdt 幾乎一樣)
idt_flush:
    mov eax, [esp+4]  ; 取出 C 語言傳進來的 idt_ptr 位址
    lidt [eax]        ; Load IDT
    ret

; 第 0 號中斷的跳板程式 (除以零)
isr0:
    ; 關閉中斷，避免在處理錯誤時又被其他中斷打擾
    cli 
    
    ; 呼叫我們在 idt.c 裡寫的 C 語言處理函式
    call isr0_handler 
    
    ; 恢復中斷並返回 (但因為我們在 handler 裡寫了 hlt，其實不會走到這裡)
    sti
    iret

```

#### 4. 更新主程式 (`kernel.c`)

打開你的 `kernel.c`，引入 `idt.h` 並進行測試。

```c
// 在最上方引入
#include "gdt.h"
#include "idt.h" // [新增]

// ... 略過中間實作 ...

void kernel_main(void) {
    terminal_initialize();
    
    kprintf("=== OS Kernel Booting ===\n");
    
    init_gdt();
    kprintf("GDT loaded successfully.\n");

    // 初始化神經系統 IDT
    init_idt();
    kprintf("IDT loaded successfully.\n");

    // [致命測試] 故意觸發除以零錯誤！
    kprintf("Testing Divide by Zero Exception...\n");
    int a = 10;
    int b = 0;
    int c = a / b; // CPU 執行到這行會立刻拋出 0 號中斷！
    
    // 如果 IDT 沒設定好，QEMU 會無限重啟。
    // 如果設定好了，這行永遠不會印出來，系統會被我們的 ISR 攔截！
    kprintf("Result: %d\n", c); 
}

```

#### 5. 更新編譯腳本 (`Makefile`)

將新朋友 `idt.o` 與 `interrupts.o` 加入編譯鏈接清單：

```makefile
# 在 myos.bin 的依賴與 ld 指令中加入 idt.o 與 interrupts.o
myos.bin: boot.o kernel.o gdt.o gdt_flush.o idt.o interrupts.o
	$(DOCKER_CMD) ld -m elf_i386 -n -T linker.ld -o myos.bin boot.o kernel.o gdt.o gdt_flush.o idt.o interrupts.o

```

---

### 執行與驗證

執行 `make clean && make`，並啟動 QEMU。

**預期結果：**
你會看到系統順利載入 GDT 和 IDT。接著，當它嘗試計算 `10 / 0` 時，不會像昨天一樣莫名其妙當機重啟，而是會立刻觸發你的跳板程式，並在畫面上印出霸氣的紅字（如果是綠字也沒關係，重點是內容）：
`[KERNEL PANIC] Exception 0: Divide by Zero!`
`System Halted.`

這代表你的作業系統已經具備了**自我保護與錯誤捕捉**的能力！

---

### 🤔 Day 7 思考題

當我們成功捕捉了 CPU 內部的「例外錯誤」後，明天的目標是捕捉來自外部硬體（像是鍵盤）的訊號。但在電腦主機板上，鍵盤、滑鼠、時鐘等硬體，並不是直接把線連到 CPU 上的，中間有一個負責「排隊與過濾」訊號的晶片。

**你覺得為什麼主機板設計需要一個「中介晶片」來收集硬體中斷，而不是讓鍵盤直接對 CPU 發送中斷訊號呢？這解決了什麼系統架構上的問題？**

完成 Day 7 的編譯與測試後，請告訴我結果！我們隨時可以進入 Day 8 的硬體中斷控制器 (PIC) 與鍵盤驅動。
