歡迎來到 **Phase 4：搶佔式多工與行程管理**！🚀

前 30 天，我們的作業系統就像是一個「單行道」。雖然我們成功在 Ring 3 跑起了 Shell，但在同一時間內，CPU 只能全心全意地服侍這唯一的一個應用程式。如果 Shell 進入了無窮迴圈（例如等待鍵盤輸入），整個系統就等於被卡住了。

在接下來的階段，我們要賦予系統**「影分身之術」**！
我們要讓作業系統利用硬體時鐘（PIT），每隔幾毫秒就無情地把 CPU 從目前的程式手中搶過來（這就是「搶佔式 Preemptive」），切換給下一個程式執行。因為切換速度太快了，人類的肉眼會覺得這兩個程式是「同時」在執行的！

這就是現代 OS 多工作業 (Multitasking) 的核心魔法：**Time-Slicing (時間切片) 與 Context Switch (上下文切換)**。

準備好讓你的 OS 迎來第二個生命了嗎？**Day 31：打造排程器與上下文切換！**
今天我們先從 Ring 0 的「核心執行緒 (Kernel Threads)」切換開始，確保排程器運作正常！

---

### 實作步驟

#### 1. 核心魔法陣：上下文切換 (`asm/switch_task.S`)

我注意到你的 Makefile 裡已經有編譯 `asm/switch_task.S` 的規則了。我們需要確保裡面的組合語言正確無誤。

這個函式的作用是：把 CPU 當下的所有狀態（暫存器）壓入「目前任務」的堆疊，然後把堆疊指標 (`esp`) 換成「下一個任務」的堆疊，最後把狀態彈出來。就這麼簡單的一個貍貓換太子！

請檢查或建立 **`asm/switch_task.S`**：

```assembly
[bits 32]
global switch_task

; void switch_task(uint32_t *current_esp, uint32_t *next_esp);
switch_task:
    ; 1. 備份當前任務的狀態到它自己的 Stack
    pusha           ; 壓入 8 個通用暫存器 (eax, ecx, edx, ebx, esp, ebp, esi, edi)
    pushf           ; 壓入 EFLAGS 狀態暫存器

    ; 2. 取得 current_esp 的指標 (在堆疊中的位置：4個狀態+8個暫存器+返回位址 = 44)
    mov eax, [esp + 44] 
    ; 將目前的 esp 存入 current_esp 所指向的記憶體
    mov [eax], esp      

    ; 3. 取得 next_esp 的指標
    mov eax, [esp + 48] 
    ; 【狸貓換太子】把 esp 換成下一個任務的 stack！
    mov esp, [eax]      

    ; 4. 從新任務的 Stack 恢復狀態
    popf            
    popa            
    ret             ; 返回！但這時的 ret 會回到下一個任務原本暫停的地方！

```

#### 2. 定義任務結構與排程器 (`lib/task.c` & `lib/task.h`)

現在我們需要 C 語言來管理這些任務。因為你的 Makefile 寫了 `$(wildcard lib/*.c)`，所以只要在 `lib/` 建立檔案，它就會自動被編譯進去！

請建立 **`lib/task.h`**：

```c
#ifndef TASK_H
#define TASK_H
#include <stdint.h>

// 任務控制區塊 (TCB)
typedef struct task {
    uint32_t esp;       // 儲存這個任務的堆疊指標
    uint32_t id;        // 任務 ID
    uint32_t *stack;    // 分配給這個任務的實體堆疊記憶體 (方便日後回收)
    struct task *next;  // 指向下一個任務 (形成環狀連結串列)
} task_t;

void init_multitasking();
void create_kernel_thread(void (*entry_point)());
void schedule();

#endif

```

接著建立 **`lib/task.c`**：

