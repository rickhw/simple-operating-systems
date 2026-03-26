哈哈，美化的部分看起來可以，

不過 terminal 在顯示文字還是 FPS=10 的速度

附上現在 code:

---
tty.c
```c
#include <stdint.h>
#include <stddef.h>
#include "tty.h"
#include "utils.h"
#include "gfx.h" // 引入我們強大的圖形引擎
#include "gui.h" // 為了呼叫 get_window

// 設定終端機的字體顏色為白色 (0xFFFFFF)，背景為純黑 (0x000000)
#define TERM_FG 0xFFFFFF
#define TERM_BG 0x008080    // 把這行原本的黑色換成深青色 (Teal)

// 假設解析度是 800x600 (你可以未來把它變成從 gfx.h 讀取)
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600
#define FONT_WIDTH    8
#define FONT_HEIGHT   8

// 【修改】改成網格座標！假設我們終端機視窗要放 45 個字元寬、25 行高
#define MAX_COLS 45
#define MAX_ROWS 25

// 記錄目前打字游標的 (X, Y) 像素座標
static int term_col = 0; // 目前在第幾直行
static int term_row = 0; // 目前在第幾橫列
static char text_buffer[MAX_ROWS][MAX_COLS];
// 記錄終端機被綁定到哪一個 GUI 視窗 ID
static int bound_win_id = -1;

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

void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        terminal_putchar(data[i]);
    }
    // 【關鍵優化】整串字串都排進 text_buffer 後，再一次性重繪！
    gui_redraw();
}

void terminal_writestring(const char* data) {
    terminal_write(data, strlen(data));
}


void terminal_bind_window(int win_id) {
    bound_win_id = win_id;
}

// 【核心修改】只在綁定的視窗被繪製時，才渲染文字！
void tty_render_window(int win_id) {
    if (bound_win_id == -1 || win_id != bound_win_id) return;

    window_t* win = get_window(win_id);
    if (win == 0) return;

    // 1. 強制畫出純黑色的內部畫布 (避開外框與標題列)
    draw_rect(win->x + 4, win->y + 22, win->width - 8, win->height - 26, 0x000000);

    // 2. 算出內容的起始像素座標 (加入 Padding 讓字不要貼齊邊緣)
    int start_x = win->x + 8;
    int start_y = win->y + 26;

    // 3. 把文字畫上去！
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
utils.c
```c
#include "tty.h"
#include "gui.h"

#include <stdarg.h>
#include <stdbool.h>

// === Memory Utils ===

// 記憶體複製 (將 src 複製 size 個 bytes 到 dst)
void* memcpy(void* dstptr, const void* srcptr, size_t size) {
    unsigned char* dst = (unsigned char*) dstptr;
    const unsigned char* src = (const unsigned char*) srcptr;
    for (size_t i = 0; i < size; i++) {
        dst[i] = src[i];
    }
    return dstptr;
}

// 記憶體填充 (將 buf 的前 size 個 bytes 填入單一數值 value)
void* memset(void* bufptr, int value, size_t size) {
    unsigned char* buf = (unsigned char*) bufptr;
    for (size_t i = 0; i < size; i++) {
        buf[i] = (unsigned char) value;
    }
    return bufptr;
}

