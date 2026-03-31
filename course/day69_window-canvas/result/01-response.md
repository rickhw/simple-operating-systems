哈哈，Rick！你遇到了所有圖形介面 (GUI) 開發者都一定會踩到的經典大坑：**「畫家演算法 (Painter's Algorithm) 與 Z-Order 覆蓋問題」**！🎨

其實**你完全可以輸入文字，系統也沒有當機**，只是你「看」不到而已！

### 🔍 案發現場解密：為什麼視窗變成一塊黑磚？

讓我們來看看你 `gui.c` 裡面的 `draw_window_internal` 是怎麼畫圖的：
1. 畫視窗邊框、標題列、`[X]` 按鈕。
2. 畫 "System Status" 的文字。
3. 呼叫 `tty_render_window()` 畫出終端機的文字。
4. **【Day 69 新增】** 畫出獨立畫布 (`win->canvas`)。

發現問題了嗎？😂
我們在 `create_window` 的時候，無差別地給**所有**視窗（包含 Terminal 和 Status）都分配了一塊 `canvas`，並且用 `memset` 把它填滿了 `0`（純黑色）。
然後在畫圖時，你把畫布的渲染放在**最下面（最後畫）**！這導致一個巨大的純黑色矩形，直接「啪」一聲蓋在了你剛剛辛苦畫好的終端機文字和狀態字串上面！

---

### 🛠️ 驅魔指南：重排圖層與節省記憶體

我們要進行兩個修改：第一是把畫布當作「底圖」先畫，第二是幫 Kernel 省點記憶體（原生視窗其實根本不需要畫布）。

請打開 **`src/kernel/lib/gui.c`** 進行以下修改：

#### 修改 1：調整圖層順序 (`draw_window_internal`)
把 `win->canvas` 的渲染移到原生內容的**上方**，讓畫布變成「背景」，而原生文字變成「前景」：

```c
static void draw_window_internal(window_t* win) {
    int is_focused = (focused_window_id == win->id);

    // 1. 視窗主體與邊框 (保持不變)
    draw_rect(win->x, win->y, win->width, win->height, 0xC0C0C0);
    // ... 略 ...
    draw_string(win->x + win->width - 17, win->y + 7, "X", 0x000000, 0xC0C0C0);

    // ==========================================
    // 【修正】把畫布渲染移到「底層」，當作背景！
    // ==========================================
    if (win->canvas != 0) {
        int start_x = win->x + 2;
        int start_y = win->y + 22;
        int canvas_w = win->width - 4;
        int canvas_h = win->height - 24;

        for (int cy = 0; cy < canvas_h; cy++) {
            for (int cx = 0; cx < canvas_w; cx++) {
                uint32_t color = win->canvas[cy * canvas_w + cx];
                draw_rect(start_x + cx, start_y + cy, 1, 1, color);
            }
        }
    }

    // ==========================================
    // 【核心新增】Kernel 原生內容蓋在畫布的最上層！
    // ==========================================
    if (strcmp(win->title, "System Status") == 0) {
        draw_string(win->x + 10, win->y + 30, "CPU: x86 32-bit", 0x000000, 0xC0C0C0);
        draw_string(win->x + 10, win->y + 50, "Memory: 16 MB", 0x000000, 0xC0C0C0);
        draw_string(win->x + 10, win->y + 70, "GUI: Active", 0x000000, 0xC0C0C0);
    }

    // 如果這個視窗有綁定終端機，這行會把它畫出來
    tty_render_window(win->id);
}
```

#### 修改 2：拒絕浪費 Kernel Heap (`create_window`)
Terminal 視窗很大，每次都給它分配一塊沒用到的畫布，會白白吃掉大約 300KB 的 Kernel Heap。我們加個小判斷：**Kernel (PID <= 1) 開的視窗不給畫布！**

```c
int create_window(int x, int y, int width, int height, const char* title, int owner_pid) {
    if (window_count >= MAX_WINDOWS) return -1;
    int id = window_count++;

    windows[id].owner_pid = owner_pid;

    int canvas_w = width - 4;
    int canvas_h = height - 24;
    
    // 【優化】只有 User App (PID > 1) 申請的視窗，才分配畫布，省下大量記憶體！
    if (owner_pid > 1 && canvas_w > 0 && canvas_h > 0) {
        windows[id].canvas = (uint32_t*)kmalloc(canvas_w * canvas_h * 4);
        memset(windows[id].canvas, 0, canvas_w * canvas_h * 4);
    } else {
        windows[id].canvas = 0; // Kernel 原生視窗不需要獨立畫布
    }

    windows[id].id = id;
    // ... 下面保持不變
```

---

### 🚀 驗收：重見光明！

存檔並執行 `make clean && make run`！

這次一進入 Desktop，你的 Terminal 就會完美地顯示 `SimpleOS> ` 提示字元，而且輸入文字絕對順暢無比。點擊 Start Menu 開啟的 "System Status" 也會乖乖露出它的字來！

確認沒問題後，就可以大膽地在 Terminal 輸入 `paint &`，欣賞你的第一支真・圖形化應用程式了！😎 準備好邁向 Day 70 了嗎？
