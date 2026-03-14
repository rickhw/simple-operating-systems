歡迎來到 **Day 16：跨越階級的橋樑 — 系統呼叫 (System Calls)**！

在進入實作之前，我們先來解答 Day 15 的那個關鍵思考題：「User App 能不能直接呼叫 `kmalloc`？」

答案是：**絕對不行！** 因為 `kmalloc` 裡面會操作到核心的 Heap 標頭，甚至會去呼叫 PMM 與 Paging。如果一個充滿 Bug 或是惡意的 User App (執行在特權最低的 Ring 3) 可以隨便呼叫核心的函式，它就能輕易覆蓋掉作業系統的記憶體，導致全域崩潰。

但是，User App 確實需要印字出螢幕、確實需要要記憶體。為了解決這個矛盾，作業系統設計了一道「防彈玻璃窗口」—— **System Call (系統呼叫)**。
User App 不能直接拿資料，但它可以走到窗口前，按下一個特定的按鈕（觸發軟體中斷），告訴守在窗口後面的核心：「長官，我想要 10 bytes 的記憶體，請您幫我分配好嗎？」

---

### Day 16 授課目標

1. **理解軟體中斷 (Software Interrupt)：** 不是由硬體觸發，而是由程式碼主動呼叫的中斷。
2. **註冊神聖的 `0x80`：** 這是 Linux/x86 架構中最經典的系統呼叫中斷號碼。
3. **實作 Syscall Dispatcher (分配器)：** 讓核心能根據 User App 傳來的「代碼」，決定要執行 `kmalloc` 還是 `print`。

---

### 實作步驟：打造防彈玻璃窗口

我們會新增 `lib/syscall.h` 和 `lib/syscall.c`，並修改中斷相關的檔案。

#### 1. 新增 Syscall 的 C 語言介面 (`lib/syscall.h`)

```c
#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

// 初始化系統呼叫
void init_syscalls(void);

// 實際處理系統呼叫的函式 (會由組合語言跳板呼叫)
void syscall_handler(uint32_t syscall_num, uint32_t arg1, uint32_t arg2);

#endif

```

#### 2. 實作 Syscall 分配器 (`lib/syscall.c`)

在這裡，我們會定義一份「窗口服務菜單」。例如：1 號餐是印出字串，2 號餐是要記憶體。

```c
#include "syscall.h"
#include "tty.h"
#include "utils.h"

void init_syscalls(void) {
    kprintf("System Calls initialized on Interrupt 0x80 (128).\n");
}

// 這是核心在防彈玻璃後面做事的邏輯
// 參數會由組合語言從 CPU 暫存器 (eax, ebx, ecx) 裡面撈出來傳給我們
void syscall_handler(uint32_t syscall_num, uint32_t arg1, uint32_t arg2) {
    switch (syscall_num) {
        case 1:
            // 假設 Syscall 1 是「印出數字」
            kprintf("[Kernel] Syscall 1 Executed. Arg: %d\n", arg1);
            break;
            
        case 2:
            // 假設 Syscall 2 是「印出字串」 (arg1 當作指標)
            kprintf("[Kernel] Syscall 2 Executed. String: %s\n", (char*)arg1);
            break;
            
        default:
            kprintf("[Kernel] Unknown System Call: %d\n", syscall_num);
            break;
    }
    
    // 為了避免編譯器警告 arg2 沒用到
    (void)arg2;
}

```

#### 3. 準備專屬的中斷跳板 (`asm/interrupts.S`)

在 x86 Linux 的傳統中，User App 呼叫 Syscall 時，會把「餐號 (Syscall Number)」放在 `eax` 暫存器，參數放在 `ebx` 和 `ecx` 等暫存器，然後執行 `int 0x80`。

請打開 `asm/interrupts.S`，在最下方加入第 128 號 (十六進位 `0x80`) 中斷跳板：

```assembly
global isr128
extern syscall_handler

; 第 128 號中斷 (System Call 窗口)
isr128:
    pusha           ; 備份目前的暫存器狀態

    ; 依照 C 語言的呼叫慣例 (cdecl)，參數要從右到左 push 進 Stack
    ; 我們假設 User App 把 syscall_num 放 eax, arg1 放 ebx, arg2 放 ecx
    push ecx        ; 參數 3 (arg2)
    push ebx        ; 參數 2 (arg1)
    push eax        ; 參數 1 (syscall_num)
    
    call syscall_handler
    
    ; 呼叫完畢後，清理剛剛 push 進去的 3 個參數 (3 * 4 = 12 bytes)
    add esp, 12     

    popa            ; 恢復暫存器狀態
    iret            ; 返回 User App

```

