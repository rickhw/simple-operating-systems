/**
 * @file src/kernel/drivers/video/tty.c
 * @brief Main logic and program flow for tty.c.
 *
 * This file handles the operations and logic associated with tty.c.
 */

#include <stdint.h>
#include <stddef.h>
#include "tty.h"
#include "utils.h"
#include "gfx.h"
#include "gui.h"

// 設定終端機的字體顏色
#define TERM_FG 0xFFFFFF    // 白色
#define TERM_BG 0x008080    // 深青色 (Teal)

// 假設解析度是 800x600
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600
#define FONT_WIDTH    8
#define FONT_HEIGHT   8


// 記錄目前打字游標的 (X, Y) 像素座標
static int term_col = 0; // 目前在第幾直行
static int term_row = 0; // 目前在第幾橫列
// 記錄終端機被綁定到哪一個 GUI 視窗 ID
static int bound_win_id = -1;

// 改用變數來控制邊界 (預設為全螢幕)
static int max_cols = 100;
static int max_rows = 75;
// 文字緩衝區開到最大 (100x75)
static char text_buffer[75][100];

// --- Public API

void terminal_initialize(void) {
    term_col = 0;
    term_row = 0;
    for (int r = 0; r < max_rows; r++) {
        for (int c = 0; c < max_cols; c++) {
            text_buffer[r][c] = '\0';
        }
    }
}

void terminal_putchar(char c) {
    if (c == '\n') {
        term_col = 0;
        term_row++;
        if (term_row >= max_rows) terminal_initialize();
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
    if (term_col >= max_cols) {
        term_col = 0;
        term_row++;
        if (term_row >= max_rows) terminal_initialize();
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

// 【新增】切換模式與邊界
void terminal_set_mode(int is_gui) {
    if (is_gui) {
        max_cols = 45;
        max_rows = 25;
    } else {
        max_cols = 100;
        max_rows = 75;
    }
    terminal_initialize(); // 切換模式時清空畫面
}

// 只在綁定的視窗被繪製時，才渲染文字！
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
    for (int r = 0; r < max_rows; r++) {
        for (int c = 0; c < max_cols; c++) {
            char ch = text_buffer[r][c];
            if (ch != '\0' && ch != ' ') {
                draw_char_transparent(ch, start_x + c * 8, start_y + r * 8, TERM_FG);
            }
        }
    }

    // 畫一個閃爍的底線游標
    draw_rect(start_x + term_col * 8, start_y + term_row * 8 + 6, 8, 2, TERM_FG);
}

// 【新增】CLI 專用的全螢幕渲染
void tty_render_fullscreen(void) {
    for (int r = 0; r < max_rows; r++) {
        for (int c = 0; c < max_cols; c++) {
            char ch = text_buffer[r][c];
            if (ch != '\0' && ch != ' ') {
                // 從畫面左上角 (0,0) 開始畫
                draw_char_transparent(ch, c * 8, r * 8, TERM_FG);
            }
        }
    }
    // 畫游標
    draw_rect(term_col * 8, term_row * 8 + 6, 8, 2, TERM_FG);
}
