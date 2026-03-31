太棒了，Rick！這代表你的行程控制區塊 (PCB) 已經成功換上全新的「身分證」了！🎉

既然 Kernel 裡已經有了所有行程的詳細資料 (PID, PPID, 狀態, 記憶體佔用)，接下來 **Day 63** 的任務，就是要**打造一座橋樑，把這些機密資訊搬運到 User Space！**

因為 User Space 的應用程式（像 `shell.elf`）受到分頁 (Paging) 的保護，絕對看不到 Kernel 的記憶體。所以我們必須開通兩個全新的 **System Calls (系統呼叫)**：
1. `sys_getpid()`: 讓程式知道「我是誰」。
2. `sys_get_process_list()`: 讓程式能「查戶口」，列出系統上所有活著的行程。

請跟著我進行這 3 個步驟，這將為明天的 `ps` 指令打下完美的基礎：

---

### 步驟 1：定義共享的行程資訊結構

我們不能直接把 Kernel 的 `task_t` 指標丟給 User，那樣太危險了。我們要定義一個專門用來「傳遞資訊」的乾淨結構體 `process_info_t`。

因為 Kernel 和 User 是兩個分離的世界，所以這個結構體必須在**兩邊的標頭檔**都宣告一次。

**1. Kernel 端：打開 `src/kernel/include/task.h`，在 `task_t` 之前加入：**
```c
// 【Day 63 新增】用於傳遞給 User Space 的行程資訊
typedef struct {
    uint32_t pid;
    uint32_t ppid;
    char name[32];
    uint32_t state;
    uint32_t memory_used; // 佔用記憶體大小 (Bytes)
} process_info_t;

// ... 並且在檔案最下方新增這個函數宣告：
int task_get_process_list(process_info_t* list, int max_count);
```

**2. User 端：打開 `src/user/include/unistd.h`，加入同樣的結構與 syscall 宣告：**
```c
// 【Day 63 新增】
typedef struct {
    unsigned int pid;
    unsigned int ppid;
    char name[32];
    unsigned int state;
    unsigned int memory_used;
} process_info_t;

// API 宣告
int getpid(void);
int get_process_list(process_info_t* list, int max_count);
```

---

### 步驟 2：在 Kernel 實作資料收集與 Syscall

現在我們要讓 Kernel 去遍歷那條由 `next` 串起來的循環鏈結串列，把資料抄寫到 `process_info_t` 陣列裡。

**1. 打開 `src/kernel/lib/task.c`，在最下方新增這個函數：**
```c
// 【Day 63 新增】收集所有行程資料
int task_get_process_list(process_info_t* list, int max_count) {
    if (!current_task) return 0;
    
    int count = 0;
    task_t* temp = (task_t*)current_task;
    
    // 遍歷 Circular Linked List
    do {
        // 不要回報已經徹底死亡的行程
        if (temp->state != TASK_DEAD) {
            list[count].pid = temp->pid;
            list[count].ppid = temp->ppid;
            strcpy(list[count].name, temp->name);
            list[count].state = temp->state;
            
            // 計算 Heap 佔用的空間 (End - Start)
            // 如果是 Kernel 任務 (沒有 user heap)，就回傳 0
            if (temp->heap_end >= temp->heap_start) {
                list[count].memory_used = temp->heap_end - temp->heap_start;
            } else {
                list[count].memory_used = 0;
            }
            
            count++;
        }
        temp = temp->next;
    } while (temp != current_task && count < max_count);
    
    return count; // 回傳總共找到了幾個行程
}
```

**2. 打開 `src/kernel/lib/syscall.c`，開通中斷 20 和 21：**
找到你的 `syscall_handler`，加入這兩個新的 case：
```c
    // ... 
    // Syscall 19: sys_getcwd
    else if (eax == 19) {
        // ... (保持不變)
    }
    // ==========================================
    // 【Day 63 新增】系統與行程資訊 Syscall
    // ==========================================
    // Syscall 20: sys_getpid
    else if (eax == 20) {
        ipc_lock();
        regs->eax = current_task->pid;
        ipc_unlock();
    }
    // Syscall 21: sys_get_process_list
    else if (eax == 21) {
        process_info_t* list = (process_info_t*)regs->ebx;
        int max_count = (int)regs->ecx;
        
        ipc_lock();
        regs->eax = task_get_process_list(list, max_count);
        ipc_unlock();
    }
    // ... Syscall 99: sys_set_display_mode ...
```

---

### 步驟 3：在 User Space 封裝 API

最後，我們要在 User 的 C Library 中，把這兩個 `int 0x80` (或 128) 的呼叫封裝成好用的 C 函數。

**打開 `src/user/lib/unistd.c`，新增：**
```c
#include "unistd.h"

// 【Day 63 新增】取得自己的 PID
int getpid(void) {
    int ret;
    __asm__ volatile (
        "int $128" 
        : "=a" (ret) 
        : "a" (20)
    );
    return ret;
}

// 【Day 63 新增】取得系統行程列表
int get_process_list(process_info_t* list, int max_count) {
    int ret;
    __asm__ volatile (
        "int $128" 
        : "=a" (ret) 
        : "a" (21), "b" (list), "c" (max_count)
    );
    return ret;
}
```

---

### 💡 Day 63 驗收測試

為了確定這條橋樑是通的，我們先在 `shell.c` 裡面偷偷加一個小測試。

打開 **`src/user/bin/shell.c`**，在 `main` 的前面或某個指令裡加入這段驗證程式碼：
```c
        // 在 shell 的 if-else 迴圈裡加入這個指令測試：
        else if (strcmp(cmd, "mypid") == 0) {
            printf("Shell PID: %d\n", getpid());
        }
```

存檔並編譯 (`make clean && make run`)。
打開系統，在 Terminal 輸入 `mypid`，如果它能正確印出 `Shell PID: 2` (或某個大於 0 的數字)，那就代表我們成功了！

搞定這個之後，明天 **Day 64** 我們就能名正言順地寫出一支獨立的 `ps.elf` 程式，把整個 OS 裡正在運作的父子行程關係、記憶體大小、甚至是 Idle Task，全都一覽無遺地印出來了！😎
