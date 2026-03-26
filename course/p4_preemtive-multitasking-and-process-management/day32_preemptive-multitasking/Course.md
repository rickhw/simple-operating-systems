準備好讓你的 Simple OS 邁向另一個全新境界了嗎？

在 Day 31，我們成功讓兩個「核心執行緒 (Ring 0)」在背景完美交替執行。但真正的作業系統，是要能同時管理多個「使用者應用程式 (Ring 3)」。

今天，我們要挑戰 OS 排程器中最精密、也最容易踩坑的機關：**跨特權階級的多工切換 (Ring 3 Preemptive Multitasking)**。

### 🤔 為什麼 Ring 3 多工特別難？(TSS 的秘密)

當 CPU 執行 Ring 3 的應用程式時，如果發生了時鐘中斷 (Timer Interrupt)，CPU 必須立刻切換到 Ring 0 的**核心堆疊 (Kernel Stack)** 來處理中斷。
問題來了：CPU 怎麼知道核心堆疊在哪裡？答案是：**它會去查 TSS 結構裡面的 `esp0` 欄位**。

如果我們同時跑了 App 1 和 App 2，但只給它們共用同一個 `esp0`（同一個核心堆疊），當 App 1 被中斷時，狀態存進了堆疊；接著切換到 App 2，App 2 執行到一半也被中斷，**App 2 的狀態就會直接覆寫掉 App 1 的狀態**！系統立刻崩潰。

👉 **唯一解法：** 每個 Ring 3 的應用程式，除了要有自己的 User Stack，還必須配發一個專屬的 **Kernel Stack**！每次排程器切換任務時，都要順手更新 TSS 的 `esp0`！

---

### Day 32 實作步驟：影分身之術

#### 1. 升級任務結構 (`lib/task.h`)

請打開 **`lib/task.h`**，為任務新增 `kernel_stack` 欄位，並宣告新的函數：

```c
#ifndef TASK_H
#define TASK_H
#include <stdint.h>

typedef struct task {
    uint32_t esp;           // 任務目前的堆疊指標 (Context Switch 用)
    uint32_t id;
    uint32_t kernel_stack;  // [新增] 此任務專屬的核心堆疊頂端位址 (供 TSS 使用)
    struct task *next;
} task_t;

void init_multitasking();
void create_user_task(uint32_t entry_point, uint32_t user_stack_top); // [新增] 建立平民任務
void schedule();

#endif

```

#### 2. 神奇的中斷返回跳板 (`asm/switch_task.S`)

當我們第一次把 CPU 交給一個全新的 App 時，我們必須「偽造」一個它剛剛經歷完中斷的假象，讓 CPU 乖乖執行 `iret` 降級到 Ring 3。

請打開 **`asm/switch_task.S`**，在最下面補上這個神奇的跳板函式：

```assembly
[bits 32]
global switch_task
global task_return_stub  ; [新增] 暴露給 C 語言使用

switch_task:
    pusha
    pushf
    mov eax, [esp + 40] 
    mov [eax], esp      
    mov eax, [esp + 44] 
    mov esp, [eax]      
    popf            
    popa            
    ret             

; [新增] 新任務的第一個起點！
; 當 switch_task 執行 ret 後，新任務會跳來這裡
; 這裡的 Stack 狀態，被我們偽造得跟 ISR 準備結束時一模一樣！
task_return_stub:
    popa            ; 恢復 8 個通用暫存器
    add esp, 8      ; 跳過 Error Code 和 Int Number (各 4 bytes)
    iret            ; 完美降級至 Ring 3！

```

#### 3. 實作 Ring 3 任務產生器 (`lib/task.c`)

這是今天最硬核的程式碼。我們要手工打造出一個包含 `SS, ESP, EFLAGS, CS, EIP` 的標準中斷返回幀 (IRET Frame)！

請打開 **`lib/task.c`**，替換/新增以下內容：

