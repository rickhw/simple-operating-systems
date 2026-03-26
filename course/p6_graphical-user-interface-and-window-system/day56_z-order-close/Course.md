哈哈，Rick！看到這張截圖，真的是太完美了！🎉

你現在看到的，是一個**完全沒有閃爍 (Zero Flickering)**、而且**不會破壞背景 (Non-destructive)** 的現代圖形介面！那種如絲般滑順的拖曳體驗，就是「雙重緩衝 (Double Buffering)」與「遊戲渲染迴圈 (Render Loop)」發揮威力的最佳證明。

你的 Simple OS 已經有了視窗，但你可能已經發現了一個小問題：**當兩個視窗疊在一起時，看起來有點呆板，而且點擊被壓在下面的視窗時，它不會跑到最前面來！** 還有，打開了視窗卻關不掉，這太不符合人體工學了！

今天 **Day 56** 的任務，我們就要來實作 GUI 系統最重要的兩個互動機制：
1. **Z-Order (視窗焦點與深度)**：點擊視窗讓它浮到最上層，且標題列顏色要跟著改變。
2. **關閉按鈕 (`[X]`)**：實作按鈕的點擊偵測，讓視窗可以被關閉！

請跟著我進行這 3 個步驟的重構：

---

### 步驟 1：升級 GUI API (`lib/include/gui.h`)

我們需要新增管理「焦點」與「關閉」的函式。請打開 **`lib/include/gui.h`**，在裡面加上：

```c
#ifndef GUI_H
#define GUI_H

#include <stdint.h>

typedef struct {
    int id;
    int x;
    int y;
    int width;
    int height;
    char title[32];
    int is_active;
} window_t;

void init_gui(void);
int create_window(int x, int y, int width, int height, const char* title);
void gui_render(int mouse_x, int mouse_y);
window_t* get_windows(void);

// 【Day 56 新增】焦點與關閉控制
void set_focused_window(int id);
int get_focused_window(void);
void close_window(int id);

#endif
```

---

### 步驟 2：實作 Z-Order 與畫出 `[X]` 按鈕 (`lib/gui.c`)

在 `gui.c` 中，我們要改變 `gui_render` 的畫圖順序（**先畫沒被選中的，最後才畫被選中的，這樣它才會疊在最上面！**），並在標題列畫出一個可愛的 `[X]` 按鈕。

請打開 **`lib/gui.c`**，替換/新增以下內容：

```c
#include "gui.h"
#include "gfx.h"
#include "utils.h"
#include "tty.h"

#define MAX_WINDOWS 10
#define TERM_BG 0x008080 

static window_t windows[MAX_WINDOWS];
static int window_count = 0;

// 【新增】記錄當前被選中的視窗 (-1 代表沒有)
static int focused_window_id = -1;

void init_gui(void) {
    for (int i = 0; i < MAX_WINDOWS; i++) windows[i].is_active = 0;
    focused_window_id = -1;
}

int create_window(int x, int y, int width, int height, const char* title) {
    if (window_count >= MAX_WINDOWS) return -1;
    int id = window_count++;
    windows[id].id = id;
    windows[id].x = x;
    windows[id].y = y;
    windows[id].width = width;
    windows[id].height = height;
    strcpy(windows[id].title, title);
    windows[id].is_active = 1;
    
    focused_window_id = id; // 新開的視窗預設取得焦點
    return id;
}

window_t* get_windows(void) { return windows; }

// 【新增】API 實作
void set_focused_window(int id) { focused_window_id = id; }
int get_focused_window(void) { return focused_window_id; }
void close_window(int id) {
    if (id >= 0 && id < MAX_WINDOWS) {
        windows[id].is_active = 0;
        if (focused_window_id == id) focused_window_id = -1;
    }
}

// 【修改】加入 Focus 判斷與 [X] 按鈕
static void draw_window_internal(window_t* win) {
    int is_focused = (focused_window_id == win->id);
    
    // 視窗主體與邊框
    draw_rect(win->x, win->y, win->width, win->height, 0xC0C0C0);
    draw_rect(win->x, win->y, win->width, 2, 0xFFFFFF); 
    draw_rect(win->x, win->y, 2, win->height, 0xFFFFFF);
    draw_rect(win->x, win->y + win->height - 2, win->width, 2, 0x808080);
    draw_rect(win->x + win->width - 2, win->y, 2, win->height, 0x808080);

    // 標題列 (有焦點是深藍色，沒焦點是灰色)
    uint32_t title_bg = is_focused ? 0x000080 : 0x808080;
    draw_rect(win->x + 2, win->y + 2, win->width - 4, 18, title_bg);
    draw_string(win->x + 6, win->y + 7, win->title, 0xFFFFFF, title_bg);

    // 【新增】畫出 [X] 關閉按鈕 (在右上角)
    draw_rect(win->x + win->width - 20, win->y + 4, 14, 14, 0xC0C0C0);
    draw_string(win->x + win->width - 17, win->y + 7, "X", 0x000000, 0xC0C0C0);
}

void gui_render(int mouse_x, int mouse_y) {
    draw_rect(0, 0, 800, 600, TERM_BG);

    // 1. 先畫「沒有焦點」的活躍視窗 (讓它們在底層)
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_active && i != focused_window_id) {
            draw_window_internal(&windows[i]);
        }
    }

    // 2. 最後畫「有焦點」的視窗 (讓它疊在最上層！)
    if (focused_window_id != -1 && windows[focused_window_id].is_active) {
        draw_window_internal(&windows[focused_window_id]);
        
        // 示範內容 (畫在 focused 的視窗裡，如果是視窗 0 的話)
        if (focused_window_id == 0) {
            draw_string(windows[0].x + 10, windows[0].y + 30, "CPU: x86 32-bit", 0x000000, 0xC0C0C0);
            draw_string(windows[0].x + 10, windows[0].y + 50, "Memory: 16 MB", 0x000000, 0xC0C0C0);
            draw_string(windows[0].x + 10, windows[0].y + 70, "GUI: Active", 0x000000, 0xC0C0C0);
        }
    }

    tty_render();
    draw_cursor(mouse_x, mouse_y);
    gfx_swap_buffers();
}
```

