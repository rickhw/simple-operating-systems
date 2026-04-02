哈哈，Rick！你的野心真的太棒了！🔥

你現在提出的需求，正是作業系統發展史上最關鍵的分水嶺：**從「作業系統核心 (Kernel)」正式跨足到「桌面環境 (Desktop Environment)」與「UI 框架 (UI Toolkit)」！**



為了實現這個宏大的目標，在架構上我們必須將它分為**兩個獨立的層面**來開發：
1. **Kernel 層 (Window Manager)**：負責視窗「外框」的管理，也就是你說的放大、縮小、置底。
2. **User Space 層 (UI Toolkit)**：負責畫布「內部」的標準元件庫 (Button, TextBox, Scrollbar)，就像 Windows 的 Win32 API 或是 Linux 的 GTK / Qt。

因為這兩個層面的程式碼變動加起來超過好幾百行，我們一口氣做完容易爆炸。
**Day 78，我們先來升級 Kernel 層的視窗管理器！** 我們要為視窗加入 `[v]置底`、`[_]最小化`、`[O]最大化`、`[X]關閉` 四大天王按鈕，並且在工作列 (Taskbar) 實作最小化視窗的還原機制！

請跟著我進行這 4 個步驟，為你的 `gui.c` 進行超級大升級：

---

### 步驟 1：擴充 `window_t` 結構 (`src/kernel/include/gui.h`)

我們要讓 Kernel 記住視窗的大小狀態、原始座標，以及 Z-Order 圖層。

打開 **`src/kernel/include/gui.h`**，在 `window_t` 裡面新增這些變數：
```c
typedef struct {
    // ... 前面的 id, x, y, width, height, title, is_active 等等 ...
    // ... 以及 owner_pid, canvas, last_click_x/y, has_new_click, last_key, has_new_key ...

    // ==========================================
    // 【Day 78 新增】視窗狀態與控制
    // ==========================================
    int is_minimized;  // 1 代表縮小到工作列
    int is_maximized;  // 1 代表最大化
    int orig_x;        // 記憶最大化之前的 X 座標
    int orig_y;        // 記憶最大化之前的 Y 座標
    int orig_w;        // 記憶最大化之前的寬度
    int orig_h;        // 記憶最大化之前的高度
    int z_layer;       // 1 為一般層，0 為置底層
} window_t;
```

---

### 步驟 2：初始化新屬性 (`src/kernel/lib/gui.c`)

打開 **`src/kernel/lib/gui.c`**，找到 `create_window` 函式，在初始化的地方給予預設值：

```c
    // 在 create_window 裡面加入：
    windows[id].is_minimized = 0;
    windows[id].is_maximized = 0;
    windows[id].z_layer = 1; // 預設為一般圖層
```

---

### 步驟 3：渲染四大天王按鈕與工作列 (`gui.c`)

我們要畫出四個按鈕，並且修改 `gui_render` 的迴圈順序，讓 `z_layer == 0` (置底) 的視窗先畫！

**1. 修改 `draw_window_internal`：**
```c
static void draw_window_internal(window_t* win) {
    if (win->is_minimized) return; // 最小化的視窗不畫本體！

    int is_focused = (focused_window_id == win->id);

    // 視窗主體與邊框 (保持你原本的 draw_rect)
    // ...
    // 標題列 (保持你原本的 title_bg 判斷與 draw_string)
    // ...

    // ==========================================
    // 【Day 78 新增】畫出四大天王控制按鈕
    // ==========================================
    int btn_y = win->y + 4;
    int base_x = win->x + win->width;

    // [v] 置底 (Send to Bottom)
    draw_rect(base_x - 68, btn_y, 14, 14, COLOR_WINDOW_BG);
    draw_string(base_x - 66, btn_y + 3, "v", COLOR_BLACK, COLOR_WINDOW_BG);

    // [_] 最小化 (Minimize)
    draw_rect(base_x - 52, btn_y, 14, 14, COLOR_WINDOW_BG);
    draw_string(base_x - 50, btn_y + 3, "_", COLOR_BLACK, COLOR_WINDOW_BG);

    // [O] 最大化 (Maximize)
    draw_rect(base_x - 36, btn_y, 14, 14, COLOR_WINDOW_BG);
    draw_string(base_x - 34, btn_y + 3, "O", COLOR_BLACK, COLOR_WINDOW_BG);

    // [X] 關閉 (Close)
    draw_rect(base_x - 20, btn_y, 14, 14, COLOR_WINDOW_BG);
    draw_string(base_x - 17, btn_y + 3, "X", COLOR_BLACK, COLOR_WINDOW_BG);

    // ... 後面的 canvas 與內容渲染保持不變 ...
}
```

**2. 修改 `draw_taskbar`，把最小化的視窗畫在工作列上：**
```c
static void draw_taskbar(void) {
    // ... (保持你原本的 Start 按鈕和時鐘匣渲染) ...

    // ==========================================
    // 【Day 78 新增】畫出最小化視窗的頁籤
    // ==========================================
    int task_x = 70; // 緊接在 Start 按鈕右邊
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_active && windows[i].is_minimized) {
            // 畫一個凸起的頁籤
            draw_rect(task_x, taskbar_y + 4, 80, 20, COLOR_WINDOW_BG);
            draw_rect(task_x, taskbar_y + 4, 80, 2, COLOR_WHITE);
            draw_rect(task_x, taskbar_y + 4, 2, 20, COLOR_WHITE);
            draw_rect(task_x, taskbar_y + 22, 80, 2, COLOR_DARK_GRAY);
            draw_rect(task_x + 78, taskbar_y + 4, 2, 20, COLOR_DARK_GRAY);
            
            // 印出前幾個字的標題
            char short_title[10];
            strncpy(short_title, windows[i].title, 9);
            short_title[9] = '\0';
            draw_string(task_x + 6, taskbar_y + 10, short_title, COLOR_BLACK, COLOR_WINDOW_BG);
            
            task_x += 84; // 下一個頁籤往右排
        }
    }
}
```

