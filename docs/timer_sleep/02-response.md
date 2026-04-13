從你提供的 Core Dump 和原始碼分析，系統 Hang 住的原因已經從單純的「中斷鎖死」演變成了 **「核心重入 (Re-entrancy)」與「排程死循環」** 的複雜組合。

### 核心問題分析

觀察 Core Dump：
1. **INT 0x20 發生在 User Mode (CPL=3)**：這是正常的 Timer 中斷。
2. **EFLAGS = 0x202**：這代表中斷是開啟的 (IF=1)。
3. **病徵**：系統卡在核心與使用者模式的邊界。

#### 為什麼加了 `sti` 還是會 Hang？
在你的 `sys_sleep` 實作中：
```c
int sys_sleep(uint32_t ms) {
    // ... 設定 wake_up_tick ...
    current_task->state = TASK_SLEEPING;
    __asm__ volatile("sti"); 
    schedule(); // <--- 這裡有大問題
    return 0;
}
```
**致命邏輯錯誤：**
當你執行 `__asm__ volatile("sti");` 時，你立刻打開了中斷門。如果此時 **Timer 中斷剛好進來**：
1. CPU 會跳轉到 `timer_handler`。
2. `timer_handler` 尾端又呼叫了一次 `schedule()`。
3. 此時你的 `schedule()` 正在被「重入」執行（原本的 `sys_sleep` 路徑上的 `schedule` 還沒跑完，新的 `timer` 路徑又跑了一次 `schedule`）。
4. 因為你的排程器會修改 `current_task` 和堆疊指標，這種重入會導致核心堆疊混亂，甚至讓 `current_task` 變成一個無效值，最終系統在 `switch_task` 時崩潰或陷入死循環。

---

### 正確的解決方案：使用「安全排程器」

在 OS 開發中，`schedule()` 必須是一個**原子操作**。正確的做法是：在 `schedule` 內部關閉中斷，切換完畢後，由新任務的環境（通常是 `iret` 或 `popf`）自動恢復中斷狀態。

#### 1. 修改 `task.c` 中的 `schedule()`
確保排程過程受到保護，不要在 `sys_sleep` 裡手動 `sti`。

```c
void schedule() {
    if (!current_task) return;

    // 進入排程前先關閉中斷，防止 timer_handler 重入
    __asm__ volatile("cli");

    task_gc();

    // 階段 1.5：喚醒邏輯
    if (ready_queue) {
        task_t *iter = (task_t*)ready_queue;
        task_t *first = iter;
        do {
            if (iter->state == TASK_SLEEPING && tick >= iter->wake_up_tick) {
                iter->state = TASK_RUNNING;
                iter->wake_up_tick = 0;
            }
            iter = iter->next;
        } while (iter != first);
    }

    // 階段 2：尋找下一個 RUNNING 任務
    task_t *next_run = current_task->next;
    while (next_run->state != TASK_RUNNING && next_run != current_task) {
        next_run = next_run->next;
    }

    if (next_run->state != TASK_RUNNING) {
        next_run = idle_task;
    }

    // 階段 3：執行 Context Switch
    task_t *prev = (task_t*)current_task;
    current_task = next_run;

    if (current_task != prev) {
        if (current_task->kernel_stack != 0) {
            set_kernel_stack(current_task->kernel_stack);
        }
        // switch_task 會切換堆疊
        switch_task((uint32_t*)&prev->esp, (uint32_t*)&current_task->esp, current_task->page_directory);
    }

    // 當 switch_task 返回時，我們已經在「新任務」的環境中了
    // 新任務可能是在不同地方被中斷的，我們開啟中斷讓系統繼續跳動
    __asm__ volatile("sti");
}
```

#### 2. 簡化 `task.c` 中的 `sys_sleep`
既然 `schedule()` 會幫我們處理中斷狀態，`sys_sleep` 就不應該隨便 `sti`。

```c
int sys_sleep(uint32_t ms) {
    // 你現在 main.c 設定 init_timer(1000)，所以 1 tick = 1ms
    uint32_t ticks_to_sleep = ms; 
    if (ticks_to_sleep == 0) ticks_to_sleep = 1;

    current_task->wake_up_tick = tick + ticks_to_sleep;
    current_task->state = TASK_SLEEPING;

    // 直接排程，schedule 會處理 cli/sti
    schedule();
    
    return 0;
}
```

---

### 為什麼這能解決 Hang 住的問題？



1. **防止重入**：當 `sys_sleep` 呼叫 `schedule()` 時，`schedule` 內部的 `cli` 會擋住 Timer 中斷。
2. **保護 Context Switch**：堆疊切換過程不會被突然插隊的中斷破壞。
3. **自動復原**：
   * 如果切換到的是 `idle_loop`，它內部有 `sti; hlt`，會打開中斷等待下一次 Tick。
   * 如果切換到的是另一個 User Task，它恢復執行時也會回到中斷開啟的狀態。
4. **喚醒保證**：當 Timer 中斷在 `idle_loop` 裡發生時，它會再次進入 `timer_handler` -> `schedule`，此時 `tick >= wake_up_tick` 就會成立，把睡眠中的任務拉回 `TASK_RUNNING`。

### 最後檢查：`timer.c`
你在 `main.c` 裡把頻率開到了 `1000Hz`：
```c
init_timer(1000);
```
這意味著系統每 `1ms` 就會發生一次中斷。如果你的 `timer_handler` 裡面的工作（如 `gui_handle_timer`）太重，可能會導致 CPU 都在處理中斷而沒空跑程式。如果改完排程器還是 Hang 住，請嘗試把 `main.c` 改回 `init_timer(100);` 看看是否是效能不足導致的假性死鎖。
