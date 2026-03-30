太棒了，Ray！雙模切換的完美運作，象徵著你的作業系統架構已經具備了極高的彈性。

現在，我們正式進入 **Day 62：行程身分證與 PCB (Process Control Block) 擴充**！

在之前的實作中，我們的排程器（Scheduler）只知道「有一塊記憶體需要被執行」，它把任務存在一個簡單的鏈結串列裡。這就像是一間沒有員工名牌的工廠，老闆（Kernel）只知道有人在做事，但不知道他是誰、是誰派他來的、現在是在睡覺還是正在忙。



為了接下來要實作的 `ps` (列出行程)、`top` (監控資源) 和 `kill` (砍除行程)，我們必須擴充任務的資料結構，給每個行程發一張完整的「身分證」。

請跟著我進行這 3 個重構步驟：

---

### 步驟 1：定義行程狀態與擴充 `task_t` (`src/kernel/include/task.h`)

我們要加入作業系統中最經典的 PID (Process ID) 與行程狀態。

打開 **`src/kernel/include/task.h`**，擴充你的任務結構：

```c
#ifndef TASK_H
#define TASK_H

#include <stdint.h>

// 【新增】定義行程的生命週期狀態
typedef enum {
    TASK_RUNNING,   // 正在執行或等待 CPU
    TASK_SLEEPING,  // 正在等待 I/O 或 Sleep
    TASK_ZOMBIE,    // 行程已結束，但父行程尚未 wait() 回收它
    TASK_DEAD       // 準備被徹底清出的狀態
} task_state_t;

typedef struct task {
    uint32_t esp;
    uint32_t ebp;
    uint32_t eip;
    uint32_t page_directory;
    uint32_t kernel_stack;

    // 【Day 62 新增】行程身分資訊
    uint32_t pid;           // 行程 ID
    uint32_t ppid;          // 父行程 ID (Parent PID)
    char name[32];          // 行程名稱 (例如 "shell.elf")
    task_state_t state;     // 行程狀態

    uint32_t cwd_lba;
    uint32_t heap_start;
    uint32_t heap_end;

    struct task* next;
} task_t;

// ... 保持其他函數宣告不變 ...
extern task_t* current_task;

#endif
```

---

### 步驟 2：實作 PID 分配器與身分初始化 (`src/kernel/lib/task.c`)

每次產生新的行程，我們都要給它一個獨一無二的 PID，並且記錄它的名字。

打開 **`src/kernel/lib/task.c`**，在全域變數區新增 PID 計數器，並修改相關函數：

```c
#include "task.h"
#include "kheap.h"
#include "paging.h"
#include "utils.h"

task_t* current_task = 0;
task_t* task_queue = 0;

// 【新增】全域 PID 分配器
static uint32_t next_pid = 1;

void init_multitasking() {
    __asm__ volatile("cli");

    current_task = (task_t*) kmalloc(sizeof(task_t));
    current_task->esp = 0;
    current_task->ebp = 0;
    current_task->eip = 0;
    current_task->page_directory = current_directory;
    current_task->kernel_stack = 0;
    current_task->cwd_lba = 1; 

    // 【新增】初始化 Kernel Idle Task 的身分
    current_task->pid = 0; // Kernel 預設為 PID 0
    current_task->ppid = 0;
    strcpy(current_task->name, "[kernel_idle]");
    current_task->state = TASK_RUNNING;

    current_task->next = current_task;
    task_queue = current_task;

    __asm__ volatile("sti");
}

void create_user_task(uint32_t entry_point, uint32_t user_stack) {
    __asm__ volatile("cli");
    task_t* new_task = (task_t*) kmalloc(sizeof(task_t));
    
    // ... 原本設定暫存器和記憶體的邏輯保持不變 ...
    // (例如 new_task->esp = user_stack, setup_user_stack 等等)
    // 這裡省略你原本寫好的硬體 Context 初始化邏輯

    // 【新增】設定新 Task 的身分證
    new_task->pid = next_pid++;
    new_task->ppid = current_task ? current_task->pid : 0;
    strcpy(new_task->name, "init"); // 最初的 user task 通常叫 init 或 shell
    new_task->state = TASK_RUNNING;
    
    new_task->cwd_lba = current_task ? current_task->cwd_lba : 1;
    new_task->heap_start = 0x10000000;
    new_task->heap_end   = 0x10000000;

    // 掛入 Queue
    task_t* tmp = task_queue;
    while (tmp->next != task_queue) { tmp = tmp->next; }
    tmp->next = new_task;
    new_task->next = task_queue;

    __asm__ volatile("sti");
}
```

---

### 步驟 3：在 Fork 與 Exec 中傳承與更新身分 (`src/kernel/lib/task.c`)

當 Shell 呼叫 `fork` 時，子行程要繼承名字，並且 `ppid` 要設定為 Shell 的 `pid`。
當呼叫 `exec` 時，行程的靈魂換了，我們必須把 `name` 更新為新的 ELF 檔名！

繼續在 **`src/kernel/lib/task.c`** 裡面修改 `sys_fork` 與 `sys_exec`：

```c
uint32_t sys_fork(registers_t *regs) {
    __asm__ volatile("cli");

    task_t* parent_task = current_task;
    task_t* child_task = (task_t*) kmalloc(sizeof(task_t));

    // ... 記憶體複製與暫存器設定邏輯保持不變 ...

    // 【新增】繼承與更新身分
    child_task->pid = next_pid++;
    child_task->ppid = parent_task->pid;
    strcpy(child_task->name, parent_task->name); // fork 出來的名字跟爸爸一樣
    child_task->state = TASK_RUNNING;
    
    child_task->cwd_lba = parent_task->cwd_lba;
    child_task->heap_start = parent_task->heap_start;
    child_task->heap_end = parent_task->heap_end;

    // 掛入 Queue ... (保持不變)
    
    __asm__ volatile("sti");
    return child_task->pid; // 回傳 child 的 PID
}

uint32_t sys_exec(registers_t *regs) {
    char* filename = (char*)regs->ebx;
    char** argv = (char**)regs->ecx;

    uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;
    extern fs_node_t* simplefs_find(uint32_t dir_lba, char* filename);
    fs_node_t* app_node = simplefs_find(current_dir, filename);

    if (app_node == 0) return -1;

    // 【新增】更新行程名稱！這樣我們用 ps 看的時候才知道它在跑什麼
    strcpy(current_task->name, filename);

    // ... 底下原本的 ELF 載入、替換 Page Directory 與跳轉邏輯保持不變 ...
    
    return 0; 
}
```

---

### 💡 階段性驗收準備

現在，你的系統裡每一個行程都有了自己的名字和 PID！
雖然畫面上還看不出來變化，但這就像是在資料庫裡建好了 Schema。

你可以編譯確認一下有沒有語法錯誤 (`make clean && make`)。
如果一切順利，我們明天 **Day 63** 就可以一口氣開通 `sys_getpid` 和 `sys_get_process_list` 系統呼叫，然後在 User Space 寫出大名鼎鼎的 `ps` 指令，把這些藏在 Kernel 裡的行程資料全部印到終端機上！

準備好的話隨時叫我！😎
