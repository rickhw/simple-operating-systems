#include <stdint.h>
#include <stddef.h>
#include "tty.h"
#include "utils.h"
#include "gfx.h" // 引入我們強大的圖形引擎

// 設定終端機的字體顏色為白色 (0xFFFFFF)，背景為純黑 (0x000000)
#define TERM_FG 0xFFFFFF
#define TERM_BG 0x008080    // 把這行原本的黑色換成深青色 (Teal)

// 假設解析度是 800x600 (你可以未來把它變成從 gfx.h 讀取)
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600
#define FONT_WIDTH    8
#define FONT_HEIGHT   8
#define MAX_ROWS (SCREEN_HEIGHT / 8) // 75 行
#define MAX_COLS (SCREEN_WIDTH / 8) // 100 列

// 記錄目前打字游標的 (X, Y) 像素座標
static int term_x = 0;
static int term_y = 0;
static char text_buffer[MAX_ROWS][MAX_COLS];

void terminal_initialize(void) {
    term_x = 0;
    term_y = 0;
    // 清空文字緩衝區
    for (int r = 0; r < MAX_ROWS; r++) {
        for (int c = 0; c < MAX_COLS; c++) {
            text_buffer[r][c] = '\0';
        }
    }
}

void terminal_putchar(char c) {
    // 處理換行符號
    if (c == '\n') {
        term_x = 0;
        term_y += FONT_HEIGHT;
        // 換行時檢查是否超出螢幕底部
        if (term_y >= SCREEN_HEIGHT) {
            terminal_initialize(); // 超出底部就清空畫面重來
        }
        return;
    }

    // 處理退格鍵 (Backspace)
    if (c == '\b') {
        if (term_x >= 8) {
            term_x -= 8;
            text_buffer[term_y / 8][term_x / 8] = '\0'; // 抹除記憶
        }
        return;
    }

    // 記在陣列裡，不再直接畫！
    text_buffer[term_y / 8][term_x / 8] = c;
    term_x += 8;

    if (term_x >= 800) {
        term_x = 0;
        term_y += 8;
        if (term_y >= 600) terminal_initialize();
    }
}

void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        terminal_putchar(data[i]);
    }
}

void terminal_writestring(const char* data) {
    terminal_write(data, strlen(data));
}

// 【新增】把記憶體裡的字，畫在目前的畫布上
void tty_render(void) {
    for (int r = 0; r < MAX_ROWS; r++) {
        for (int c = 0; c < MAX_COLS; c++) {
            char ch = text_buffer[r][c];
            if (ch != '\0' && ch != ' ') {
                draw_char_transparent(ch, c * 8, r * 8, TERM_FG);
            }
        }
    }
}
