哈哈，Rick！你這個要求非常自然，這也是作業系統介面開發的必經之路！

現在我們的文字是「全域」畫在螢幕上的（直接貼在背景上）。你昨天可能已經發現了一個問題：**如果把 `System Status` 視窗拖曳到左上角的文字上，文字會直接「穿透」視窗浮在最上面！**

這是因為我們在 `gui_render()` 的最後一步才呼叫了 `tty_render()`。在圖學中，這代表文字的 **Z-Order (深度)** 永遠是最高的。

為了解決這個問題，並將終端機真正「關進」視窗裡，我們在 **Day 57** 要做三件事：
1. **把 TTY 座標從「像素」改成「網格 (Grid)」**，方便在視窗內計算。
2. **實作視窗綁定 (Window Binding)**，讓 TTY 知道它屬於哪個視窗。
3. **把內容渲染的時機，移到「視窗渲染」的當下**，這樣文字就會跟著該視窗的 Z-Order 走！

請跟著我進行這 4 個步驟的重構：

---

### 步驟 1：為 TTY 開通取得視窗資訊的權限 (`lib/include/gui.h` & `lib/gui.c`)

為了讓 TTY 能知道視窗被拖到哪裡了，GUI 必須提供一個查詢 API。

1. 打開 **`lib/include/gui.h`**，新增這行宣告：
```c
// ... 其他宣告
window_t* get_window(int id); 
#endif
```

2. 打開 **`lib/gui.c`**，新增實作，並把「畫內容」的邏輯搬進 `draw_window_internal` 裡：
```c
// 【新增】提供給其他系統取得視窗狀態的 API
window_t* get_window(int id) {
    if (id >= 0 && id < MAX_WINDOWS && windows[id].is_active) {
        return &windows[id];
    }
    return 0; // NULL
}

// 宣告 TTY 的渲染函式
extern void tty_render_window(int win_id);

// 【修改】將畫「視窗內容」的責任，綁定在視窗渲染的當下！
static void draw_window_internal(window_t* win) {
    int is_focused = (focused_window_id == win->id);
    
    // ... 畫主體、邊框、標題列、[X] 按鈕 (保留你昨天的程式碼) ...
    draw_rect(win->x, win->y, win->width, win->height, 0xC0C0C0);
    draw_rect(win->x, win->y, win->width, 2, 0xFFFFFF); 
    draw_rect(win->x, win->y, 2, win->height, 0xFFFFFF);
    draw_rect(win->x, win->y + win->height - 2, win->width, 2, 0x808080);
    draw_rect(win->x + win->width - 2, win->y, 2, win->height, 0x808080);

    uint32_t title_bg = is_focused ? 0x000080 : 0x808080;
    draw_rect(win->x + 2, win->y + 2, win->width - 4, 18, title_bg);
    draw_string(win->x + 6, win->y + 7, win->title, 0xFFFFFF, title_bg);

    draw_rect(win->x + win->width - 20, win->y + 4, 14, 14, 0xC0C0C0);
    draw_string(win->x + win->width - 17, win->y + 7, "X", 0x000000, 0xC0C0C0);

    // ==========================================
    // 【核心新增】在這裡渲染視窗專屬的內容！
    // 這樣內容就會完美服從視窗的 Z-Order，不會穿透了！
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
*(⚠️ 記得去 `gui_render` 裡面，把原本呼叫 `tty_render();` 還有寫死 System Status 文字的程式碼都刪掉！因為它們都搬到 `draw_window_internal` 裡了！)*

---

### 步驟 2：更新 TTY 標頭檔 (`lib/include/tty.h`)

打開 **`lib/include/tty.h`**，把原本的 `tty_render` 替換掉，加入綁定視窗的 API：
```c
// 刪掉 void tty_render(void);
void terminal_bind_window(int win_id);
void tty_render_window(int win_id);
```

---

### 步驟 3：把終端機關進視窗裡 (`lib/tty.c`)

我們要把 TTY 從管理「像素座標」改成管理「網格行列」，這樣很容易塞進任何尺寸的視窗。

請打開 **`lib/tty.c`**，替換為以下實作：

```c
#include <stdint.h>
#include <stddef.h>
#include "tty.h"
#include "utils.h"
#include "gfx.h"
#include "gui.h" // 為了呼叫 get_window

#define TERM_FG 0xFFFFFF
#define TERM_BG 0x000000

