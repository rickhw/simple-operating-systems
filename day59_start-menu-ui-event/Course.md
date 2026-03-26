太棒了，Rick！🎊 你的作業系統現在正式擁有了現代 GUI 的靈魂：**非同步畫面合成器 (Async Compositor)**！

這代表你的系統已經脫離了「每印一個字就全身抽搐一次」的史前時代，真正達到了 60 FPS 的遊戲級渲染水準。現在你的 `ls` 指令可以瞬間噴出所有文字，而且滑鼠拖曳依然如絲般滑順！

既然系統跑得這麼順暢，**Day 59** 我們要來為這個桌面加入最具代表性的互動元件：**「開始選單 (Start Menu)」與「UI 事件攔截」！**



目前我們的滑鼠點擊，只會傻傻地去判斷「有沒有點到視窗」。如果我們要讓左下角的 `[Start]` 按鈕活過來，我們必須實作一套 **UI 事件分發機制**：當滑鼠點下時，先檢查是不是點到了系統 UI（像是開始選單、工作列），如果不是，才把點擊事件交給下方的視窗。

請跟著我進行這 3 個步驟，讓你的 Start 按鈕真正發揮作用：

---

### 步驟 1：繪製開始選單 (`lib/gui.c`)

請打開 **`lib/gui.c`**，我們要在裡面加入開始選單的狀態與繪圖邏輯。

1. 在全域變數區（`focused_window_id` 附近）新增狀態：
```c
// 【新增】開始選單是否開啟的狀態
static int start_menu_open = 0;
```

2. 實作 `draw_start_menu` 函式（放在 `draw_taskbar` 下面）：
```c
// 【新增】畫出 3D 開始選單
static void draw_start_menu(void) {
    if (!start_menu_open) return;

    int menu_w = 150;
    int menu_h = 100;
    int menu_x = 4;
    int menu_y = 600 - 28 - menu_h; // 貼在工作列正上方 (Y=472)

    // 畫底板與 3D 邊框 (左上亮，右下暗)
    draw_rect(menu_x, menu_y, menu_w, menu_h, 0xC0C0C0);
    draw_rect(menu_x, menu_y, menu_w, 2, 0xFFFFFF);
    draw_rect(menu_x, menu_y, 2, menu_h, 0xFFFFFF);
    draw_rect(menu_x, menu_y + menu_h - 2, menu_w, 2, 0x808080);
    draw_rect(menu_x + menu_w - 2, menu_y, 2, menu_h, 0x808080);

    // 畫選單項目
    draw_string(menu_x + 10, menu_y + 12, "1. Terminal", 0x000000, 0xC0C0C0);
    draw_string(menu_x + 10, menu_y + 42, "2. Sys Status", 0x000000, 0xC0C0C0);
    draw_string(menu_x + 10, menu_y + 72, "3. Shutdown", 0x000000, 0xC0C0C0);
}
```

3. 在 `gui_render` 的最後面，把選單畫上去（要疊在工作列之上，滑鼠之下）：
```c
    // ...
    // 4. 畫工作列 (Taskbar)
    draw_taskbar();

    // 【新增】5. 畫開始選單 (必須疊在所有視窗和工作列上面)
    draw_start_menu();

    // 6. 畫滑鼠游標 (永遠在最最上層)
    draw_cursor(mouse_x, mouse_y);
    // ...
```

---

### 步驟 2：實作 UI 點擊攔截器 (`lib/include/gui.h` & `lib/gui.c`)

我們要寫一個專門檢查「有沒有點到系統 UI」的函式。

1. 打開 **`lib/include/gui.h`**，新增宣告：
```c
// 檢查是否點擊了系統 UI，回傳 1 代表已處理，0 代表未攔截
int gui_check_ui_click(int x, int y); 
```

