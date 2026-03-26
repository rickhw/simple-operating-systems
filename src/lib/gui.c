#include "gui.h"
#include "gfx.h"
#include "utils.h"
#include "tty.h"

#define MAX_WINDOWS 10
#define TERM_BG 0x008080 // 桌面底色

extern void tty_render_window(int win_id);

static window_t windows[MAX_WINDOWS];
static int window_count = 0;

// 【新增】記錄當前被選中的視窗 (-1 代表沒有)
static int focused_window_id = -1;

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

// 【加回來】
window_t* get_windows(void) {
    return windows;
}

// 【新增】API 實作
void set_focused_window(int id) { focused_window_id = id; }
int get_focused_window(void) { return focused_window_id; }
void close_window(int id) {
    if (id >= 0 && id < MAX_WINDOWS) {
        windows[id].is_active = 0;
        if (focused_window_id == id) focused_window_id = -1;
    }
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
    }

    draw_cursor(mouse_x, mouse_y);
    gfx_swap_buffers();
}
