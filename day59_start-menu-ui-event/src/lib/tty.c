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
