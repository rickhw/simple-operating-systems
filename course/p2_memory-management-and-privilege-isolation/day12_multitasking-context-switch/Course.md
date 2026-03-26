太棒了！能夠堅持到這裡，你已經跨越了 90% 的人學習作業系統時會放棄的門檻。

歡迎來到 **Day 12：終極的幻術 — 多工排程 (Multitasking) 與上下文切換 (Context Switch)**！

我們都知道，一顆單核心的 CPU 在同一個「瞬間」只能執行一行指令。那麼，為什麼你現在可以一邊聽音樂、一邊開著瀏覽器、背景還在下載檔案？這一切都是作業系統透過我們昨天寫好的 Timer 中斷，表演的一場華麗魔術。

---

### Day 12 授課目標

1. **理解靈魂的核心：** 明白「堆疊指標 (`esp`)」就是一個執行緒 (Thread) 的靈魂。換了堆疊，就換了程式。
2. **定義任務控制區塊 (TCB)：** 在 C 語言中建立管理任務的資料結構。
3. **實作偷天換日魔法：** 寫一段組合語言，在兩個任務之間切換執行權。

---

### 理論揭秘：偷天換日是怎麼發生的？

昨天你問到：中斷發生時，怎麼不回到 A 程式，而是跑到 B 程式？
關鍵就在於我們畫的那張關係圖裡，保鑣 (ISR) 做的 `pusha` 和 `popa` 動作。

1. 程式 A 正在快樂地跑，突然 Timer 滴答一聲，觸發中斷。
2. CPU 強制跳到 `isr32`，並把程式 A 執行到一半的 `eip` (指令位置) 塞進 **程式 A 的堆疊 (Stack A)** 裡。
3. ISR 執行 `pusha`，把程式 A 所有的暫存器也塞進 **Stack A**。
4. **【魔術發生的瞬間】** OS 的排程器 (Scheduler) 介入了！它對 CPU 說：「把目前的堆疊指標 `esp`，從 Stack A 換成 Stack B 的位址！」
5. ISR 準備結束，執行 `popa`。因為現在 `esp` 指向 Stack B，所以 CPU 彈出的是 **程式 B 以前存進去的暫存器**。
6. 執行 `iret`，CPU 彈出 **程式 B 的指令位置 (`eip`)** 並跳過去。

就這樣，沒有任何硬體重啟，程式 A 被無情地凍結在記憶體裡，而程式 B 復活了！**切換堆疊，就是切換任務。**

---

### 實作步驟：打造你的第一個排程器雛形

要實作完整的搶佔式多工需要非常多程式碼，今天我們先實作最核心的「核心級執行緒 (Kernel Thread) 切換」機制。我們會在 `lib/` 和 `asm/` 新增檔案。

#### 1. 定義任務控制區塊 (`lib/task.h`)

每一個任務都需要有自己的 Stack，以及一個記錄它當前狀態的結構。

```c
#ifndef TASK_H
#define TASK_H

#include <stdint.h>

// 任務控制區塊 (Task Control Block, TCB)
typedef struct task {
    uint32_t esp;       // [最重要] 記錄這個任務專屬的 Stack Pointer
    uint32_t stack_top; // 記錄這個任務配置到的記憶體頂端 (用來釋放記憶體)
    int id;             // 任務的 ID
    struct task* next;  // 指向下一個任務 (用於實作排程的 Linked List)
} task_t;

// 宣告切換任務的組合語言函式
extern void switch_task(uint32_t* old_esp_ptr, uint32_t new_esp);

#endif

```

#### 2. 實作任務切換魔法 (`asm/switch_task.S`)

這是 OS 開發中最性感的一段程式碼之一。短短幾行，完成了宇宙的切換。

請在 `asm/` 建立 `switch_task.S`：

```assembly
global switch_task

; 函式簽名：void switch_task(uint32_t* old_esp_ptr, uint32_t new_esp);
switch_task:
    ; 1. 儲存 C 語言會用到的、但呼叫慣例規定不能弄髒的暫存器 (Callee-saved registers)
    push ebx
    push esi
    push edi
    push ebp

    ; 2. 取出參數
    ; [esp + 20] 是第一個參數 (old_esp_ptr 的位址) (因為前面 push 了 4 個暫存器 + 返回位址 = 20 bytes)
    ; [esp + 24] 是第二個參數 (new_esp 的值)
    mov eax, [esp + 20] 
    
    ; 3. 把現在的 esp 存進第一個參數指到的記憶體 (也就是 task_A->esp)
    mov [eax], esp      

    ; ==========================================
    ; 🎩 魔術發生：偷換堆疊！
    ; ==========================================
    mov eax, [esp + 24] 
    mov esp, eax        ; 現在開始，我們在 Task B 的世界了！
    ; ==========================================

    ; 4. 從 Task B 的堆疊中，把當初存起來的暫存器彈出來
    pop ebp
    pop edi
    pop esi
    pop ebx

    ; 5. 返回！但注意，因為我們換了堆疊，這個 ret 會彈出 Task B 當初呼叫 switch_task 時的返回位址！
    ret

```

