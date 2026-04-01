哈哈，Rick！這個畫面實在太精采了！能在自己寫的 OS 裡面開啟「工作管理員」，看著各個應用程式的記憶體用量，這絕對是最高級的享受！

你點出的這兩個問題，證明了你已經具備了資深 OS 開發者的敏銳度！這兩件事完美揭露了我們系統目前的兩個「偷懶設計」，讓我們來把它們徹底解決：

### 🕵️ 案發現場解密

**問題 1：大小寫混亂 (PId NAMe mEm Kb ACTIoN)**
你猜得完全正確！上一回合我為了節省程式碼行數，只給你了 `A, C, E, I, K, L, N, P, R, S, T, Z` 這幾個字母。當你印出 `MEM` 時，因為 `M` 找不到點陣資料，觸發了我們寫的「自動降級為小寫 `m`」的保護機制，所以就變成了這種有趣的「駭客文」了！😂

**問題 2：殺了 Process，但視窗和殭屍還在 (Ghost Windows & Zombies)**
這是一個經典的 **「子系統脫鉤」** 問題：
1. **Ghost Window**：`sys_kill` 只負責把 Process 的狀態改掉，但它「忘記」打電話給 GUI 引擎說：「喂！這個 PID 死掉了，請把它的視窗收掉！」所以視窗就變成幽靈留在了畫面上。
2. **Zombie Process**：我們之前用 `&` 啟動 `paint`（背景執行），代表 Shell 老爸「沒有」呼叫 `wait()` 來等它。當 `sys_kill` 殺死 `paint` 時，因為沒有老爸來收屍，它就變成了永遠卡在名單上的 `ZOMBIE` 行程。

讓我們用 3 個步驟，打造完美的「垃圾回收與收屍機制」！

---

### 步驟 1：補齊全套大寫字型 (`src/user/bin/taskmgr.c`)

打開 `taskmgr.c`，把大寫字母區段替換成下面這組「全套完整版」，你的標題就會變回正常的全大寫了：

```c
    ['A']={0x3c,0x66,0x66,0x7e,0x66,0x66,0x00}, ['B']={0x7c,0x66,0x7c,0x66,0x66,0x7c,0x00},
    ['C']={0x3c,0x66,0x60,0x60,0x66,0x3c,0x00}, ['D']={0x78,0x6c,0x66,0x66,0x6c,0x78,0x00},
    ['E']={0x7e,0x60,0x7c,0x60,0x60,0x7e,0x00}, ['F']={0x7e,0x60,0x7c,0x60,0x60,0x60,0x00},
    ['G']={0x3c,0x60,0x6e,0x66,0x66,0x3c,0x00}, ['H']={0x66,0x66,0x7e,0x66,0x66,0x66,0x00},
    ['I']={0x3c,0x18,0x18,0x18,0x18,0x3c,0x00}, ['J']={0x06,0x06,0x06,0x06,0x66,0x3c,0x00},
    ['K']={0x66,0x6c,0x78,0x78,0x6c,0x66,0x00}, ['L']={0x60,0x60,0x60,0x60,0x60,0x7e,0x00},
    ['M']={0xc6,0xee,0xfe,0xd6,0xc6,0xc6,0x00}, ['N']={0x66,0x76,0x7e,0x7e,0x6e,0x66,0x00},
    ['O']={0x3c,0x66,0x66,0x66,0x66,0x3c,0x00}, ['P']={0x7c,0x66,0x66,0x7c,0x60,0x60,0x00},
    ['Q']={0x3c,0x66,0x66,0x66,0x6c,0x36,0x00}, ['R']={0x7c,0x66,0x66,0x7c,0x6c,0x66,0x00},
    ['S']={0x3c,0x60,0x3c,0x06,0x66,0x3c,0x00}, ['T']={0x7e,0x18,0x18,0x18,0x18,0x18,0x00},
    ['U']={0x66,0x66,0x66,0x66,0x66,0x3c,0x00}, ['V']={0x66,0x66,0x66,0x66,0x3c,0x18,0x00},
    ['W']={0xc6,0xc6,0xd6,0xfe,0xee,0xc6,0x00}, ['X']={0x66,0x3c,0x18,0x3c,0x66,0x66,0x00},
    ['Y']={0x66,0x66,0x3c,0x18,0x18,0x18,0x00}, ['Z']={0x7e,0x0c,0x18,0x30,0x60,0x7e,0x00},
```