// === StringUilts ===
// 輔助函式：反轉字串
void reverse_string(char* str, int length) {
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char *strcpy(char *dest, const char *src) {
    char *saved = dest;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return saved;
}

// 核心工具：整數轉字串 (itoa)
// value: 要轉換的數字, str: 存放結果的陣列, base: 進位制 (10或16)
void itoa(int value, char* str, int base) {
    int i = 0;
    bool is_negative = false;
    unsigned int uvalue; // [新增] 用來做運算的無號整數

    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    // 處理負數 (僅限十進位)
    if (value < 0 && base == 10) {
        is_negative = true;
        uvalue = (unsigned int)(-value);
    } else {
        // [關鍵] 如果是16進位，直接把位元當作無號整數看待 (完美對應記憶體的原始狀態)
        uvalue = (unsigned int)value;
    }

    // 逐位取出餘數 (改用 uvalue)
    while (uvalue != 0) {
        int rem = uvalue % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'A' : rem + '0';
        uvalue = uvalue / base;
    }

    if (is_negative) {
        str[i++] = '-';
    }

    str[i] = '\0';
    reverse_string(str, i);
}

int strlen(const char* str) {
    int len = 0;
    while (str[len] != '\0') len++;
    return len;
}


// === IO Utils ===

// Kernel 專屬格式化輸出 (kprintf)
void kprintf(const char* format, ...) {
    va_list args;
    va_start(args, format); // 初始化不定參數列表

    for (size_t i = 0; format[i] != '\0'; i++) {
        // 如果不是 '%'，就當作一般字元直接印出
        if (format[i] != '%') {
            terminal_putchar(format[i]);
            continue;
        }

        // 遇到 '%'，看下一個字元是什麼來決定格式
        i++;
        switch (format[i]) {
            case 'd': { // 十進位整數
                int num = va_arg(args, int);
                char buffer[32];
                itoa(num, buffer, 10);
                terminal_writestring(buffer);
                break;
            }
            case 'x': { // 十六進位整數
                // [關鍵] 改用 unsigned int 取出參數
                unsigned int num = va_arg(args, unsigned int);
                char buffer[32];
                terminal_writestring("0x");
                itoa(num, buffer, 16);
                terminal_writestring(buffer);
                break;
            }
            case 's': { // 字串
                char* str = va_arg(args, char*);
                terminal_writestring(str);
                break;
            }
            case 'c': { // 單一字元
                // char 在變數傳遞時會被提升為 int
                char c = (char) va_arg(args, int);
                terminal_putchar(c);
                break;
            }
            case '%': { // 印出 '%' 本身
                terminal_putchar('%');
                break;
            }
            default: // 未知的格式，原樣印出
                terminal_putchar('%');
                terminal_putchar(format[i]);
                break;
        }
    }

    va_end(args); // 清理不定參數列表

    // 【關鍵優化】整句 kprintf 組合完畢後，再一次性渲染到螢幕上！
    gui_redraw();
}


uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

void outw(uint16_t port, uint16_t data) {
    __asm__ volatile ("outw %1, %0" : : "dN" (port), "a" (data));
}
```

---
gui.c
```c
#include "gui.h"
#include "gfx.h"
#include "utils.h"
#include "tty.h"
#include "rtc.h"

#define MAX_WINDOWS 10
#define TERM_BG 0x008080 // 桌面底色

extern void tty_render_window(int win_id);

static window_t windows[MAX_WINDOWS];
static int window_count = 0;

// 【新增】記錄當前被選中的視窗 (-1 代表沒有)
static int focused_window_id = -1;
static int last_mouse_x = 400;
static int last_mouse_y = 300;

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


static void draw_taskbar(void) {
    int screen_w = 800;
    int screen_h = 600;
    int taskbar_h = 28;
    int taskbar_y = screen_h - taskbar_h;

    // 1. 主底板 (經典亮灰色)
    draw_rect(0, taskbar_y, screen_w, taskbar_h, 0xC0C0C0);
    // 上方的高光白線，營造凸起感
    draw_rect(0, taskbar_y, screen_w, 2, 0xFFFFFF);

    // 2. Start 按鈕 (凸起的 3D 按鈕)
    draw_rect(4, taskbar_y + 4, 60, 20, 0xC0C0C0);
    draw_rect(4, taskbar_y + 4, 60, 2, 0xFFFFFF); // 上亮邊
    draw_rect(4, taskbar_y + 4, 2, 20, 0xFFFFFF); // 左亮邊
    draw_rect(4, taskbar_y + 22, 60, 2, 0x808080); // 下陰影
    draw_rect(62, taskbar_y + 4, 2, 20, 0x808080); // 右陰影
    draw_string(14, taskbar_y + 10, "Start", 0x000000, 0xC0C0C0);

    // 3. 右下角時鐘系統匣 (凹陷的 3D 區域)
    int tray_w = 66;
    int tray_x = screen_w - tray_w - 4;
    draw_rect(tray_x, taskbar_y + 4, tray_w, 20, 0xC0C0C0);
    draw_rect(tray_x, taskbar_y + 4, tray_w, 2, 0x808080);  // 上陰影 (凹陷感)
    draw_rect(tray_x, taskbar_y + 4, 2, 20, 0x808080);  // 左陰影
    draw_rect(tray_x, taskbar_y + 22, tray_w, 2, 0xFFFFFF); // 下亮邊
    draw_rect(tray_x + tray_w - 2, taskbar_y + 4, 2, 20, 0xFFFFFF); // 右亮邊

    // 4. 取得硬體時間並轉成字串
    int h, m;
    read_time(&h, &m);

    char time_str[6] = "00:00";
    time_str[0] = (h / 10) + '0';
    time_str[1] = (h % 10) + '0';
    time_str[3] = (m / 10) + '0';
    time_str[4] = (m % 10) + '0';

    // 畫出時間
    draw_string(tray_x + 10, taskbar_y + 10, time_str, 0x000000, 0xC0C0C0);
}

// -- Public API

void init_gui(void) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i].is_active = 0;
    }
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

window_t* get_window(int id) {
    if (id >= 0 && id < MAX_WINDOWS && windows[id].is_active) {
        return &windows[id];
    }
    return 0; // NULL
}

window_t* get_windows(void) {
    return windows;
}

void set_focused_window(int id) { focused_window_id = id; }

int get_focused_window(void) { return focused_window_id; }

void close_window(int id) {
    if (id >= 0 && id < MAX_WINDOWS) {
        windows[id].is_active = 0;
        if (focused_window_id == id) focused_window_id = -1;
    }
}

void gui_render(int mouse_x, int mouse_y) {
    last_mouse_x = mouse_x;
    last_mouse_y = mouse_y;

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
    }

    // 3. 畫終端機文字
    // tty_render();

    // 4. 畫工作列 (Taskbar)
    draw_taskbar();

    // 5. 畫滑鼠游標 (永遠在最最上層)
    draw_cursor(mouse_x, mouse_y);

    // 交換畫布
    gfx_swap_buffers();
}

// 【新增】提供給打字機呼叫的重繪函式
void gui_redraw(void) {
    gui_render(last_mouse_x, last_mouse_y);
}
```
