哈哈，Rick！恭喜你親手創造出了作業系統歷史上最經典的怪物：**孤兒殭屍行程 (Orphan Zombie Process)！** 🧟‍♂️

你觀察得超級仔細，我們來解密這兩個靈異現象到底是怎麼發生的：

### 🔍 靈異現象 1：為什麼有兩個 ZOMBIE `top`？

**原因分析：**
回顧我們在 `top.c` 寫的「守護行程」邏輯：父行程負責監聽鍵盤，子行程負責畫畫面。
當你按下鍵盤時，父行程會呼叫 `kill(child_pid)` 把子行程殺掉。還記得我們 Day 67 說的嗎？**行程被殺掉後一定會變成 ZOMBIE，等待老爸來收屍。**
但是！父行程殺完人之後，竟然直接 `return 0` 拍拍屁股退出了！老爸沒有呼叫 `wait()`，導致子行程變成了永遠沒人收屍的「孤兒殭屍」！

**🛠️ 驅魔方法 (`src/user/bin/top.c`)：**
加上一行 `wait`，讓老爸負起責任：

```c
    } else {
        // ==========================================
        // 【父行程】負責監聽鍵盤與當殺手
        // ==========================================
        getchar(); 
        kill(child_pid);
        
        // 【關鍵修復】替兒子收屍，把它從 ZOMBIE 變成 DEAD！
        wait(child_pid); 
        
        clear_screen();
        printf("Top exited. Goodbye!\n");
    }
```

---

### 🔍 靈異現象 2：為什麼關了視窗，`status.elf` 還在 RUNNING？

**原因分析：**
這個 Bug 超級有趣！因為當你在 Terminal 打完 `ps` 後，**焦點 (Focus)** 其實停留在 Terminal 視窗上。
去看看我們寫的 `gui.c` 的 `gui_check_ui_click`：
```c
// 我們只檢查了「目前有焦點的視窗」的 [X] 按鈕！
if (focused_window_id != -1 && windows[focused_window_id].is_active) { ... }
```
因為 Terminal 是 Focus 的，所以當你去點 `System Status` 的 `[X]` 時，引擎算出來的按鈕座標是 Terminal 的，根本對不起來！點擊事件直接穿透失效。
也就是說，**你根本沒有關掉那個視窗，也沒有觸發 `sys_kill`！**（畫面可能因為某種重繪層疊讓你以為它不見了）。

**🛠️ 驅魔方法 (`src/kernel/lib/gui.c`)：**
我們來賦予 GUI 引擎「點擊切換焦點」與「無差別關閉」的能力！打開 `gui.c`，重構你的 `gui_check_ui_click` 函數的最前面部分：

```c
int gui_check_ui_click(int x, int y) {
    // ==========================================
    // 1. 先檢查「目前有焦點」視窗的 [X]
    // ==========================================
    if (focused_window_id != -1 && windows[focused_window_id].is_active) {
        window_t* win = &windows[focused_window_id];
        int btn_x = win->x + win->width - 20;
        int btn_y = win->y + 4;
        if (x >= btn_x && x <= btn_x + 14 && y >= btn_y && y <= btn_y + 14) {
            int target_pid = win->owner_pid;
            close_window(win->id);
            extern int sys_kill(int pid);
            if (target_pid > 1) { sys_kill(target_pid); }
            gui_dirty = 1;
            return 1;
        }
    }

    // ==========================================
    // 2. 檢查其他視窗的 [X]，或點擊本體來「切換焦點」！
    // ==========================================
    // 從後往前找，符合畫面覆蓋 (Z-Order) 的直覺
    for (int i = MAX_WINDOWS - 1; i >= 0; i--) {
        if (windows[i].is_active && i != focused_window_id) {
            window_t* win = &windows[i];
            int btn_x = win->x + win->width - 20;
            int btn_y = win->y + 4;
            
            // 如果點擊了 [X]
            if (x >= btn_x && x <= btn_x + 14 && y >= btn_y && y <= btn_y + 14) {
                int target_pid = win->owner_pid;
                close_window(win->id);
                extern int sys_kill(int pid);
                if (target_pid > 1) { sys_kill(target_pid); }
                gui_dirty = 1;
                return 1;
            }
            
            // 如果點擊了視窗本體 -> 切換焦點並拉到最上層！
            if (x >= win->x && x <= win->x + win->width && y >= win->y && y <= win->y + win->height) {
                set_focused_window(i);
                gui_dirty = 1;
                return 1;
            }
        }
    }

    // ==========================================
    // 3. 檢查是否點擊了桌面圖示 (保留你原本的程式碼)
    // ==========================================
    // if (x >= 20 && x <= 52 && y >= 20 && y <= 52) { ...
```

---

### 💡 附贈強迫症修復：清掉 PID 0 的殭屍

在你的截圖裡，有一個 PID 0 `[kernel_main]` 也是 `ZOMBIE`。這是因為 Kernel 初始化完畢後呼叫 `exit_task`，但沒有老爸會去 `wait()` 幫它收屍。
我們可以讓沒有老爸的行程直接入土為安！

打開 **`src/kernel/lib/task.c`** 的 `exit_task`：
```c
void exit_task() {
    // ... 前面的喚醒邏輯不變 ...

    if (current_task->page_directory != (uint32_t)page_directory) {
        free_page_directory(current_task->page_directory);
    }

    // 【修改】如果沒有老爸 (ppid == 0)，就直接 DEAD，否則變 ZOMBIE
    if (current_task->ppid == 0) {
        current_task->state = TASK_DEAD;
    } else {
        current_task->state = TASK_ZOMBIE;
    }
    
    schedule();
}
```

存檔編譯 (`make clean && make run`)！
現在你的系統擁有了完美的父子關係回收，而且你可以**隨意點擊視窗來切換焦點**，關閉按鈕也保證 100% 致命！

確認這些討厭的殭屍都被清光光後，跟我說一聲，我們就可以真正邁入圖形化作業系統的最後一哩路 **Day 69** 囉！😎