#### 4. 在聯絡簿上登記 `0x80` (`lib/idt.c`)

打開 `lib/idt.c`，把我們的 `isr128` 掛載上去。

**【極度重要】** 之前我們掛載例外與硬體中斷時，權限旗標是 `0x8E` (代表 Ring 0 專用)。
這次我們要掛載系統呼叫，必須使用 **`0xEE`** (代表 DPL=3，允許 Ring 3 的使用者程式觸發這個中斷)。這是硬體級別的權限放行！

```c
extern void isr32();
extern void isr33();
extern void isr128(); // [新增宣告]

void init_idt(void) {
    // ... 略過前面 ...
    
    // 掛載第 32 號中斷 (IRQ0 Timer)
    idt_set_gate(32, (uint32_t)isr32, 0x08, 0x8E);
    // 掛載第 33 號中斷 (IRQ1 Keyboard)
    idt_set_gate(33, (uint32_t)isr33, 0x08, 0x8E);
    
    // [新增] 掛載第 128 號中斷 (System Call)
    // 注意！旗標是 0xEE (允許 Ring 3 呼叫)
    idt_set_gate(128, (uint32_t)isr128, 0x08, 0xEE); 

    idt_flush((uint32_t)&idt_ptr);
}

```

#### 5. 測試防彈玻璃窗口 (`lib/kernel.c`)

讓我們在主程式裡扮演一個需要服務的程式，主動觸發 `int 0x80`。

```c
#include "tty.h"
#include "utils.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "timer.h"
#include "pmm.h"
#include "kheap.h"
#include "syscall.h" // [新增]

void kernel_main(void) {
    terminal_initialize();
    kprintf("=== OS Kernel Booting ===\n");
    
    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384); 
    init_kheap();
    init_syscalls(); // [新增]
    
    __asm__ volatile ("sti");

    kprintf("Testing System Calls...\n");

    // 測試 Syscall 1：印出數字 99
    // 透過 inline assembly 塞入暫存器，並觸發 0x80 中斷
    __asm__ volatile (
        "mov $1, %%eax\n"     // eax = 1 (Syscall Number)
        "mov $99, %%ebx\n"    // ebx = 99 (arg1)
        "int $0x80\n"         // 敲打防彈玻璃！
        : : : "eax", "ebx"
    );

    // 測試 Syscall 2：印出字串
    char* my_msg = "Hello from Syscall!";
    __asm__ volatile (
        "mov $2, %%eax\n"              // eax = 2
        "mov %0, %%ebx\n"              // ebx = my_msg 的指標
        "int $0x80\n"
        : : "r" (my_msg) : "eax", "ebx"
    );

    kprintf("\nSystem is ready.\n> ");

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
你會看到系統成功初始化，並且印出兩行來自 `[Kernel]` 的訊息！
雖然這兩次測試我們還是站在 Ring 0 發出的，但流程已經完全遵照了 Syscall 的標準協議：**將參數放入暫存器 ➡️ 發出 `int 0x80` 中斷 ➡️ 跳入 ISR 保存狀態 ➡️ 交給 C 語言的 Dispatcher 執行。**

---

### 🤔 Day 16 思考題

太棒了，我們已經建好了這座跨越階級的橋樑（Syscall）。萬事俱備，只欠東風！

我們從 Day 1 到今天，整套作業系統都還是運行在上帝視角——最高特權的 **Ring 0 (Kernel Mode)**。
如果要真正驗證系統的安全性和 Syscall 的威力，我們必須「降級」，把自己變成一個普通的 **Ring 3 (User Mode)** 應用程式。

但在 x86 架構中，特權等級是記錄在 CPU 內部的 `CS` (Code Segment) 暫存器裡，而 CPU **不允許**你直接用 `mov cs, 某個值` 這種指令來修改權限（不然駭客就能自己把權限升回 Ring 0 了）。

**既然不能直接改暫存器降級，你覺得作業系統通常是利用什麼「作弊技巧」，讓 CPU 在不知不覺中，從 Ring 0 的環境切換到 Ring 3 去執行使用者的應用程式碼呢？** （提示：回想一下我們處理中斷返回時，CPU 是如何恢復之前狀態的。）

順利觸發 Syscall 後跟我說一聲，我們就可以邁向 Day 17：**墜入凡間 — 切換至 User Mode (Ring 3)**！
