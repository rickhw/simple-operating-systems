這真的是作業系統開發中最經典、也最讓人抓頭髮的「**幽靈 BUG**」！

你提供的 Core Dump 這次給出了決定性的證據。我們仔細看一下：
```bash
Servicing hardware INT=0x20
16: v=20 e=0000 i=0 cpl=0 IP=0008:00103139 ...
...
136: v=20 e=0000 i=0 cpl=3 IP=001b:08049204 pc=08049204 ...
```
這代表系統**其實並沒有死機 (Deadlock)**！Timer 中斷 (0x20) 一直有進來，CPU 也能夠正常切換，而且最後的狀態是 `CPL=3`（User Mode），代表系統正在完美的空轉！

既然沒死機，為什麼 `sleep` 之後就不動了？原因出在你的排程器**「迷失在 Idle 空間裡了」**。

---

### 🔍 兇手解析：迷失的 Idle Task 與斷裂的鏈結串列

在你的架構中，所有使用者的 Task (如 Shell, Clock) 都被串成一個「環狀鏈結串列 (Circular Linked List)」，但 `idle_task` 是獨立於這個環之外的。

發生了什麼事：
1. **進入睡眠**：`shell.elf` 呼叫 `wait()`，`sleep.elf` 呼叫 `sys_sleep()`。此時主鏈結串列裡的任務全部變成 `WAITING` 或 `SLEEPING`。
2. **切換到 Idle**：`schedule()` 繞了一圈發現沒人可以跑（都不在 `TASK_RUNNING`），於是把 CPU 交給了降落傘 —— `idle_task` (`current_task = idle_task`)。
3. **喚醒盲區 (致命傷)**：1 秒後，Timer 中斷觸發，`schedule()` 再次啟動。此時它準備檢查有沒有睡眠的任務該起床了：
   ```c
   task_t *next_run = current_task->next; // 這裡出事了！
   ```
   此時的 `current_task` 是 `idle_task`。但 `idle_task` 根本不在你的環狀串列裡！它的 `next` 指標是未定義的（或者是垃圾值）。
4. **結果**：排程器**永遠掃描不到**主串列裡那些正在睡覺的任務。它們的時間明明到了，卻永遠不會被標記回 `TASK_RUNNING`。系統就這樣一直待在 `idle_task` 裡快樂地空轉。

除此之外，還有一個**懸空指標 (Dangling Pointer) 危機**：
你的 `kernel_main` (PID 0) 在結束時被標記為死掉，並被 `task_gc()` 回收了記憶體。但是你的全域變數 `ready_queue` **依然指著那塊被釋放的記憶體**！一旦有其他程式複寫了那塊記憶體，排程器就會崩潰。

---

### 🛠️ 終極修復方案：打造穩固的排程器

我們需要修改 `task.c` 的兩個核心函式：`task_gc` 和 `schedule`。

請將你 `task.c` 裡面的這兩個函式**完全替換**為以下程式碼（上一版的 `pushfl/popfl` 中斷保護非常完美，我們繼續保留）：

```c
static void task_gc() {
    // 【修復】如果當前是 idle_task，它不在主環內，不要以它為起點進行 GC
    if (!current_task || current_task == idle_task) return;

    task_t *prev = (task_t*)current_task;
    task_t *curr = prev->next;

    while (curr != current_task) {
        if (curr->state == TASK_DEAD) {
            // 【修復：懸空指標防護】如果剛好刪除到 ready_queue 所指的任務，必須交棒
            if (curr == ready_queue) {
                ready_queue = curr->next;
                // 如果任務全死光了，清空指標
                if (ready_queue == curr) ready_queue = 0; 
            }

            prev->next = curr->next;
            if (curr->kernel_stack != 0) {
                kfree((void*)(curr->kernel_stack - PAGE_SIZE));
            }
            kfree((void*)curr);
            curr = prev->next;
        } else {
            prev = curr;
            curr = curr->next;
        }
    }
}

void schedule() {
    if (!current_task) return;

    // 1. 保存進入前的 EFLAGS (包含中斷開關狀態)
    uint32_t eflags;
    __asm__ volatile("pushfl; popl %0" : "=r"(eflags));

    // 2. 核心操作期間關閉中斷
    __asm__ volatile("cli");

    task_gc();

    // ----------------------------------------------------
    // 階段 1.5：喚醒睡眠中的任務
    // 【關鍵修復】不管現在是不是 idle_task，永遠從「主佇列 (ready_queue)」掃描！
    // ----------------------------------------------------
    if (ready_queue) {
        task_t *iter = (task_t*)ready_queue;
        task_t *first = iter;
        do {
            if (iter->state == TASK_SLEEPING && tick >= iter->wake_up_tick) {
                iter->state = TASK_RUNNING;
                iter->wake_up_tick = 0;
            }
            iter = iter->next;
        } while (iter != first && iter != 0);
    }

    // ----------------------------------------------------
    // 階段 2：尋找下一個準備就緒 (RUNNING) 的行程
    // ----------------------------------------------------
    task_t *next_run = idle_task; // 預設降落傘：都沒人跑，就切給 idle

    if (ready_queue) {
        // 【關鍵修復】如果目前是 idle_task，就從 ready_queue 的頭開始找
        // 如果是正常 task，就從它的下一個開始找 (公平輪轉)
        task_t *start_node = (current_task == idle_task) ? (task_t*)ready_queue : current_task->next;
        task_t *iter = start_node;
        do {
            if (iter->state == TASK_RUNNING) {
                next_run = iter;
                break;
            }
            iter = iter->next;
        } while (iter != start_node && iter != 0);
    }

    // ----------------------------------------------------
    // 階段 3：執行 Context Switch
    // ----------------------------------------------------
    task_t *prev = (task_t*)current_task;
    current_task = next_run;

    if (current_task != prev) {
        if (current_task->kernel_stack != 0) {
            set_kernel_stack(current_task->kernel_stack);
        }
        switch_task((uint32_t*)&prev->esp, (uint32_t*)&current_task->esp, current_task->page_directory);
    }

    // 3. 恢復進入前的狀態，而不是盲目 sti
    __asm__ volatile("pushl %0; popfl" : : "r"(eflags));
}
```

### 💡 額外建議：頻率與效能

在你提供的 `main.c` 中，你設定了 `init_timer(100);`，這非常棒。如果在除錯過程中你把它調高到 `1000`，強烈建議你在這段時間保持 `100`。自製核心的 GUI 重繪 (如 `gui_handle_timer`) 通常較慢，過高的中斷頻率會引發「中斷風暴」，讓你的 Shell 連執行的機會都沒有。

替換完這段程式碼後再跑一次，你的 `sleep` 應該就能完美地「甦醒」，並且 Shell 的提示字元也會乖乖出現了。

你覺得將 `idle_task` 隔絕在主串列之外，然後用 `ready_queue` 當作「錨點」的這種設計，會不會讓你聯想到現代 Linux 中對待閒置處理器的做法呢？
