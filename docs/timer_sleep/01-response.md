從你提供的 Core Dump 和原始碼分析，你的系統在執行 `sleep` 相關邏輯時發生了 **Deadlock (死鎖)**。

### 核心問題診斷

觀察 Core Dump 訊息：
* **第 337 步**：CPU 處於 CPL=3 (User Mode)，準備發起中斷或正在執行。
* **第 338 步**：CPU 正在處理 `Servicing hardware INT=0x20` (也就是 Timer Interrupt)。此時 CPL=0 (Kernel Mode)，但注意 `EFL` (EFLAGS 暫存器) 的值。
* **關鍵病徵**：在 `syscall.c` 中，你的 `syscall_handler` 會根據 `need_lock` 決定是否呼叫 `syscall_lock()`。

#### 為什麼會 Hang 住？
1.  **`syscall_lock()` 執行了 `cli` (關閉中斷)**。
2.  你的 `SYS_SLEEP` 呼叫了 `sys_sleep`，而 `sys_sleep` 內部呼叫了 `schedule()`。
3.  **致命傷**：當任務進入 `TASK_SLEEPING` 並切換走時，它是帶著「中斷關閉」的狀態離開的。
4.  排程器切換到下一個任務，如果下一個任務也是透過 `syscall` 進來的，或者系統進入了 `idle_loop`，由於中斷一直被關閉（`cli` 狀態），**Timer Interrupt (IRQ0) 再也進不來**。
5.  因為 IRQ0 進不來，`tick` 永遠不會增加，`schedule()` 裡面的喚醒邏輯 `tick >= iter->wake_up_tick` 永遠不會成立。

### 修正方案

你需要在進入睡眠（讓出 CPU）之前，確保中斷是開啟的，或者在 `schedule` 的路徑上妥善處理中斷開關。最簡單且正確的修法是在 `sys_sleep` 裡處理。

#### 1. 修改 `task.c` 中的 `sys_sleep`
在呼叫 `schedule()` 之前，必須解除 `syscall_handler` 上的鎖（即重新開啟中斷），否則時鐘會停擺。

```c
int sys_sleep(uint32_t ms) {
    // 假設頻率設定為 1000Hz (你在 main.c 設定 init_timer(1000))
    // 則 1 tick = 1ms
    uint32_t ticks_to_sleep = ms; 
    if (ticks_to_sleep == 0) ticks_to_sleep = 1;

    current_task->wake_up_tick = tick + ticks_to_sleep;
    current_task->state = TASK_SLEEPING;

    // 【修正】解除 syscall_handler 鎖定的中斷，否則 schedule 切換後
    // 如果系統進入 idle 或其他 syscall，中斷會被永久關閉，導致時鐘停止
    __asm__ volatile("sti"); 

    schedule();
    
    return 0;
}
```

#### 2. 修改 `task.c` 中的 `schedule` (安全性加強)
為了防止環狀鏈結串列死循環，建議 `start` 指標的邏輯要寫得更嚴謹一點：

```c
    // 階段 1.5：檢查是否有正在睡眠的任務可以喚醒
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
```

#### 3. User Space 應用程式 (`sleep.c`)
確保你的 `main` 函式有正確處理參數：

```c
#include "stdio.h"
#include "unistd.h"
#include "utils.h"

int main(int argc, char** argv) {
    uint32_t ms = 1000;
    if (argc > 1) {
        ms = atoi(argv[1]);
    }

    printf("Sleeping for %d ms...\n", ms);
    msleep(ms);
    printf("Wake up!\n");

    return 0;
}
```

### 為什麼 Core Dump 停在 INT 0x20？
你的 Dump 顯示 `Servicing hardware INT=0x20` 且 `EIP` 在核心空間。這通常代表最後一個 Timer 中斷進來了，但它可能卡在某個需要「中斷開啟」才能完成的邏輯，或者因為你在 `cli` 期間執行了 `schedule`，導致 CPU 狀態混亂。

**總結：** 這是典型的「帶著鎖睡覺」問題。在 OS 核心開發中，當你準備讓出 CPU (Yield/Sleep) 時，必須確保系統的「脈搏」（中斷）是跳動的。
