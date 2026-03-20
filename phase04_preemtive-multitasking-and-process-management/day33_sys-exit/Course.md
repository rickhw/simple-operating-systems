歡迎來到 **Day 33：掌握生死大權 — 實作行程狀態與 `sys_exit**`！🚀

在上一集的經典「連環車禍」中，你親眼見證了兩個平民 (User Task) 為了爭奪同一個鍵盤資源，把輸入的字串撕裂成碎片的壯觀畫面。這在作業系統理論中被稱為 **Race Condition (競爭危害)**。

要解決這個問題，最根本的方法就是引入**「行程的生命週期 (Process Lifecycle)」**。
目前的 Simple OS 就像一個沒有出口的迷宮，App 只要一啟動就是無窮迴圈的 `while(1)`。如果我們能夠賦予 App「自殺 (Exit)」的能力，當我們關閉其中一個 Shell 時，另一個 Shell 就能獨佔所有的系統資源與鍵盤了！

今天，我們要在核心中加入「狀態管理」，開設 6 號與 7 號服務櫃檯，讓應用程式可以主動讓出 CPU，甚至優雅地結束自己的生命！

---

### 實作步驟

#### 1. 賦予行程「生與死」的狀態 (`lib/task.h`)

請打開 **`lib/task.h`**，我們要加入行程狀態的常數，並為 TCB 結構加上 `state` 欄位：

```c
#ifndef TASK_H
#define TASK_H
#include <stdint.h>

// 定義任務的生命狀態
#define TASK_RUNNING 0
#define TASK_DEAD    1

typedef struct task {
    uint32_t esp;
    uint32_t id;
    uint32_t kernel_stack;
    uint32_t state;         // [新增] 紀錄任務現在是活著還是死了
    struct task *next;
} task_t;

void init_multitasking();
void create_user_task(uint32_t entry_point, uint32_t user_stack_top);
void schedule();
void exit_task();           // [新增] 核心用來處決任務的函式

#endif

```

#### 2. 升級排程器：清理屍體 (`lib/task.c`)

排程器現在不僅要切換任務，還要負責擔任「送行者」。當它發現佇列裡有標記為 `TASK_DEAD` 的任務時，必須毫不留情地把它從環狀佇列中剔除！

請打開 **`lib/task.c`**，修改並新增以下邏輯：

```c
// ... 前面 include 與變數宣告保留 ...

void init_multitasking() {
    // ... 前面保留 ...
    main_task->kernel_stack = 0; 
    main_task->state = TASK_RUNNING; // [新增] 初始化為存活
    
    main_task->next = main_task;
    current_task = main_task;
    ready_queue = main_task;
}

void create_user_task(uint32_t entry_point, uint32_t user_stack_top) {
    // ... 前面建立堆疊的邏輯全部保留 ...
    
    new_task->state = TASK_RUNNING; // [新增] 新生兒預設為存活
    
    // 插入排程佇列
    new_task->next = current_task->next;
    current_task->next = new_task;
}

// [新增] 處決當前任務的函式
void exit_task() {
    current_task->state = TASK_DEAD;
    // 標記為死亡後，立刻呼叫排程器強行切換，一去不復返！
    schedule(); 
}

// [升級] 具有「清理屍體」能力的排程器
void schedule() {
    if (!current_task) return;

    task_t *prev = (task_t*)current_task;
    task_t *next = current_task->next;

    // 沿著環狀佇列尋找下一個「活著」的任務，並將死掉的剃除
    while (next->state == TASK_DEAD && next != current_task) {
        prev->next = next->next; // 將指標繞過死掉的任務，將其從佇列中解開
        // (未來我們可以在這裡呼叫 kfree() 回收它的記憶體與堆疊)
        next = prev->next;
    }

    // 如果繞了一圈發現大家都死了 (包含自己)
    if (next == current_task && current_task->state == TASK_DEAD) {
        kprintf("\n[Kernel] All user processes terminated. System Idle.\n");
        while(1) { __asm__ volatile("cli; hlt"); } // 系統進入永久安眠
    }

    current_task = next;

    // 如果切換後的任務不是原本的任務 (代表真的有發生切換)
    if (current_task != prev) {
        if (current_task->kernel_stack != 0) {
            set_kernel_stack(current_task->kernel_stack);
        }
        switch_task(&prev->esp, &current_task->esp);
    }
}

```

