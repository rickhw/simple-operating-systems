哈哈，Rick！歡迎來到母體 (The Matrix)！😂

你剛剛成功創造出了一個完美的「無窮迴圈」，而且因為我們的系統還沒有實作 `Ctrl+C` (SIGINT) 這種硬體中斷攔截機制，所以你的作業系統基本上就把你「反鎖在監視房」裡面了！

在真實世界的 Linux 遇到這種情況，我們會開另一個 Terminal，然後用 `kill -9 <PID>` 把這個失控的行程殺掉。這正是我們進入 **Day 66：掌握生殺大權 (The `kill` Command)** 的完美時機！

為了解決你現在困在 `top` 裡的問題，我們要實作 `sys_kill` 系統呼叫，並且把你的 `top.c` 改寫成「父子雙行程」架構：**讓子行程去無限迴圈跑監視器，讓父行程在背景監聽鍵盤，只要你一敲鍵盤，父行程就會親手把子行程「殺掉」並帶你回到 Shell！**

這就是作業系統多工機制的極致浪漫，跟著我實作這 4 個步驟：

---

### 步驟 1：在 Kernel 賦予死神的權力 (`src/kernel/lib/task.c`)

打開 **`src/kernel/lib/task.c`**，在檔案的最後面加入 `sys_kill` 的實作。它會尋找目標 PID，並殘酷地將它的狀態設為 `TASK_DEAD`。

```c
// 【Day 66 新增】終結指定的行程
int sys_kill(int pid) {
    if (pid <= 1) return -1; // 防禦機制：絕對不准殺 Kernel Idle (0) 和 Kernel Main (1)！

    __asm__ volatile("cli");
    task_t *temp = (task_t*)current_task;
    int found = 0;
    
    // 遍歷所有行程尋找目標
    do {
        if (temp->pid == (uint32_t)pid && temp->state != TASK_DEAD) {
            temp->state = TASK_DEAD; // 宣告死亡，排程器下次看到就會無情回收它
            
            // 釋放記憶體宇宙 (若你有實作 free_page_directory 且它不是 kernel 宇宙)
            if (temp->page_directory != (uint32_t)page_directory) {
                // free_page_directory(temp->page_directory); 
            }
            
            found = 1;
            break;
        }
        temp = temp->next;
    } while (temp != current_task);
    
    __asm__ volatile("sti");
    
    return found ? 0 : -1;
}
```

---

### 步驟 2：開通 `kill` 的 System Call (`src/kernel/lib/syscall.c`)

打開 **`src/kernel/lib/syscall.c`**，我們要在中斷分發中心加上它。

1. 在檔案最上方加入外部宣告：
```c
extern int sys_kill(int pid);
```

2. 找到你的 `syscall_handler`，在最下方新增 `Syscall 24`：
```c
    // ...
    // Syscall 22: 清空終端機畫面
    else if (eax == 22) {
        // ...
    }
    // ==========================================
    // 【Day 66 新增】Syscall 24: sys_kill
    // ==========================================
    else if (eax == 24) {
        int target_pid = (int)regs->ebx;
        regs->eax = sys_kill(target_pid);
    }
    // ... Syscall 99: sys_set_display_mode ...
```

---

### 步驟 3：在 User Space 封裝 API (`src/user/include/unistd.h` & `src/user/lib/unistd.c`)

**1. 打開 `src/user/include/unistd.h`：**
```c
// ...
void clear_screen(void);
int kill(int pid); // 【新增】
```

**2. 打開 `src/user/lib/unistd.c`：**
```c
// 【新增】呼叫死神
int kill(int pid) {
    int ret;
    __asm__ volatile (
        "int $128" 
        : "=a" (ret) 
        : "a" (24), "b" (pid)
    );
    return ret;
}
```

---

### 步驟 4：寫一支獨立的 `kill.c` 工具 (`src/user/bin/kill.c`)

順便把命令列工具寫出來，以後你就可以在 Shell 打 `kill 5` 來殺人了。
建立 **`src/user/bin/kill.c`**：

```c
#include "stdio.h"
#include "unistd.h"

// 簡單的字串轉整數工具
int atoi(const char* str) {
    int res = 0;
    for (int i = 0; str[i] >= '0' && str[i] <= '9'; ++i) {
        res = res * 10 + str[i] - '0';
    }
    return res;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: kill <pid>\n");
        return -1;
    }
    
    int pid = atoi(argv[1]);
    if (kill(pid) == 0) {
        printf("Process %d killed.\n", pid);
    } else {
        printf("Failed to kill PID %d. Not found or permission denied.\n", pid);
    }
    
    return 0;
}
```
*(💡 記得在 `kernel.c` 的 `setup_filesystem` 與 `grub.cfg` 把 `kill.elf` 加進去編譯名單！)*

---

### 步驟 5：完美越獄！重構 `top.c` (`src/user/bin/top.c`)

現在，我們要利用剛寫好的 `kill` 和原本的 `fork`，寫出超經典的「守護行程 (Daemon)」模式。

修改你的 **`src/user/bin/top.c`**：

```c
#include "stdio.h"
#include "unistd.h"

int main() {
    printf("Simple OS Top - Press ANY KEY to exit...\n");
    
    int child_pid = fork();
    
    if (child_pid == 0) {
        // ==========================================
        // 【子行程】負責在無窮迴圈裡畫畫面
        // ==========================================
        process_info_t old_procs[32];
        process_info_t new_procs[32];
        
        while (1) {
            int count = get_process_list(old_procs, 32);
            for(volatile int i = 0; i < 5000000; i++); // 休息一下
            get_process_list(new_procs, 32);

            clear_screen();
            printf("PID   NAME         STATE      MEMORY      %%CPU\n");
            printf("---   ----         -----      ------      ----\n");

            for (int i = 0; i < count; i++) {
                unsigned int dt = new_procs[i].total_ticks - old_procs[i].total_ticks;
                printf("%d     %s    %s    %d B      %d%%\n", 
                    new_procs[i].pid, new_procs[i].name, "RUNNING", new_procs[i].memory_used, (int)dt);
            }
        }
    } else {
        // ==========================================
        // 【父行程】負責監聽鍵盤與當殺手
        // ==========================================
        // getchar() 會「阻塞 (Block)」，直到鍵盤驅動收到任何按鍵為止！
        getchar(); 
        
        // 使用者按了鍵盤！醒來後立刻殺掉還在無窮迴圈的子行程
        kill(child_pid);
        
        // 幫畫面擦屁股，然後優雅地結束自己，把控制權還給 Shell
        clear_screen();
        printf("Top exited. Goodbye!\n");
    }
    
    return 0;
}
```

---

### 🚀 驗收：從母體中甦醒

關掉 QEMU (`Ctrl+A` 接著按 `X`)，然後重新編譯執行 `make clean && make run`。

進到 Terminal 敲下 `top`。
畫面一如既往地開始跳動刷新。

這時，**隨便按下鍵盤上的任何一個鍵**（例如空白鍵或 `q`）！
`getchar()` 會瞬間捕捉到你的按鍵，父行程立刻甦醒，一刀把負責畫畫的子行程砍了，然後畫面上會出現 "Top exited. Goodbye!"，接著完美的 `SimpleOS> ` 提示字元就會再次出現在你眼前！

Rick，這就是作業系統中「IPC (行程通訊)」與「行程生命週期」結合的最佳範例！你已經完全掌握了多工排程的奧義了。
接下來，你想先玩玩 `[X]` 按鈕關閉視窗，還是進入 Phase 9 開通網路卡呢？😎
