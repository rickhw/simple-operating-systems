太棒了，Rick！我們正式進入 **Day 65：實作 `top` 指令與 CPU 使用率計算**。

如果說 `ps` 是幫系統拍一張「靜態快照」，那麼 `top` 就是一個「動態監視器」。實作 `top` 最難的地方不在於畫介面，而在於**計算每個行程到底佔用了 CPU 多少百分比（%CPU）**。

在作業系統的世界裡，我們沒有「時鐘速度百分比」這種東西，我們只有 **Ticks（時鐘滴答）**。



請跟著我進行這 4 個步驟，讓你的 OS 具備即時監控能力：

---

### 步驟 1：Kernel 端的計時器重構 (`src/kernel/include/task.h` & `src/kernel/lib/timer.c`)

我們要讓 Kernel 在每次時鐘中斷發生時，記錄下「這 1 個滴答是誰用掉的」。

**1. 打開 `src/kernel/include/task.h`，在 `task_t` 中加入計時器：**
```c
typedef struct task {
    uint32_t esp;
    uint32_t pid;
    uint32_t ppid;
    char name[32];
    
    // 【Day 65 新增】記錄這個行程總共跑了多少個 Ticks
    uint32_t total_ticks; 
    
    uint32_t page_directory;
    // ... 其他欄位保持不變 ...
} task_t;
```

**2. 打開 `src/kernel/lib/timer.c`，在 `timer_handler` 裡計算功勞：**
```c
#include "task.h"

// 假設這是你的時鐘中斷處理函式 (IRQ 0)
void timer_handler(registers_t *regs) {
    // 1. 全域系統滴答加 1
    system_ticks++;

    // 【核心新增】誰在跑，就加在誰頭上！
    if (current_task) {
        current_task->total_ticks++;
    }

    // 2. 觸發 GUI 渲染 (你之前加的)
    gui_handle_timer();

    // 3. 呼叫排程器
    schedule();
}
```

---

### 步驟 2：更新通訊協議 (`src/kernel/include/task.h` & `src/user/include/unistd.h`)

我們要讓 `total_ticks` 也能透過 `get_process_list` 傳給 User Space。

**1. 修改兩邊的 `process_info_t` 結構，加入 `total_ticks`：**
```c
typedef struct {
    uint32_t pid;
    uint32_t ppid;
    char name[32];
    uint32_t state;
    uint32_t memory_used;
    uint32_t total_ticks; // 【Day 65 新增】
} process_info_t;
```

**2. 修改 `src/kernel/lib/task.c` 中的 `task_get_process_list`：**
```c
        // 在迴圈內加入這一行拷貝邏輯
        list[count].total_ticks = temp->total_ticks;
```

---

### 步驟 3：實作 `top.elf` 應用程式 (`src/user/bin/top.c`)

這是最精彩的部分！`top` 的公式如下：
`%CPU = (行程在 1 秒內的 Tick 增量) / (系統在 1 秒內的總 Tick 增量) * 100`

建立 **`src/user/bin/top.c`**：

```c
#include "stdio.h"
#include "unistd.h"

// 簡單的清螢幕 (假設你的 TTY 支援或直接用大量的 \n 模擬)
void clear_screen() {
    // 這裡我們暫時用暴力法，之後可以實作 ANSI Escape Code
    for(int i = 0; i < 5; i++) printf("\n");
}

int main() {
    process_info_t old_procs[32];
    process_info_t new_procs[32];
    
    printf("Simple OS Top - Press Ctrl+C to stop (if implemented)\n");

    while (1) {
        // 1. 第一波採樣
        int count = get_process_list(old_procs, 32);
        
        // 2. 休息 1 秒 (這裡假設你的 sleep 單位是毫秒，或者你自己寫個迴圈空轉)
        // 註：如果你還沒實作 sleep，先用一個大的 for 迴圈替代
        for(volatile int i = 0; i < 5000000; i++); 

        // 3. 第二波採樣
        get_process_list(new_procs, 32);

        clear_screen();
        printf("PID   NAME         STATE      MEMORY      %%CPU\n");
        printf("---   ----         -----      ------      ----\n");

        for (int i = 0; i < count; i++) {
            // 計算這段時間內增加的 Ticks
            uint32_t dt = new_procs[i].total_ticks - old_procs[i].total_ticks;
            
            // 假設系統頻率是 100Hz，這 1 秒總共有 100 Ticks
            // 這裡簡單把 dt 當成百分比 (這是一個粗略的計算)
            int cpu_usage = dt; 

            printf("%d     %s    %s    %d B      %d%%\n", 
                new_procs[i].pid, 
                new_procs[i].name,
                "RUNNING", // 簡化處理
                new_procs[i].memory_used,
                cpu_usage
            );
        }
    }
    return 0;
}
```

---

### 步驟 4：更新編譯與開機配置

1. 在 **`src/kernel/kernel.c`** 的 `setup_filesystem` 加入 `"top.elf"`。
2. 在 **`scripts/grub.cfg`** 裡加入 `module /boot/top.elf`。

---

### 🚀 驗收：觀看你的系統心跳

執行 `make clean && make run`。
進入 Terminal 後輸入：
```bash
top
```

你將會看到一個每秒更新一次的列表。最酷的地方在於：
* 當你什麼都不做時，`[kernel_idle]` 的 `%CPU` 應該會接近 90% 以上。
* 當你快速移動滑鼠（觸發 GUI 重繪）時，你應該會看到 `[kernel_main]` 或 `shell.elf` 的 `%CPU` 瞬間飆升！

這證明你的排程器確實精準地分配了每一微秒的 CPU 資源。

Rick，恭喜你！你的系統現在不僅能跑程式，還能「自我檢診」了。下一步 **Day 66**，我們要來實作最強大的權力：**`kill` 指令與訊號機制**，讓你可以隨手終結任何不聽話的行程！準備好了嗎？😎