```c
#include "task.h"
#include "pmm.h"
#include "kheap.h"
#include "tty.h"
#include "gdt.h" // 為了使用 set_kernel_stack()

volatile task_t *current_task = 0;
volatile task_t *ready_queue = 0;
uint32_t next_task_id = 0;

extern void switch_task(uint32_t *current_esp, uint32_t *next_esp);
extern void task_return_stub(); // 剛剛在組合語言寫的跳板

void init_multitasking() {
    kprintf("[Task] Initializing Multitasking...\n");
    task_t *main_task = (task_t*) kmalloc(sizeof(task_t));
    main_task->id = next_task_id++;
    main_task->esp = 0;
    main_task->kernel_stack = 0; // 主核心不需要特別設定
    main_task->next = main_task;
    current_task = main_task;
    ready_queue = main_task;
}

// [新增] 建立 Ring 3 任務的終極工廠
void create_user_task(uint32_t entry_point, uint32_t user_stack_top) {
    task_t *new_task = (task_t*) kmalloc(sizeof(task_t));
    new_task->id = next_task_id++;

    // 1. 配發此任務專屬的 Kernel Stack (4KB)
    uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
    new_task->kernel_stack = (uint32_t) kstack;

    // 2. 偽造 Ring 3 的硬體中斷幀 (給 iret 使用)
    *(--kstack) = 0x23;             // SS (User Data Segment，RPL=3)
    *(--kstack) = user_stack_top;   // ESP (平民專屬堆疊)
    *(--kstack) = 0x0202;           // EFLAGS (IF=1，開啟中斷)
    *(--kstack) = 0x1B;             // CS (User Code Segment，RPL=3)
    *(--kstack) = entry_point;      // EIP (程式進入點)

    // 3. 偽造 ISR 壓入的除錯資訊
    *(--kstack) = 0; // Error Code
    *(--kstack) = 0; // Int Number

    // 4. 偽造 task_return_stub 裡的 popa (8 個暫存器)
    for(int i=0; i<8; i++) *(--kstack) = 0;

    // 5. 偽造 switch_task 的返回位址與狀態
    *(--kstack) = (uint32_t) task_return_stub; // switch_task 的 ret 會跳去這裡！
    *(--kstack) = 0x0202; // switch_task 的 pushf

    new_task->esp = (uint32_t) kstack;

    // 插入排程佇列
    new_task->next = current_task->next;
    current_task->next = new_task;
}

void schedule() {
    if (!current_task || current_task->next == current_task) return;
    
    task_t *prev_task = (task_t*)current_task;
    current_task = current_task->next; 

    // 【關鍵魔法】每次切換任務前，更新 TSS 的 esp0！
    // 這樣下一次中斷發生時，CPU 才會把狀態存進這個新任務自己的 Kernel Stack 裡！
    if (current_task->kernel_stack != 0) {
        set_kernel_stack(current_task->kernel_stack);
    }

    switch_task(&prev_task->esp, &current_task->esp);
}

```

#### 4. 啟動雙胞胎 Shell！(`lib/kernel.c`)

現在，我們要見證歷史了。我們將從硬碟讀出 **同一個** Shell 應用程式，但我們會呼叫兩次 `create_user_task`，給它們**不同的 User Stack**，讓它們成為兩個獨立的行程同時在螢幕上執行！

請打開 **`lib/kernel.c`**，修改載入與執行的部分：

```c
// ... 前面掛載與格式化 SimpleFS 保留 ...

    kprintf("[Kernel] Fetching 'my_app.elf' from Virtual File System...\n");
    fs_node_t* app_node = simplefs_find("my_app.elf");
    
    if (app_node != 0) {
        uint8_t* app_buffer = (uint8_t*) kmalloc(app_node->length);
        vfs_read(app_node, 0, app_node->length, app_buffer);
        uint32_t entry_point = elf_load((elf32_ehdr_t*)app_buffer);
        
        if (entry_point != 0) {
            kprintf("Creating TWO independent User Tasks...\n\n");
            
            // 啟動多工作業排程器
            init_multitasking();

            // 為 App 1 分配專屬 User Stack (映射到 0x083FF000)
            uint32_t ustack1_phys = (uint32_t) pmm_alloc_page();
            map_page(0x083FF000, ustack1_phys, 7);
            
            // 為 App 2 分配專屬 User Stack (映射到 0x084FF000)
            uint32_t ustack2_phys = (uint32_t) pmm_alloc_page();
            map_page(0x084FF000, ustack2_phys, 7);

            // 建立兩個 Ring 3 任務！它們擁有相同的起點，但各自的 Stack 完全獨立！
            create_user_task(entry_point, 0x083FF000 + 4096);
            create_user_task(entry_point, 0x084FF000 + 4096);

            // Kernel 功成身退，開啟中斷讓 Timer 接管世界！
            kprintf("Kernel dropping to idle state. Have fun!\n");
            __asm__ volatile ("sti"); 
            while(1) { __asm__ volatile ("hlt"); }
        }
    }

```

---

### 執行與驗證！

深呼吸，輸入：

```bash
make clean && make run

```

**你將會看到什麼震撼的畫面？**
因為兩個 Shell 會同時執行並爭搶螢幕輸出，你很可能會看到兩個 Shell 互相交錯印出歡迎詞：

```text
======================================
======Welcome to Simple OS Shell!     
======================================
      Welcome to Simple OS Shell!     
...
SimpleOS> SimpleOS> 

```

這畫面雖然有點混亂，但這絕對是**最純粹、最暴力的並行處理 (Concurrency) 的視覺證明**！

當你敲打鍵盤時，兩個 Shell 可能會同時搶著把字元讀走（因為它們共用同一個鍵盤環狀緩衝區），導致指令輸入變得斷斷續續。

這不是 Bug，這就是作業系統設計裡經典的 **Race Condition (競爭危害)**！我們不僅成功實現了多工作業，還完美地重現了作業系統課本上的經典問題！跑起來之後跟我說，這成就感是不是無與倫比？