2. 打開 **`lib/gui.c`**，在最下方加入這段負責處理點擊與啟動 App 的核心邏輯：
```c
// 【核心新增】UI 事件分發中心
int gui_check_ui_click(int x, int y) {
    // 1. 檢查是否點擊了左下角的 Start 按鈕 (X: 4~64, Y: 576~596)
    if (x >= 4 && x <= 64 && y >= 576 && y <= 596) {
        start_menu_open = !start_menu_open; // 切換開關
        gui_dirty = 1;
        return 1; // 成功攔截這次點擊
    }

    // 2. 如果選單是開著的，檢查是否點了裡面的選項
    if (start_menu_open) {
        int menu_y = 600 - 28 - 100; // 472
        
        if (x >= 4 && x <= 154 && y >= menu_y && y <= menu_y + 100) {
            // 點擊 "1. Terminal"
            if (y >= menu_y + 10 && y <= menu_y + 30) {
                // 每次打開新視窗，位置稍微往下移一點
                int offset = window_count * 20; 
                int term_win = create_window(50 + offset, 50 + offset, 368, 228, "Simple OS Terminal");
                terminal_bind_window(term_win); // 將終端機輸入流轉移到新視窗
            }
            // 點擊 "2. Sys Status"
            else if (y >= menu_y + 40 && y <= menu_y + 60) {
                int offset = window_count * 20;
                create_window(450 - offset, 100 + offset, 300, 200, "System Status");
            }
            // 點擊 "3. Shutdown"
            else if (y >= menu_y + 70 && y <= menu_y + 90) {
                draw_rect(0, 0, 800, 600, 0x000000); // 假裝關機畫面
                draw_string(230, 280, "It is now safe to turn off your computer.", 0xFF8000, 0x000000);
                gfx_swap_buffers();
                while(1) __asm__ volatile("cli; hlt"); // 凍結系統
            }
            
            start_menu_open = 0; // 點完選項後自動收起選單
            gui_dirty = 1;
            return 1;
        } else {
            // 點到選單外面的任何地方，就自動收起選單
            start_menu_open = 0;
            gui_dirty = 1;
            // 這裡不 return 1，讓底下的視窗可以繼續處理這個點擊
        }
    }
    
    return 0; // 不是 UI 事件，放行給視窗處理
}
```

---

### 步驟 3：讓滑鼠接上攔截器 (`lib/mouse.c`)

最後，讓滑鼠在檢查視窗之前，先問問 UI 系統要不要攔截這次點擊。

打開 **`lib/mouse.c`**，找到 `if (left_click && !prev_left_click)` 的地方，把它包起來：

```c
                // 【核心互動邏輯】
                if (left_click && !prev_left_click) {
                    // 滑鼠「剛按下的瞬間」
                    
                    // 【新增】先問 GUI 系統有沒有點到 Start 選單或工作列？
                    if (gui_check_ui_click(mouse_x, mouse_y)) {
                        // 如果 return 1，代表 GUI 處理掉了，我們就不要再檢查底下的視窗了！
                    } 
                    else {
                        // 如果 GUI 沒處理，我們才繼續做視窗的 Z-Order 碰撞偵測
                        int clicked_id = -1;
                        int current_focus = get_focused_window();
                        
                        // ... 下方原本的 current_focus 判斷、clicked_id 判斷完全保持不變 ...
                        if (current_focus != -1 && wins[current_focus].is_active) {
                            // ...
```

---

存檔，敲下：
```bash
make clean && make run
```

啟動後，把你的滑鼠移到左下角的 `Start` 按鈕上點下去！
你會看到一個完美的復古選單彈出來！

最有趣的是：
1. 點擊 **1. Terminal**：你的 OS 會立刻生成一個全新的 Terminal 視窗！而且你會發現，你接下來打字的內容，會自動轉移到這個「最新」的視窗裡面（這就是 `terminal_bind_window` 的魅力）！
2. 點擊 **3. Shutdown**：你會看到經典的橘色字體「It is now safe to turn off your computer.」，然後系統完美凍結！

你現在已經做出了一個擁有完整「應用程式啟動器」的桌面環境了！去點擊看看，享受一下當 OS 架構師的快感吧！😎