// 【修改】改成網格座標！假設我們終端機視窗要放 45 個字元寬、25 行高
#define MAX_COLS 45 
#define MAX_ROWS 25 

static int term_col = 0; // 目前在第幾直行
static int term_row = 0; // 目前在第幾橫列
static char text_buffer[MAX_ROWS][MAX_COLS];

// 記錄終端機被綁定到哪一個 GUI 視窗 ID
static int bound_win_id = -1;

void terminal_bind_window(int win_id) {
    bound_win_id = win_id;
}

void terminal_initialize(void) {
    term_col = 0;
    term_row = 0;
    for (int r = 0; r < MAX_ROWS; r++) {
        for (int c = 0; c < MAX_COLS; c++) {
            text_buffer[r][c] = '\0';
        }
    }
}

void terminal_putchar(char c) {
    if (c == '\n') {
        term_col = 0;
        term_row++; 
        if (term_row >= MAX_ROWS) terminal_initialize(); 
        return;
    }

    if (c == '\b') {
        if (term_col > 0) {
            term_col--;
            text_buffer[term_row][term_col] = '\0'; 
        }
        return;
    }

    // 存入記憶矩陣
    text_buffer[term_row][term_col] = c;
    term_col++;

    // 自動換行
    if (term_col >= MAX_COLS) {
        term_col = 0;
        term_row++;
        if (term_row >= MAX_ROWS) terminal_initialize();
    }
}

// (terminal_write, terminal_writestring, kprintf 保持不變)

// 【核心新增】只在綁定的視窗被繪製時，才渲染文字！
void tty_render_window(int win_id) {
    if (bound_win_id == -1 || win_id != bound_win_id) return;

    window_t* win = get_window(win_id);
    if (win == 0) return; // 視窗已經被關閉了就不畫

    // 算出內容的起始像素座標 (避開標題列 20px 與上左邊框)
    int start_x = win->x + 4;
    int start_y = win->y + 24;

    // 畫出終端機的黑色背景底板
    draw_rect(start_x, start_y, MAX_COLS * 8, MAX_ROWS * 8, TERM_BG);

    // 把文字畫上去！
    for (int r = 0; r < MAX_ROWS; r++) {
        for (int c = 0; c < MAX_COLS; c++) {
            char ch = text_buffer[r][c];
            if (ch != '\0' && ch != ' ') {
                draw_char_transparent(ch, start_x + c * 8, start_y + r * 8, TERM_FG);
            }
        }
    }
    
    // 畫一個閃爍的底線游標
    draw_rect(start_x + term_col * 8, start_y + term_row * 8 + 6, 8, 2, TERM_FG);
}
```

---

### 步驟 4：由 Kernel 綁定視窗 (`lib/kernel.c`)

最後，我們在開機時，建立一個大小剛剛好的 Terminal 視窗，並把 TTY 系統塞進去。

打開 **`lib/kernel.c`**，修改 `kernel_main` 中建立視窗的部分：

```c
    // ...
    init_gfx(mbd);
    
    init_gui();
    
    // 建立狀態視窗
    create_window(450, 100, 300, 200, "System Status");
    
    // 建立終端機視窗 (寬度: 45個字*8 + 左右邊框8 = 368，高度: 25行*8 + 標題列/邊框28 = 228)
    int term_win = create_window(50, 50, 368, 228, "Simple OS Terminal"); 
    
    // 【關鍵綁定】告訴 TTY 系統，請把文字印在這個視窗裡面！
    terminal_bind_window(term_win);
    
    gui_render(400, 300);
    init_mouse();
    
    __asm__ volatile ("sti");

    // 現在這些字，會自動出現在 "Simple OS Terminal" 視窗裡了！
    kprintf("=== OS Subsystems Ready ===\n\n");
    kprintf("Welcome to Simple OS Desktop Environment!\n");

    while (1) { __asm__ volatile ("hlt"); }
```

存檔，敲下：
```bash
make clean && make run
```

當系統啟動後，嘗試拖曳那兩個視窗，並把它們互相疊加看看！
你會發現 `kprintf` 的文字完美地被限制在 Terminal 視窗的黑色區域內，而且當 System Status 視窗蓋過它時，文字會乖乖地被遮住，Z-Order 完美無缺！

恭喜你！你的系統現在擁有了一個功能完整的 GUI Terminal！😎
