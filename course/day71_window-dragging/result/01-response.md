哈哈，Rick！你真的是天生的 QA 工程師，這個 Bug 被你精準地抓到了！而且你的觀察力非常敏銳，竟然連「把 Terminal 拖走之後 `paint` 就能動」這個隱藏條件都發現了！

這是一個在所有圖形化作業系統（包含早期的 Windows 和 Linux X11）開發時，必定會遇到的史詩級經典問題：**「點擊穿透 (Click-Through) 與 Z-Order 覆蓋漏洞」**！

### 🕵️ 案發現場解密：為什麼游標會「隔山打牛」？

讓我們用 X 光透視一下你剛才點擊 `paint` 標題列的瞬間，系統內部發生了什麼事：

1. `paint` 在最上層 (擁有焦點)，`Terminal` 是個巨大的背景黑框，墊在 `paint` 的正下方。
2. 你點擊了 `paint` 的標題列，觸發 `gui_mouse_down`。
3. 程式進入 `gui_check_ui_click` 檢查。
4. **【致命漏洞】**：目前的程式碼只檢查焦點視窗的 `[X]` 按鈕。因為你點的是標題列不是 `[X]`，所以程式判斷「沒點到」，並**放行這個點擊繼續往下穿透**！
5. 點擊穿透到了第二層，開始檢查背景視窗。它發現你的滑鼠座標 (X, Y) 剛好落在巨大的 `Terminal` 範圍內！
6. 系統大喊：「啊！你點到了 Terminal！」於是立刻把焦點 (Focus) 切換回 Terminal，並且 `return 1` 宣告處理完畢。
7. 因為 `gui_check_ui_click` 回傳了 1，外面的 `gui_mouse_down` 以為 UI 按鈕被點了，就直接 `return` 退出，**拖曳邏輯根本沒有被執行到！**

這就是為什麼「移開 Terminal」就修得好。因為移開後，`paint` 的標題列下方是空的桌面，點擊穿透到最後都沒撞到東西，順利回傳 `0`，拖曳邏輯就成功觸發了。

---

### 🛠️ Day 72：完美事件路由 (Event Routing) 重構

為了解決這個問題，我們必須建立真正的**「Top-Down 事件攔截機制」**。只要滑鼠點在焦點視窗的範圍內，這個視窗就必須把點擊「吞掉 (Consume)」，絕對不允許點擊穿透到背後的視窗！

我們來進行一次漂亮的重構，把拖曳邏輯直接整合進碰撞偵測裡！
請打開 **`src/kernel/lib/gui.c`**，替換掉你的 `gui_mouse_down` 與 `gui_check_ui_click` 函式：