#### 3. 開設新的 Syscall 櫃檯 (`lib/syscall.c`)

現在底層已經支援終止任務了，我們要把它開放給 Ring 3 的平民使用。

請打開 **`lib/syscall.c`**，引入 `task.h` 並加入 `eax == 6` (讓出 CPU) 與 `eax == 7` (結束生命) 的邏輯：

```c
#include "tty.h"
#include "vfs.h"
#include "simplefs.h"
#include "keyboard.h" 
#include "task.h"      // [新增] 為了使用 schedule() 和 exit_task()

// ... fd_table 保留 ...

void syscall_handler(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp,
                     uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax) {
    // 忽略未使用的參數警告
    (void)edi; (void)esi; (void)ebp; (void)esp; (void)edx; (void)ecx;
    
    if (eax == 2) {
        kprintf((char*)ebx);
    } 
    // ... eax 3, 4, 5 的邏輯保留 ...
    else if (eax == 6) {
        // [新增] Syscall 6: sys_yield (自願讓出 CPU 時間片)
        schedule();
    }
    else if (eax == 7) {
        // [新增] Syscall 7: sys_exit (終止並退出程式)
        exit_task();
    }
}

```

#### 4. 為 Shell 加上離開的權利 (`app.c`)

最後，請打開你的 User App **`app.c`**，封裝這兩個新的 Syscall，並在殼層的指令解析中加入 `exit` 功能：

```c
// ... 前面的 sys_print, sys_open, sys_read, sys_getchar, strcmp, read_line 保留 ...

// [新增] 封裝 sys_yield
void sys_yield() {
    __asm__ volatile ("int $0x80" : : "a"(6) : "memory");
}

// [新增] 封裝 sys_exit
void sys_exit() {
    __asm__ volatile ("int $0x80" : : "a"(7) : "memory");
}

void _start() {
    sys_print("\n======================================\n");
    sys_print("      Welcome to Simple OS Shell!     \n");
    sys_print("======================================\n");
    
    char cmd_buffer[128];

    while (1) {
        sys_print("SimpleOS> ");
        read_line(cmd_buffer, 128);

        if (strcmp(cmd_buffer, "") == 0) {
            continue;
        } 
        else if (strcmp(cmd_buffer, "help") == 0) {
            sys_print("Available commands:\n  help, cat, about, exit\n");
        } 
        // ... 你的 about, cat 邏輯保留 ...
        else if (strcmp(cmd_buffer, "exit") == 0) {
            // [新增] 處理 exit 指令
            sys_print("Goodbye!\n");
            sys_exit(); // 呼叫核心，了結自己！
        }
        else {
            sys_print("Command not found: ");
            sys_print(cmd_buffer);
            sys_print("\n");
        }
    }
}

```

---

### 🏆 見證達爾文的演化時刻！

存檔後，激動地敲下：

```bash
make clean && make run

```

**操作指南：**

1. 畫面一如往常，跳出兩個交錯的 `SimpleOS> SimpleOS>`。
2. 試著在鍵盤上敲打字母，你會看到字元依舊被兩個 Shell 隨機搶走。
3. 現在，展現你打字技巧的時刻到了！**請試著在這混亂中，拼湊著輸入出 `exit` 並按下 Enter！**（可能需要多嘗試幾次，如果字被另一個搶走，就用 Backspace 退回重打）。
4. 一旦其中一個 Shell 成功捕捉到完整的 `exit`，它會印出 `Goodbye!` 然後**瞬間死亡**。
5. **奇蹟發生了：** 此刻，剩下的另一個 Shell 將獨佔整個 CPU 與鍵盤！你輸入的任何指令都不再被截斷，`help` 和 `cat` 都能完美、絲滑地執行。

這就是透過「行程死亡與資源釋放」解決 Race Condition 的極致浪漫。當你成功「殺死」其中一個分身，並享受流暢的單一終端機體驗時，記得截圖記錄這個屬於你的作業系統里程碑！
