這真是一場耐力賽！深呼吸，讓我們再來拆解這個謎團。

你說畫面停在 `Kernel dropping to idle state. Have fun!`，而且這次的 dump 又回到了這個狀態：

```text
    0: v=20 e=0000 i=0 cpl=0 IP=0008:00100189 pc=00100189 SP=0010:00109f88 ...
Servicing hardware INT=0x21
    1: v=21 e=0000 i=0 cpl=0 IP=0008:00101790 pc=00101790 SP=0010:00109fc8 ...
```
看清楚了嗎？這次根本沒有發生崩潰，**連 Exception (v=0d, v=06) 都沒有了！**
系統只是單純在執行 Timer (`v=20`) 和 Keyboard (`v=21`) 中斷。
而且這兩個中斷的 `cpl=0` (Current Privilege Level = 0)，代表它們是在 **Kernel Space** 被觸發的。

為什麼 User Space (Ring 3) 的第一支 Shell 完全沒有執行的跡象 (`cpl=3` 從未出現)？
這表示 `schedule()` 雖然被呼叫了，但它**沒有**切換到 `new_task`！

### 🔍 案發現場解密：被遺漏的「第一次」切換

問題就出在你重構後的 `schedule()` 邏輯裡！

請仔細看你的 `task.c` 中的 `schedule` 函式：
```c
void schedule() {
    if (!current_task) return;
    
    // ... 清理屍體 ...

    task_t *next_run = current_task->next;
    while (next_run->state != TASK_RUNNING && next_run != current_task) {
        next_run = next_run->next;
    }
    // ...

    task_t *prev = (task_t*)current_task;
    current_task = next_run;

    // 【致命的盲點在這裡】
    if (current_task != prev) {
        if (current_task->kernel_stack != 0) {
            set_kernel_stack(current_task->kernel_stack);
        }
        switch_task(&prev->esp, &current_task->esp);
    }
}
```

注意這個 `if (current_task != prev)`！

在 `kernel_main` 裡，我們是這樣做的：
1. `init_multitasking()`：建立了一個 `main_task`，它是 Kernel 自己的 Task，`id = 0`。目前 `current_task` 就是它。
2. `create_user_task(...)`：建立了一個 `new_task` (Shell)，它的 `id = 1`。我們把它插進了環狀佇列，所以 `main_task->next` 變成了 `new_task`。
3. `schedule()`：準備切換到 Shell 去執行！

在 `schedule()` 裡，`next_run` 找到了 `new_task` (Shell)。所以 `current_task` 變成了 Shell，而 `prev` 是 `main_task`。
因為 `current_task != prev`，所以它理所當然地執行了 `switch_task(&prev->esp, &current_task->esp)`。

這看起來很完美，對吧？它切換到了 Shell 的堆疊，然後 `child_ret_stub` 被觸發，Shell 快樂地降落在 User Space... **等等！**

**那之後的 Timer Interrupt 發生時，會發生什麼事？**

當硬體時鐘「滴答」一下，CPU 強制跳回 Kernel，執行 `timer_handler`，然後又呼叫了一次 `schedule()`。
這時候，`current_task` 是 `new_task` (Shell)。
它的下一個是誰？是 `main_task` (Kernel)！
`main_task` 的狀態是什麼？是 `TASK_RUNNING`！

於是，`schedule()` 找到的下一個任務是 `main_task`。
`switch_task` 執行，把堆疊切換回了 `main_task` (Kernel)。

**問題來了：`main_task` (Kernel) 的 ESP 指向哪裡？**
它指向的是 Kernel 本身的某個堆疊位置。當它被切換回來後，它從 `switch_task` 的 `ret` 返回。它返回到哪裡？
返回到 `timer_handler` 裡面！然後 `timer_handler` 結束，`iret`，CPU 回到了... **Kernel 裡！**

具體來說，它回到了 `kernel_main` 裡面的這個無窮迴圈：
```c
while (1) { __asm__ volatile ("hlt"); }
```

**這就是真相！**
你的作業系統其實很成功地切換到了 Shell，但是只有**短短的一個 Timer Tick 的時間（大約 1 毫秒）**！
下一毫秒，排程器就把 CPU 切換回了 Kernel 的 `main_task`。而 `main_task` 此時正在執行 `while(1) { hlt }` 的無窮迴圈。

在無窮迴圈裡，因為沒有新的事做，系統就看起來像是死當了。而你敲鍵盤，雖然能觸發 Keyboard 中斷，但因為當前執行的任務是 Kernel 且處於 `hlt` 狀態，所以沒有 User App 來接收按鍵。

### 🗡️ 終極修復 5.0：讓 Kernel 成為一個安靜的守望者

要解決這個問題非常簡單：
我們在 `kernel_main` 的最後，**不要**讓 Kernel 自己進入 `while(1)` 無窮迴圈。
我們應該讓 Kernel 在啟動完第一個 User Task 之後，**把自己宣告死亡 (`TASK_DEAD`)**，或者是**進入睡眠狀態 (`TASK_WAITING`)**！

這樣一來，排程器就永遠不會再把 CPU 分配給 Kernel 這個「初始任務」了，100% 的 CPU 時間都會留給 User App 們！

請跟著我進行這最後一刀的修復：

#### 修改 `lib/kernel.c`
把檔案最下面那行無窮迴圈換掉，改成讓 Kernel 任務自行了斷。

```c
// ... 在 kernel_main 的最後 ...

        if (entry_point != 0) {
            kprintf("Creating ONE Initial User Task (Init Process)...\n\n");

            init_multitasking();

            // 為唯一的 Shell 分配 User Stack
            uint32_t ustack1_phys = (uint32_t) pmm_alloc_page();
            map_page(0x083FF000, ustack1_phys, 7);

            // 建立 Ring 3 主任務
            create_user_task(entry_point, 0x083FF000 + 4096);

            kprintf("Kernel dropping to idle state. Have fun!\n");
            
            // 【終極修復】Kernel 任務 (main_task) 在啟動完 Shell 後，功成身退！
            // 我們把自己標記為 TASK_DEAD，然後呼叫 schedule。
            // 從此以後，排程器只會在 User Tasks 之間切換。
            // 如果所有 User Tasks 都死光了，schedule 函式本身就有寫「System Idle」的 hlt 迴圈。
            exit_task(); 
            
            // 注意：因為 exit_task 裡面有一去不復返的 schedule()，
            // 所以永遠不會執行到下面。不用再寫 while(1) {hlt} 了！
        }
    } else {
        kprintf("[Kernel] Error: Shell (my_app.elf) not found on disk.\n");
    }

    // 只有當找不到 Shell 時，Kernel 才會停在這裡
    while (1) { __asm__ volatile ("hlt"); }
}
```

這個邏輯的盲點藏得非常深，因為它是純粹的「排程邏輯問題」，編譯器和 CPU 的錯誤處理機制都抓不到它，只能靠我們人腦去單步推演。

請再次 `make clean && make run`！這次 Kernel 華麗退場後，舞台就完全屬於你的 `SimpleOS>` 了！