---

### 步驟 3：精準的滑鼠點擊偵測 (`lib/mouse.c`)

現在要來處理最核心的互動邏輯：我們需要區分「剛按下的瞬間 (Edge Detection)」，判斷按到了 `[X]` 還是標題列！

請打開 **`lib/mouse.c`**，替換 `mouse_handler` 的 `case 2:` 區塊：

```c
#include "gui.h"

static int dragged_window_id = -1; 
static int prev_left_click = 0; // 【新增】記錄上一次的左鍵狀態，用來偵測「剛按下」的瞬間

void mouse_handler(void) {
    uint8_t status = inb(0x64);
    while (status & 0x01) { 
        int8_t mouse_in = inb(0x60);
        
        switch (mouse_cycle) {
            case 0:
                if (mouse_in & 0x08) { mouse_byte[0] = mouse_in; mouse_cycle++; }
                break;
            case 1:
                mouse_byte[1] = mouse_in; mouse_cycle++; break;
            case 2:
                mouse_byte[2] = mouse_in; mouse_cycle = 0;

                mouse_x += mouse_byte[1];
                mouse_y -= mouse_byte[2];
                if (mouse_x < 0) mouse_x = 0;
                if (mouse_x > 790) mouse_x = 790;
                if (mouse_y < 0) mouse_y = 0;
                if (mouse_y > 590) mouse_y = 590;

                int left_click = mouse_byte[0] & 0x01;
                window_t* wins = get_windows();

                // 【核心互動邏輯】
                if (left_click && !prev_left_click) {
                    // 滑鼠「剛按下的瞬間」
                    int clicked_id = -1;

                    // 為了符合 Z-Order，我們應該「倒過來」檢查，先檢查最上層的 (也就是 Focused 視窗)
                    int current_focus = get_focused_window();
                    if (current_focus != -1 && wins[current_focus].is_active) {
                        if (mouse_x >= wins[current_focus].x && mouse_x <= wins[current_focus].x + wins[current_focus].width &&
                            mouse_y >= wins[current_focus].y && mouse_y <= wins[current_focus].y + wins[current_focus].height) {
                            clicked_id = current_focus;
                        }
                    }

                    // 如果最上層沒點到，再檢查其他視窗
                    if (clicked_id == -1) {
                        for (int i = 0; i < 10; i++) {
                            if (wins[i].is_active && i != current_focus) {
                                if (mouse_x >= wins[i].x && mouse_x <= wins[i].x + wins[i].width &&
                                    mouse_y >= wins[i].y && mouse_y <= wins[i].y + wins[i].height) {
                                    clicked_id = i;
                                    break;
                                }
                            }
                        }
                    }

                    // 如果真的點到了某個視窗
                    if (clicked_id != -1) {
                        set_focused_window(clicked_id); // 將它拉到最上層！

                        // 判斷是否點到了右上角的 [X] 按鈕
                        if (mouse_x >= wins[clicked_id].x + wins[clicked_id].width - 20 &&
                            mouse_x <= wins[clicked_id].x + wins[clicked_id].width - 6 &&
                            mouse_y >= wins[clicked_id].y + 4 && mouse_y <= wins[clicked_id].y + 18) {
                            close_window(clicked_id); // 關閉視窗！
                        } 
                        // 判斷是否點到了標題列，準備拖曳
                        else if (mouse_y >= wins[clicked_id].y && mouse_y <= wins[clicked_id].y + 20) {
                            dragged_window_id = clicked_id;
                        }
                    }
                } 
                else if (left_click && prev_left_click) {
                    // 滑鼠「持續按住」中
                    if (dragged_window_id != -1) {
                        wins[dragged_window_id].x += mouse_byte[1];
                        wins[dragged_window_id].y -= mouse_byte[2];
                    }
                } 
                else {
                    // 左鍵放開
                    dragged_window_id = -1;
                }

                prev_left_click = left_click; // 更新狀態紀錄
                gui_render(mouse_x, mouse_y);
                break;
        }
        status = inb(0x64); 
    }
    outb(0xA0, 0x20); 
    outb(0x20, 0x20); 
}
```

---

存檔，敲下：
```bash
make clean && make run
```

這一次的體驗會非常有感：
1. **疊疊樂**：把兩個視窗拖在一起，然後輪流點擊它們。你會發現點到的視窗會瞬間跑到最上層，而且標題列會變成深藍色，另一個會變成灰色！
2. **無情關閉**：點擊視窗右上角的 `[X]`，視窗會瞬間消失在桌面上！

你現在已經完全掌握了 GUI 作業系統的視窗生命週期！測試成功後，你覺得下一個目標，是把左上角的終端機文字「搬進」 Terminal 視窗裡面，還是想挑戰在桌面上畫個背景圖呢？😎
