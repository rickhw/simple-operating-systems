#include "gui.h"
#include "gfx.h"
#include "utils.h"
#include "tty.h"

#define MAX_WINDOWS 10
#define TERM_BG 0x008080 // 桌面底色

static window_t windows[MAX_WINDOWS];
static int window_count = 0;

void init_gui(void) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i].is_active = 0;
    }
}

int create_window(int x, int y, int width, int height, const char* title) {
    if (window_count >= MAX_WINDOWS) return -1;

    int id = window_count;
    windows[id].id = id;
    windows[id].x = x;
    windows[id].y = y;
    windows[id].width = width;
    windows[id].height = height;
    strcpy(windows[id].title, title);
    windows[id].is_active = 1;

    window_count++;
    return id;
}

window_t* get_windows(void) {
    return windows;
}


// 【終極遊戲渲染迴圈】
void gui_render(int mouse_x, int mouse_y) {
    // 1. 畫背景 (Clear Screen)
    draw_rect(0, 0, 800, 600, TERM_BG);

    // 2. 畫視窗 (Draw Sprites)
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_active) {
            draw_window(windows[i].x, windows[i].y, windows[i].width, windows[i].height, windows[i].title);

            if (i == 0) {
                draw_string(windows[i].x + 10, windows[i].y + 30, "CPU: x86 32-bit", 0x000000, 0xC0C0C0);
                draw_string(windows[i].x + 10, windows[i].y + 50, "Memory: 16 MB", 0x000000, 0xC0C0C0);
                draw_string(windows[i].x + 10, windows[i].y + 70, "GUI: Active", 0x000000, 0xC0C0C0);
            }
        }
    }

    // 3. 畫終端機文字 (Draw UI Layer)
    tty_render();

    // 4. 畫滑鼠游標 (Draw Cursor On Top)
    draw_cursor(mouse_x, mouse_y);

    // 5. 瞬間交換畫布！(Swap Buffers)
    gfx_swap_buffers();
}