**3. 修改 `gui_render` 的迴圈順序 (實作 Z-Order)：**
```c
    // 1. 畫桌面與圖示
    draw_desktop_background();
    draw_desktop_icons();

    // 2. 先畫「置底層 (z_layer == 0)」的視窗
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_active && windows[i].z_layer == 0 && i != focused_window_id) {
            draw_window_internal(&windows[i]);
        }
    }

    // 3. 再畫「一般層 (z_layer == 1)」的視窗
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_active && windows[i].z_layer == 1 && i != focused_window_id) {
            draw_window_internal(&windows[i]);
        }
    }

    // 4. 最後畫「有焦點」的最上層視窗
    if (focused_window_id != -1 && windows[focused_window_id].is_active) {
        draw_window_internal(&windows[focused_window_id]);
    }
```

---

### 步驟 4：攔截按鈕點擊事件 (`gui.c`)

最後，我們要賦予這四顆按鈕真正的靈魂！

**1. 修改 `handle_window_click`：**
找到原本處理 `[X]` 關閉的地方，替換成下面這段完整的控制邏輯：

```c
            int base_x = win->x + win->width;
            
            // 判斷四顆按鈕的點擊
            if (y >= win->y + 4 && y <= win->y + 18) {
                if (x >= base_x - 20 && x <= base_x - 6) {
                    // 1. [X] 關閉
                    int target_pid = win->owner_pid;
                    close_window(win->id);
                    if (target_pid > 1) { sys_kill(target_pid); }
                } 
                else if (x >= base_x - 36 && x <= base_x - 22) {
                    // 2. [O] 最大化 / 還原
                    if (win->is_maximized) {
                        win->x = win->orig_x; win->y = win->orig_y;
                        win->width = win->orig_w; win->height = win->orig_h;
                        win->is_maximized = 0;
                    } else {
                        win->orig_x = win->x; win->orig_y = win->y;
                        win->orig_w = win->width; win->orig_h = win->height;
                        win->x = 0; win->y = 0;
                        win->width = SCREEN_WIDTH; 
                        win->height = SCREEN_HEIGHT - TASKBAR_HEIGHT;
                        win->is_maximized = 1;
                    }
                }
                else if (x >= base_x - 52 && x <= base_x - 38) {
                    // 3. [_] 最小化
                    win->is_minimized = 1;
                    if (focused_window_id == win->id) focused_window_id = -1; // 取消焦點
                }
                else if (x >= base_x - 68 && x <= base_x - 54) {
                    // 4. [v] 置底
                    win->z_layer = 0; // 丟入置底層
                    if (focused_window_id == win->id) focused_window_id = -1; // 取消焦點，讓它沉下去
                }
            } 
            else if (y >= win->y && y <= win->y + 20) {
                // 如果點擊標題列其他地方，代表使用者想抓取，解除置底狀態！
                win->z_layer = 1; 
                dragged_window_id = win->id;
                drag_offset_x = x - win->x;
                drag_offset_y = y - win->y;
            }
```
*(💡 記得在「擁有焦點」和「背景視窗」的兩個檢查區塊裡，都要替換這段邏輯喔！)*

**2. 修改 `handle_desktop_click`，讓工作列的頁籤可以被點擊還原：**
```c
    // 在 handle_desktop_click 最下方加入：
    
    // 檢查是否點擊了工作列上的最小化頁籤
    int task_x = 70;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_active && windows[i].is_minimized) {
            if (x >= task_x && x <= task_x + 80 && 
                y >= SCREEN_HEIGHT - TASKBAR_HEIGHT + 4 && y <= SCREEN_HEIGHT - 8) {
                
                windows[i].is_minimized = 0; // 解除最小化
                set_focused_window(i);       // 取得焦點
                gui_dirty = 1;
                return 1;
            }
            task_x += 84;
        }
    }
    return 0; // 沒點到任何桌面圖示或工作列
```

---

### 🚀 驗收：現代化視窗管理器誕生！

存檔執行 `make clean && make run`！

打開幾個視窗，盡情嘗試這四個新功能：
* 點擊 `[v]`，視窗會立刻失去焦點並沉到所有視窗的**最底層**！
* 點擊 `[_]`，視窗會消失，並在下方的 Taskbar 變成一個小頁籤，**點擊頁籤**又能瞬間還原！
* 點擊 `[O]`，視窗外框會瞬間放大塞滿整個螢幕！（*註：目前最大化只會放大外框，裡面的 Canvas 會保持原尺寸置左上，未來我們在 User Space UI 框架裡可以實作 `Resize Event` 通知 App 重新申請畫布。*）

這就是 Window Manager 的最高境界！確認這一切都流暢運作後，請告訴我，我們即將在 **Day 79** 正式打造專屬於 User Space 的 **`SimpleUI` 元件庫 (Button & TextBox)**，讓你的 App 開發速度一飛衝天！😎