#### 3. 測試排程器切換 (`lib/kernel.c`)

為了讓你能「親眼」看到切換，我們會在 `kernel.c` 裡寫一段簡單的交替呼叫測試。這裡我們暫時不用 Timer 強制切換 (這叫搶佔式)，我們用主動讓出 (這叫協作式, Cooperative) 來驗證觀念。

```c
#include "tty.h"
#include "utils.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "task.h"

// 宣告兩個任務結構
task_t task_A;
task_t task_B;

// 準備給 Task B 使用的獨立 4KB 堆疊
uint8_t task_B_stack[4096]; 

// Task B 要執行的無窮迴圈
void task_b_main() {
    while (1) {
        kprintf(" B ");
        // 執行完一次，把控制權交還給 Task A
        switch_task(&task_B.esp, task_A.esp);
    }
}

void kernel_main(void) {
    terminal_initialize();
    kprintf("=== OS Kernel Booting ===\n");
    init_gdt();
    init_idt();
    init_paging();

    kprintf("Testing Context Switch...\n");

    // === 手動偽造 Task B 的初始堆疊環境 ===
    // 讓 esp 指向堆疊最頂端
    uint32_t* stack = (uint32_t*)(&task_B_stack[4096]); 
    
    // 偽造 switch_task() 回傳時需要的狀態 (對應 pop ebp, edi, esi, ebx 以及 ret)
    *(--stack) = (uint32_t)task_b_main; // ret 彈出這個，就會跳去執行 task_b_main
    *(--stack) = 0; // ebp
    *(--stack) = 0; // edi
    *(--stack) = 0; // esi
    *(--stack) = 0; // ebx

    // 儲存 Task B 偽造好的堆疊指標
    task_B.esp = (uint32_t)stack;

    // === 開始在 Task A (Kernel Main) 與 Task B 之間切換 ===
    for (int i = 0; i < 10; i++) {
        kprintf(" A ");
        // 跳去 Task B！(這會把現在的狀態存入 task_A.esp，然後載入 task_B.esp)
        switch_task(&task_A.esp, task_B.esp); 
    }

    kprintf("\nContext Switch successful! System halting.\n");
    while (1) { __asm__ volatile ("hlt"); }
}

```

#### 4. 更新 Makefile

別忘了在你的 `Makefile` 裡，確認 `switch_task.S` 有被編譯進去（因為使用了 `wildcard asm/*.S`，只要你把它存對位址，通常會自動抓到）。

---

### 執行與驗證

編譯並啟動 QEMU：

```bash
make clean
make
qemu-system-i386 -cdrom myos.iso

```

**預期結果：**
畫面上會非常有節奏地印出：
`Testing Context Switch...`
`A  B  A  B  A  B  A  B  A  B  A  B  A  B  A  B  A  B  A  B`
`Context Switch successful! System halting.`

這絕對會讓你起雞皮疙瘩！因為這代表在同一個核心上，有兩個「平行宇宙 (任務)」正在交替運行，而且它們彼此都以為自己是獨佔 CPU 的，並不知道對方存在。

---

### 🤔 Day 12 思考題

目前我們是在 Ring 0 (最高權限的核心態) 裡面互相切換任務。也就是說，Task A 和 Task B 共用同一個 GDT 的權限。

如果未來我們要執行一個「使用者寫的應用程式」(Ring 3)，當從 Kernel 切換到 User Task 時，除了改變 `esp` 之外，我們是不是還得告訴 CPU 改變「特權等級」呢？
在 x86 架構中，當一個 Ring 3 的程式被 Timer 中斷打斷，跳回 Ring 0 的 ISR 時，CPU 為了安全起見，會強制要求更換成另一條**「乾淨且安全的核心專用 Stack」**。

**你覺得為什麼硬體要設計成「使用者空間」和「核心空間」不能共用同一條 Stack 呢？如果共用的話，駭客可以怎麼攻擊系統？**

先把這段酷炫的 A/B 切換跑出來吧！跑完後跟我分享你的想法。