```c
#include "task.h"
#include "pmm.h"
#include "kheap.h"
#include "tty.h"

volatile task_t *current_task = 0;
volatile task_t *ready_queue = 0;
uint32_t next_task_id = 0;

extern void switch_task(uint32_t *current_esp, uint32_t *next_esp);

void init_multitasking() {
    kprintf("[Task] Initializing Multitasking...\n");
    
    // 為正在執行的「主程式」建立一個任務區塊
    task_t *main_task = (task_t*) kmalloc(sizeof(task_t));
    main_task->id = next_task_id++;
    main_task->esp = 0; 
    main_task->next = main_task; // 自己指自己，形成一個人的環

    current_task = main_task;
    ready_queue = main_task;
}

void create_kernel_thread(void (*entry_point)()) {
    task_t *new_task = (task_t*) kmalloc(sizeof(task_t));
    new_task->id = next_task_id++;
    
    // 向 PMM 要一頁 (4KB) 作為這個新執行緒的 Stack
    uint32_t stack_phys = pmm_alloc_page();
    // 在核心模式下，我們可以直接使用實體位址作為 Stack (假設尚未啟用嚴格高位址映射)
    uint32_t *stack = (uint32_t*) (stack_phys + 4096); 
    new_task->stack = stack;

    // 偽造一個「剛剛被中斷」的 Stack 狀態
    *(--stack) = (uint32_t) entry_point; // ret 彈出的 EIP (要執行的函數)
    *(--stack) = 0x0202;                 // EFLAGS (中斷開啟 IF=1)
    *(--stack) = 0; // EAX
    *(--stack) = 0; // ECX
    *(--stack) = 0; // EDX
    *(--stack) = 0; // EBX
    *(--stack) = 0; // ESP (pusha 會忽略這個值)
    *(--stack) = 0; // EBP
    *(--stack) = 0; // ESI
    *(--stack) = 0; // EDI

    new_task->esp = (uint32_t) stack;

    // 把新任務插進環狀佇列中 (掛在 current_task 後面)
    new_task->next = current_task->next;
    current_task->next = new_task;
}

// 排程器：負責挑選下一個任務並切換
void schedule() {
    if (!current_task || current_task->next == current_task) return;

    task_t *prev_task = (task_t*)current_task;
    current_task = current_task->next; // 輪到下一個！

    switch_task(&prev_task->esp, &current_task->esp);
}

```

#### 3. 把計時器變成心跳 (`lib/timer.c`)

硬體計時器 (PIT) 是排程器的心跳。每一次「滴答」，我們就強迫執行 `schedule()`！

請打開 **`lib/timer.c`**，在你的 `timer_handler` 裡加入排程呼叫：

```c
#include "timer.h"
#include "tty.h"
#include "io.h"
#include "task.h" // [新增]

uint32_t tick = 0;

void timer_handler() {
    tick++;
    
    // 每 100 個 ticks 印一次時間 (避免洗畫面)
    // if (tick % 100 == 0) {
    //    kprintf("Uptime: %d seconds\n", tick / 100);
    // }

    // [新增] 每次時鐘中斷，就呼叫排程器強行切換任務！
    schedule(); 

    // 發送 EOI 給 PIC
    outb(0x20, 0x20);
}

```

#### 4. 在 Kernel 裡見證分身魔法 (`lib/kernel.c`)

為了測試排程器，我們今天先不急著進 Ring 3，我們要在 `kernel_main` 裡創建兩個核心執行緒！

請打開 **`lib/kernel.c`**，在 `init_kheap();` 之後加入測試代碼：

```c
// ... 前面系統初始化保留 ...
    init_kheap(); 
    
    // [新增] 啟動多工作業
    init_multitasking();

    __asm__ volatile ("sti"); 
    kprintf("=== OS Subsystems Ready ===\n\n");

    // 宣告兩個無窮迴圈的任務函數
    extern void thread_A();
    extern void thread_B();
    
    // 建立兩個分身任務
    create_kernel_thread(thread_A);
    create_kernel_thread(thread_B);

    kprintf("Starting Multitasking Demo! (Press Ctrl+C to exit QEMU later)\n");

    // Kernel 本身 (Main Task) 變成一個無窮迴圈
    while(1) {
        // 主執行緒可以選擇什麼都不做，因為 Timer 會自動切換到 A 和 B
        __asm__ volatile("hlt");
    }

```

然後在 `kernel.c` 最底下補上這兩個測試函數：

```c
// 測試用執行緒 A
void thread_A() {
    while(1) {
        kprintf("A");
        // 為了讓人眼能看清楚，我們用空迴圈稍微拖慢速度
        for(volatile int i = 0; i < 5000000; i++); 
    }
}

// 測試用執行緒 B
void thread_B() {
    while(1) {
        kprintf("B");
        for(volatile int i = 0; i < 5000000; i++);
    }
}

```

*(💡 註：為了專注測試排程器，你可以先暫時把載入硬碟 App 的程式碼註解掉)*

---

### 執行與見證奇蹟！

大口深呼吸，輸入：

```bash
make clean && make run

```

**預期結果 🏆**
你會看到螢幕上開始狂印：
`ABABABABABABABABABABABABABABABAB...`

**這代表什麼？**
程式 A 執行到一半，Timer 中斷觸發，硬生生地把 CPU 搶走，把 A 的狀態壓入 Stack；接著拿出 B 的 Stack，讓程式 B 甦醒印出 "B"；然後 Timer 再次觸發，又把 CPU 搶回給 A！

這就是作業系統真正的核心大腦——**搶佔式排程器**！
它證明了你的 OS 現在擁有了同時管理多個靈魂的能力。試著跑起來看看，成功印出 `ABAB` 後跟我說，我們 Day 32 再來挑戰把它應用回 Ring 3 的應用程式上！
