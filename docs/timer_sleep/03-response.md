從你提供的第二次 Core Dump 訊息中，我們可以看到一個非常關鍵的線索：

```bash
# 步驟 134 & 135：EAX=2 (SYS_WAIT 或你的 sleep 編號)，此時中斷是關閉的 (EFL=0x202, 但這是進入核心前的快照)
# 重點在 136 步：
Servicing hardware INT=0x20
136: v=20 e=0000 i=0 cpl=3 IP=001b:08049204 pc=08049204 SP=0023:083ffcdc env->regs[R_EAX]=00000000
EIP=08049204 EFL=00000202
```

**這代表系統其實沒有真的「死鎖」，而是陷入了「空轉」：**
中斷 0x20 (Timer) 確實有進來，但你的任務沒有被正確喚醒。

### 核心問題：`total_ticks` 與 `tick` 的混淆

檢查你的 `timer.c` 裡面的 `timer_handler`：
```c
void timer_handler(void) {
    tick++;  // 這裡增加的是全域變數 tick

    if (current_task != 0) {
        current_task->total_ticks++; // 這裡增加的是 PCB 裡的 total_ticks
    }
    // ...
}
```

再看你的 `task.c` 的 `schedule()`：
```c
if (iter->state == TASK_SLEEPING && tick >= iter->wake_up_tick) {
    iter->state = TASK_RUNNING;
    iter->wake_up_tick = 0;
}
```

這裡隱藏了一個致命的變數作用域問題。雖然你在 `task.c` 開頭包含了 `timer.h` 或宣告了 `extern uint32_t tick;`，但在某些編譯環境下，如果沒有處理好，`task.c` 讀到的 `tick` 可能永遠是 0，或者因為 `volatile` 處理不當導致讀取不到最新值。

但更可能的「炸彈」在於 **`cli/sti` 的不對稱性**。

---

### 徹底修正方案

這是一個具備強健性的 `schedule` 與 `sys_sleep` 組合，請依照此邏輯更新你的 `task.c`：

#### 1. 修正 `schedule()`
目前的 `schedule` 在結尾處強行執行了 `sti`。這是不對的，因為如果 `schedule` 是從 `timer_handler` 進來的，中斷本來就是開啟的（或者是受中斷門保護的）；如果是從 `syscall` 進來的，則不應該隨便破壞呼叫者的中斷狀態。

```c
// task.c

void schedule() {
    if (!current_task) return;

    // 關鍵：保存目前的中斷狀態
    uint32_t eflags;
    __asm__ volatile("pushf; pop %0; cli" : "=r"(eflags));

    task_gc();

    // 階段 1.5：喚醒邏輯 (確保使用正確的全域變數 tick)
    if (ready_queue) {
        task_t *iter = (task_t*)ready_queue;
        task_t *first = iter;
        do {
            // 注意：這裡必須確保外部宣告了 extern volatile uint32_t tick;
            if (iter->state == TASK_SLEEPING && tick >= iter->wake_up_tick) {
                iter->state = TASK_RUNNING;
            }
            iter = iter->next;
        } while (iter != first);
    }

    // 階段 2：尋找下一個 RUNNING
    task_t *next_run = current_task->next;
    while (next_run->state != TASK_RUNNING && next_run != current_task) {
        next_run = next_run->next;
    }

    if (next_run->state != TASK_RUNNING) {
        next_run = idle_task;
    }

    // 階段 3：Context Switch
    task_t *prev = (task_t*)current_task;
    current_task = next_run;

    if (current_task != prev) {
        if (current_task->kernel_stack != 0) {
            set_kernel_stack(current_task->kernel_stack);
        }
        switch_task((uint32_t*)&prev->esp, (uint32_t*)&current_task->esp, current_task->page_directory);
    }

    // 關鍵：恢復原本的中斷狀態，而不是盲目 sti
    __asm__ volatile("push %0; popf" : : "r"(eflags));
}
```

#### 2. 修正 `sys_sleep` (精簡化)
因為 `schedule()` 現在會處理中斷的保存與恢復，`sys_sleep` 只需要負責狀態修改。

```c
int sys_sleep(uint32_t ms) {
    // 假設你在 main.c 設定 init_timer(100)，則 1 tick = 10ms
    // 如果你設定 init_timer(1000)，則 1 tick = 1ms
    // 檢查 main.c 裡你是 init_timer(100);
    uint32_t ticks_to_sleep = ms / 10; 
    if (ticks_to_sleep == 0) ticks_to_sleep = 1;

    current_task->wake_up_tick = tick + ticks_to_sleep;
    current_task->state = TASK_SLEEPING;

    // 進入 schedule，它會 cli 並保存現場，然後切換走
    schedule();
    
    return 0;
}
```

### 為什麼之前會 Hang？ (深度分析)



1. **核心重入死結**：你在 `sys_sleep` 呼叫 `schedule`，而 `timer_handler` 也呼叫 `schedule`。如果兩者重疊，PCB 鏈結串列會被破壞。
2. **`cli` 遺失**：在之前的版本中，如果你在 `sys_sleep` 裡 `sti` 後立刻被 `timer` 中斷，新的 `schedule` 跑完回到 `sys_sleep` 續跑原本的 `schedule` 時，中斷狀態可能會亂掉。
3. **Tick 未更新**：你的 Dump 顯示 `Servicing hardware INT=0x20` 出現了，這表示硬體有發出聲音，但任務沒醒。這通常是因為 `iter->wake_up_tick` 計算錯誤，或者 `tick` 這個全域變數在 `task.c` 裡讀到的是舊的值（優化問題）。

**請檢查這件事：**
在 `task.c` 檔案的最上方，確保 `tick` 的宣告是：
```c
extern volatile uint32_t tick;
```
如果少了 `volatile`，`schedule()` 迴圈裡的 `tick >= iter->wake_up_tick` 可能會被編譯器優化成只讀取一次 `tick` 的值，導致它永遠看不到 `tick` 的增加。

### 最後一個建議：
把 `main.c` 裡的 `init_timer(1000)` 改回 `init_timer(100)`。
每秒 1000 次中斷對一個剛寫好的自製 OS 負擔太重了，尤其是還有 `gui_handle_timer` 在裡面重繪滑鼠，這會消耗大量 CPU 時間，導致 Task 幾乎分不到算力。先用 100Hz 穩定功能，再追求 1ms 的精度。