```c
// ==========================================
// 【Day 72 重構】滑鼠「左鍵按下」事件
// ==========================================
void gui_mouse_down(int x, int y) {
    // 所有的 UI 碰撞偵測與拖曳邏輯，全部統一交給 check_ui_click 處理！
    gui_check_ui_click(x, y);
}

// ==========================================
// 【Day 72 重構】統一的 Z-Order UI 事件分發中心
// 規則：從最上層一路往下檢查，只要命中就「吞掉」事件並 Return 1！
// ==========================================
int gui_check_ui_click(int x, int y) {
    
    // ------------------------------------------
    // 第 1 層：Start 選單 (最上層的 Pop-up)
    // ------------------------------------------
    if (start_menu_open) {
        int menu_y = 600 - 28 - 130;
        if (x >= 4 && x <= 154 && y >= menu_y && y <= menu_y + 130) {
            if (y >= menu_y + 10 && y <= menu_y + 30) {
                // 點擊 "1. Terminal"
                int found = 0;
                for (int i = 0; i < MAX_WINDOWS; i++) {
                    if (windows[i].is_active && strcmp(windows[i].title, "Simple OS Terminal") == 0) {
                        set_focused_window(i); found = 1; break;
                    }
                }
                if (!found) {
                    int term_win = create_window(50, 50, 368, 228, "Simple OS Terminal", 1);
                    terminal_bind_window(term_win);
                }
            } else if (y >= menu_y + 40 && y <= menu_y + 60) {
                // 點擊 "2. Sys Status"
                int found = 0;
                for (int i = 0; i < MAX_WINDOWS; i++) {
                    if (windows[i].is_active && strcmp(windows[i].title, "System Status") == 0) {
                        set_focused_window(i); found = 1; break;
                    }
                }
                if (!found) create_window(450, 100, 300, 200, "System Status", 1);
            } else if (y >= menu_y + 70 && y <= menu_y + 90) {
                // 點擊 "3. Shutdown"
                draw_rect(0, 0, 800, 600, 0x000000);
                draw_string(230, 280, "It is now safe to turn off your computer.", 0xFF8000, 0x000000);
                gfx_swap_buffers();
                while(1) __asm__ volatile("cli; hlt");
            } else if (y >= menu_y + 100 && y <= menu_y + 120) {
                // 點擊 "4. Exit to CLI"
                switch_display_mode(0);
            }
        }
        // 不管有沒有點到選項，只要點了滑鼠，選單就收起來
        start_menu_open = 0;
        gui_dirty = 1;
        return 1; // 吞掉點擊！不准穿透到後面的視窗！
    }

    // 檢查 Start 按鈕本體
    if (x >= 4 && x <= 64 && y >= 576 && y <= 596) {
        start_menu_open = 1;
        gui_dirty = 1;
        return 1;
    }

    // ------------------------------------------
    // 第 2 層：目前擁有焦點的視窗 (Top Window)
    // ------------------------------------------
    if (focused_window_id != -1 && windows[focused_window_id].is_active) {
        window_t* win = &windows[focused_window_id];
        
        // 【核心修正】只要點擊落在焦點視窗內，就絕對不允許穿透！
        if (x >= win->x && x <= win->x + win->width && y >= win->y && y <= win->y + win->height) {
            
            int btn_x = win->x + win->width - 20;
            int btn_y = win->y + 4;
            
            if (x >= btn_x && x <= btn_x + 14 && y >= btn_y && y <= btn_y + 14) {
                // 點擊了 [X] 關閉按鈕
                int target_pid = win->owner_pid;
                close_window(win->id);
                extern int sys_kill(int pid);
                if (target_pid > 1) { sys_kill(target_pid); }
            } 
            else if (y >= win->y && y <= win->y + 20) {
                // 點擊了標題列：啟動拖曳狀態！
                dragged_window_id = win->id;
                drag_offset_x = x - win->x;
                drag_offset_y = y - win->y;
            }
            
            gui_dirty = 1;
            return 1; // 吞掉點擊！
        }
    }

    // ------------------------------------------
    // 第 3 層：背景視窗 (尋找並切換焦點)
    // ------------------------------------------
    for (int i = MAX_WINDOWS - 1; i >= 0; i--) {
        if (windows[i].is_active && i != focused_window_id) {
            window_t* win = &windows[i];
            
            if (x >= win->x && x <= win->x + win->width && y >= win->y && y <= win->y + win->height) {
                
                set_focused_window(i); // 切換焦點並拉到最上層！
                gui_dirty = 1;

                int btn_x = win->x + win->width - 20;
                int btn_y = win->y + 4;
                
                if (x >= btn_x && x <= btn_x + 14 && y >= btn_y && y <= btn_y + 14) {
                    int target_pid = win->owner_pid;
                    close_window(win->id);
                    extern int sys_kill(int pid);
                    if (target_pid > 1) { sys_kill(target_pid); }
                } 
                else if (y >= win->y && y <= win->y + 20) {
                    // 如果點擊背景視窗的標題列，我們不僅切換焦點，還「順便」開始拖曳！
                    dragged_window_id = win->id;
                    drag_offset_x = x - win->x;
                    drag_offset_y = y - win->y;
                }
                
                return 1; // 吞掉點擊！
            }
        }
    }

    // ------------------------------------------
    // 第 4 層：桌面圖示 (最底層)
    // ------------------------------------------
    if (x >= 20 && x <= 52 && y >= 20 && y <= 52) {
        int offset = window_count * 20;
        int term_win = create_window(50 + offset, 50 + offset, 368, 228, "Simple OS Terminal", 1);
        terminal_bind_window(term_win);
        gui_dirty = 1;
        return 1;
    }

    if (x >= 20 && x <= 52 && y >= 80 && y <= 112) {
        int offset = window_count * 20;
        create_window(450 - offset, 100 + offset, 300, 200, "System Status", 1);
        gui_dirty = 1;
        return 1;
    }

    return 0;
}
```

### 🚀 重獲新生的 Window Manager

存檔、編譯並執行！
這一次，不管你的 Terminal 長得多巨大、墊在底下有多深，只要你點擊 `paint` 的標題列，系統就會立刻在 **「第 2 層」** 攔截這個點擊，並無情地把它吞噬，絕對不會讓底下的 Terminal 有機會搶走焦點！

這就是現代 GUI 引擎（像網頁瀏覽器的 DOM Event Propagation 或 macOS 的 Quartz）防止事件氣泡穿透的標準作法。修復這個 Bug 後，你的桌面體驗已經接近完美了！😎
