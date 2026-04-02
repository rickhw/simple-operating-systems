哈哈，Rick！這張截圖真的太猛了！🎉🎉🎉

看到 `[v] [_] [O] [X]` 四大天王按鈕整齊地排在每個視窗的右上角，而且底下工作列也完美出現了「System S」的最小化頁籤，這畫面真的讓人感動到起雞皮疙瘩！你現在等於是一手包辦了從最底層的 Bootloader、記憶體管理，一路打通到最上層的 GUI 視窗管理器 (Window Manager)！

既然視窗外框的控制已經趨近完美，我們就正式踏入 **Day 79：User Space UI 元件庫 (SimpleUI Toolkit)！**

回頭看看你寫的 `taskmgr.c`、`explorer.c` 和 `notepad.c`，你會發現我們一直在做重複的苦工：每次寫一個新 App，都要把那一長串的 8x8 `font` 陣列複製貼上，還要重新刻一次 `draw_box` 跟 `draw_text`。這太不符合軟體工程的 DRY (Don't Repeat Yourself) 原則了！

今天，我們要把它們全部抽離出來，做成一個共用的靜態函式庫 `SimpleUI`，並且提供標準的 **Button (按鈕)** 控制項。有了它，以後你要寫新的 GUI 軟體，程式碼會縮短好幾倍！

請跟著我進行這 3 個步驟：

---

### 步驟 1：定義 SimpleUI 標頭檔 (`src/user/include/simpleui.h`)

我們要定義 UI 顏色的巨集、按鈕的結構體，以及公開的繪圖 API。

建立 **`src/user/include/simpleui.h`**：

```c
#ifndef SIMPLEUI_H
#define SIMPLEUI_H

// 標準調色盤
#define UI_COLOR_WHITE      0xFFFFFF
#define UI_COLOR_BLACK      0x000000
#define UI_COLOR_GRAY       0xC0C0C0
#define UI_COLOR_DARK_GRAY  0x808080
#define UI_COLOR_RED        0xFF0000
#define UI_COLOR_BLUE       0x4169E1
#define UI_COLOR_YELLOW     0xFFD700
#define UI_COLOR_GREEN      0x00FF00

// 標準按鈕結構
typedef struct {
    int x, y, w, h;
    char text[32];
    unsigned int bg_color;
    unsigned int text_color;
    int is_pressed; // 記錄按鈕是否正在被按下 (用來畫 3D 凹陷效果)
} ui_button_t;

// 基礎繪圖 API (加上 cw, ch 來避免畫出畫布範圍)
void ui_draw_rect(unsigned int* canvas, int cw, int ch, int x, int y, int w, int h, unsigned int color);
void ui_draw_text(unsigned int* canvas, int cw, int ch, int x, int y, const char* str, unsigned int color);

// 元件 API
void ui_draw_button(unsigned int* canvas, int cw, int ch, ui_button_t* btn);
int ui_is_clicked(ui_button_t* btn, int cx, int cy);

#endif
```

---

### 步驟 2：實作 SimpleUI 函式庫 (`src/user/lib/simpleui.c`)

把那些又臭又長的字型陣列和繪圖邏輯全部塞到這裡！同時加上超有質感的 3D 按鈕繪製邏輯。

建立 **`src/user/lib/simpleui.c`**：
*(💡 這裡我省略了 font 陣列的中間部分，請把你 `taskmgr.c` 裡那組包含完整大小寫的 128x8 字型陣列完整貼到這個檔案裡！)*

```c
#include "simpleui.h"

// ==========================================
// 1. 微型 8x8 字型庫 (請把完整版貼在這裡)
// ==========================================
static const unsigned char font[128][8] = {
    // ... [請貼上你之前在 taskmgr.c 用的完整字型陣列] ...
};

// ==========================================
// 2. 基礎繪圖實作
// ==========================================
void ui_draw_rect(unsigned int* canvas, int cw, int ch, int x, int y, int w, int h, unsigned int color) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            if (x + j >= 0 && x + j < cw && y + i >= 0 && y + i < ch) {
                canvas[(y + i) * cw + (x + j)] = color;
            }
        }
    }
}

void ui_draw_text(unsigned int* canvas, int cw, int ch, int x, int y, const char* str, unsigned int color) {
    while (*str) {
        unsigned char c = *str++;
        if (c >= 'A' && c <= 'Z' && font[c][0] == 0 && font[c][1] == 0) c += 32; // 降級保護
        for (int r = 0; r < 8; r++) {
            for (int c_bit = 0; c_bit < 8; c_bit++) {
                if (font[c][r] & (1 << (7 - c_bit))) {
                    int px = x + c_bit, py = y + r;
                    if (px >= 0 && px < cw && py >= 0 && py < ch) {
                        canvas[py * cw + px] = color;
                    }
                }
            }
        }
        x += 8; 
    }
}

// ==========================================
// 3. 元件實作：立體按鈕 (Button)
// ==========================================
void ui_draw_button(unsigned int* canvas, int cw, int ch, ui_button_t* btn) {
    // 畫按鈕底色
    ui_draw_rect(canvas, cw, ch, btn->x, btn->y, btn->w, btn->h, btn->bg_color);
    
    // 簡單的 3D 邊框邏輯
    unsigned int light = UI_COLOR_WHITE;
    unsigned int dark = UI_COLOR_DARK_GRAY;
    
    if (btn->is_pressed) { 
        // 按下時，亮暗反轉，產生凹陷感！
        light = UI_COLOR_DARK_GRAY; 
        dark = UI_COLOR_WHITE; 
    } 
    
    ui_draw_rect(canvas, cw, ch, btn->x, btn->y, btn->w, 2, light); // 上
    ui_draw_rect(canvas, cw, ch, btn->x, btn->y, 2, btn->h, light); // 左
    ui_draw_rect(canvas, cw, ch, btn->x, btn->y + btn->h - 2, btn->w, 2, dark); // 下
    ui_draw_rect(canvas, cw, ch, btn->x + btn->w - 2, btn->y, 2, btn->h, dark); // 右

    // 計算文字置中
    int text_len = 0;
    while(btn->text[text_len]) text_len++;
    int tx = btn->x + (btn->w - (text_len * 8)) / 2;
    int ty = btn->y + (btn->h - 8) / 2;
    
    // 如果按鈕被按下，文字也跟著往下右偏移 1 像素，增加實體感
    if (btn->is_pressed) { tx += 1; ty += 1; } 
    
    ui_draw_text(canvas, cw, ch, tx, ty, btn->text, btn->text_color);
}

// 碰撞偵測輔助
int ui_is_clicked(ui_button_t* btn, int cx, int cy) {
    return (cx >= btn->x && cx <= btn->x + btn->w && cy >= btn->y && cy <= btn->y + btn->h);
}
```
*(💡 非常重要：記得去你的 **`Makefile`** 裡，把 `src/user/lib/simpleui.o` 加到 User Library 的編譯清單裡！)*

---

### 步驟 3：用 SimpleUI 重構 Task Manager！

現在，我們來見證魔法。我們把 `taskmgr.c` 裡那堆噁心的字型陣列全刪掉，改用 `simpleui.h`。你會發現程式碼變得無比乾淨！

打開 **`src/user/bin/taskmgr.c`**，大刀闊斧地重構：

```c
#include "stdio.h"
#include "unistd.h"
#include "simpleui.h" // 【Day 79】引入我們強大的 UI 框架！

#define CANVAS_W 340
#define CANVAS_H 280

int strlen(const char* s) { int i=0; while(s[i]) i++; return i; }
void itoa(int n, char* buf) { /* 保持原樣 */
    if (n == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    int i = 0; while (n > 0) { buf[i++] = (n % 10) + '0'; n /= 10; }
    buf[i] = '\0';
    for (int j = 0; j < i / 2; j++) { char t = buf[j]; buf[j] = buf[i - 1 - j]; buf[i - 1 - j] = t; }
}

int main() {
    int win_id = create_gui_window("Task Manager", CANVAS_W + 4, CANVAS_H + 24);
    if (win_id < 0) return -1;

    unsigned int* my_canvas = (unsigned int*)sbrk(CANVAS_W * CANVAS_H * 4);
    process_info_t p_list[32]; 
    int refresh_timer = 0;

    while(1) {
        if (refresh_timer % 100 == 0) {
            int p_count = get_process_list(p_list, 32);

            // 使用 SimpleUI 繪圖！
            ui_draw_rect(my_canvas, CANVAS_W, CANVAS_H, 0, 0, CANVAS_W, CANVAS_H, UI_COLOR_WHITE);
            ui_draw_rect(my_canvas, CANVAS_W, CANVAS_H, 0, 0, CANVAS_W, 20, UI_COLOR_BLUE);
            ui_draw_text(my_canvas, CANVAS_W, CANVAS_H, 10, 6, "PID   NAME       MEM (KB)   ACTION", UI_COLOR_WHITE);

            int y_offset = 25;
            for (int i = 0; i < p_count; i++) {
                char buf[16];
                itoa(p_list[i].pid, buf);
                ui_draw_text(my_canvas, CANVAS_W, CANVAS_H, 10, y_offset, buf, UI_COLOR_BLACK);
                ui_draw_text(my_canvas, CANVAS_W, CANVAS_H, 50, y_offset, p_list[i].name, UI_COLOR_BLACK);
                itoa(p_list[i].memory_used / 1024, buf);
                ui_draw_text(my_canvas, CANVAS_W, CANVAS_H, 140, y_offset, buf, UI_COLOR_BLACK);

                // ==========================================
                // 使用 SimpleUI 的立體按鈕元件！
                // ==========================================
                if (p_list[i].pid > 1 && strcmp(p_list[i].name, "taskmgr.elf") != 0) {
                    ui_button_t kill_btn = {
                        .x = 250, .y = y_offset - 2, .w = 48, .h = 14, 
                        .bg_color = UI_COLOR_RED, .text_color = UI_COLOR_WHITE, .is_pressed = 0
                    };
                    // 把 "KILL" 複製進按鈕文字
                    char* src = "KILL"; char* dst = kill_btn.text;
                    while(*src) *dst++ = *src++; *dst = '\0';
                    
                    ui_draw_button(my_canvas, CANVAS_W, CANVAS_H, &kill_btn);
                }

                y_offset += 20;
                if (y_offset > CANVAS_H - 20) break;
            }
            update_gui_window(win_id, my_canvas);
        }
        refresh_timer++;

        // 處理 IPC 點擊事件
        int cx, cy;
        if (get_window_event(win_id, &cx, &cy)) {
            if (cy >= 25) {
                int clicked_row = (cy - 25) / 20;
                
                // 改用按鈕物件的邏輯來檢查碰撞 (這裡簡化為直接套用座標區間)
                ui_button_t dummy_btn = { .x = 250, .y = 25 + clicked_row * 20 - 2, .w = 48, .h = 14 };
                
                if (ui_is_clicked(&dummy_btn, cx, cy)) {
                    int p_count = get_process_list(p_list, 32);
                    if (clicked_row >= 0 && clicked_row < p_count) {
                        int target_pid = p_list[clicked_row].pid;
                        if (target_pid > 1 && strcmp(p_list[clicked_row].name, "taskmgr.elf") != 0) {
                            kill(target_pid);
                            refresh_timer = 0; 
                        }
                    }
                }
            }
        }
        yield(); 
    }
    return 0;
}
```

---

### 🚀 驗收：UI 框架的力量！

現在你的 User Space 擁有自己專屬的 `SimpleUI` 函式庫了！
未來你想開發 `calculator.elf` (計算機) 或是小遊戲，只要 `#include "simpleui.h"`，就能直接叫出按鈕和畫出文字，再也不用複製貼上一大堆底層繪圖碼。

編譯並執行 `make clean && make run`！
打開 `taskmgr`，你會發現右邊的 `KILL` 變成了有**立體 3D 邊框**的專業按鈕，整個 App 看起來精緻度大升級！

如果這個 UI 庫成功上線了，我們 **Day 80** 就可以用它來寫一個作業系統必備的經典小遊戲：「**踩地鼠**」或是「**圈圈叉叉 (Tic-Tac-Toe)**」了！你想挑戰哪個？😎