---

### 步驟 2：賦予 GUI 引擎「抄家」的能力 (`src/kernel/lib/gui.c`)

我們要讓 GUI 引擎提供一個 API，只要給它 PID，它就會去掃蕩該 PID 開的所有視窗並強制關閉。

打開 **`src/kernel/lib/gui.c`**，在檔案底部加入這個函式：

```c
// ==========================================
// 【Day 76 修復】根據 PID 關閉該行程擁有的所有視窗
// ==========================================
void gui_close_windows_by_pid(int pid) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_active && windows[i].owner_pid == pid) {
            close_window(windows[i].id);
        }
    }
    gui_dirty = 1; // 標記畫面需要重繪
}
```

---

### 步驟 3：完美死亡機制 (`src/kernel/lib/task.c`)

我們要修改 `sys_kill` 和 `exit_task`。在殺死行程前，先呼叫 GUI 引擎抄家。接著判斷：**如果這個行程的老爸沒有在等它，我們就直接把它設為 `TASK_DEAD` (入土為安)，不要變成 `TASK_ZOMBIE` 佔用名額！**

打開 **`src/kernel/lib/task.c`**：

**1. 修改 `sys_kill`：**
```c
int sys_kill(int pid) {
    if (pid <= 1) return -1; // 防禦機制：絕對不准殺 Kernel Idle 和 Main

    __asm__ volatile("cli");
    task_t *temp = (task_t*)current_task;
    int found = 0;

    do {
        if (temp->pid == (uint32_t)pid && temp->state != TASK_DEAD) {
            
            // 1. 【修復 Ghost Window】呼叫 GUI 引擎，把這個行程的視窗全關了！
            extern void gui_close_windows_by_pid(int pid);
            gui_close_windows_by_pid(pid);

            // 2. 檢查老爸是不是正在等它死？
            task_t *parent = current_task;
            int parent_waiting = 0;
            do {
                if (parent->pid == temp->ppid && parent->state == TASK_WAITING && parent->wait_pid == pid) {
                    parent->state = TASK_RUNNING;
                    parent->wait_pid = 0;
                    parent_waiting = 1;
                    break;
                }
                parent = parent->next;
            } while (parent != current_task);

            // 3. 【修復 Zombie】如果老爸沒在等 (例如背景執行的程式)，直接入土為安！
            if (parent_waiting) {
                temp->state = TASK_ZOMBIE;
            } else {
                temp->state = TASK_DEAD;
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

**2. 順手修改 `exit_task` (讓自己正常退出時也具備相同機制)：**
```c
void exit_task() {
    // 1. 【修復 Ghost Window】程式自己正常退出時，也要收起視窗！
    extern void gui_close_windows_by_pid(int pid);
    gui_close_windows_by_pid(current_task->pid);

    task_t *temp = current_task->next;
    int parent_waiting = 0;
    
    while (temp != current_task) {
        if (temp->state == TASK_WAITING && temp->wait_pid == current_task->pid) {
            temp->state = TASK_RUNNING;
            temp->wait_pid = 0;
            parent_waiting = 1;
        }
        temp = temp->next;
    }

    if (current_task->page_directory != (uint32_t)page_directory) {
        free_page_directory(current_task->page_directory);
    }

    // 2. 【修復 Zombie】如果沒有老爸等著收屍，就直接 DEAD
    if (current_task->ppid == 0 || !parent_waiting) {
        current_task->state = TASK_DEAD;
    } else {
        current_task->state = TASK_ZOMBIE;
    }

    schedule();
}
```

---

### 🚀 驗收：死神降臨！

存檔後 `make clean && make run`！

這次打開 `taskmgr &`，你會發現：
1. 標題列變成了整齊劃一的 `PID  NAME  MEM KB  ACTION`。
2. 當你再次點擊 `paint.elf` 旁邊的 `[KILL]` 按鈕時，**`paint` 的視窗會瞬間消失得無影無蹤**！
3. 再次輸入 `ps` 查看，`paint.elf` 已經徹底從行程名單上被抹除了，沒有留下任何 ZOMBIE！

你的 OS 現在擁有了極度乾淨、不漏記憶體的 Process / GUI 連動生命週期管理！體驗完這個爽快感後，跟我說一聲，我們就可以邁向 Day 77 囉！😎
