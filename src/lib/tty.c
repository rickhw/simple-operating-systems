#include <stdint.h>
#include <stddef.h>
#include "tty.h"
#include "utils.h"
#include "gfx.h" // 引入我們強大的圖形引擎

// 設定終端機的字體顏色為白色 (0xFFFFFF)，背景為純黑 (0x000000)
#define TERM_FG 0xFFFFFF
#define TERM_BG 0x000000

// 記錄目前打字游標的 (X, Y) 像素座標
static int term_x = 0;
static int term_y = 0;

// 假設解析度是 800x600 (你可以未來把它變成從 gfx.h 讀取)
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600
#define FONT_WIDTH    8
#define FONT_HEIGHT   8

void terminal_initialize(void) {
    term_x = 0;
    term_y = 0;
    // 用圖形引擎把整個螢幕塗黑，當作清空畫面
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, TERM_BG);
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
        if (term_x >= FONT_WIDTH) {
            term_x -= FONT_WIDTH;
            draw_char(' ', term_x, term_y, TERM_FG, TERM_BG); // 用空白蓋掉舊字元
        } else if (term_y >= FONT_HEIGHT) {
            // 退到上一行 (這裡簡化處理，直接退到上一行最右邊)
            term_y -= FONT_HEIGHT;
            term_x = SCREEN_WIDTH - FONT_WIDTH;
            draw_char(' ', term_x, term_y, TERM_FG, TERM_BG);
        }
        return;
    }

    // 呼叫圖形引擎畫出字元！
    draw_char(c, term_x, term_y, TERM_FG, TERM_BG);

    // 畫完之後，游標往右移
    term_x += FONT_WIDTH;

    // 如果超過螢幕寬度，就自動換行
    if (term_x >= SCREEN_WIDTH) {
        term_x = 0;
        term_y += FONT_HEIGHT;
        if (term_y >= SCREEN_HEIGHT) {
            terminal_initialize();
        }
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
