/**
 * @file src/kernel/drivers/video/gfx.c
 * @brief Main logic and program flow for gfx.c.
 *
 * This file handles the operations and logic associated with gfx.c.
 */

#include "gfx.h"
#include "paging.h"
#include "tty.h"
#include "font8x8.h"
#include "utils.h"

// 圖形引擎的私有狀態
static uint8_t* fb_addr = 0;
static uint32_t fb_pitch = 0;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint8_t  fb_bpp = 0;

// 隱形畫布 (Back Buffer)
// 800x600 * 4 Bytes = 約 1.92 MB。我們把它放在 BSS 區段，既安全又快速！
static uint32_t back_buffer[800 * 600];

// 10x10 游標點陣圖 (1: 白色邊框, 2: 黑色填滿, 0: 透明)
static const uint8_t cursor_bmp[10][10] = {
    {1,1,0,0,0,0,0,0,0,0},
    {1,2,1,0,0,0,0,0,0,0},
    {1,2,2,1,0,0,0,0,0,0},
    {1,2,2,2,1,0,0,0,0,0},
    {1,2,2,2,2,1,0,0,0,0},
    {1,2,2,2,2,2,1,0,0,0},
    {1,2,2,2,2,2,2,1,0,0},
    {1,2,2,1,1,1,1,1,1,0},
    {1,1,1,0,0,0,0,0,0,0},
    {1,0,0,0,0,0,0,0,0,0}
};

void init_gfx(multiboot_info_t* mbd) {
    if (mbd->flags & (1 << 12)) {
        fb_addr = (uint8_t*) (uint32_t) mbd->framebuffer_addr;
        fb_pitch = mbd->framebuffer_pitch;
        fb_width = mbd->framebuffer_width;
        fb_height = mbd->framebuffer_height;
        fb_bpp = mbd->framebuffer_bpp;

        // 計算並映射 VRAM
        uint32_t fb_size = fb_pitch * fb_height;
        for (uint32_t i = 0; i < fb_size; i += 4096) {
            map_vram((uint32_t)fb_addr + i, (uint32_t)fb_addr + i);
        }

        // 刷新 TLB
        uint32_t cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
        __asm__ volatile("mov %0, %%cr3" : : "r"(cr3));
    }
}

// 現在所有的畫筆，都畫在隱形的 back_buffer 上！
void put_pixel(int x, int y, uint32_t color) {
    if (x < 0 || (uint32_t)x >= fb_width || y < 0 || (uint32_t)y >= fb_height) return;
    back_buffer[y * fb_width + x] = color;
}

uint32_t get_pixel(int x, int y) {
    if (x < 0 || (uint32_t)x >= fb_width || y < 0 || (uint32_t)y >= fb_height) return 0;
    return back_buffer[y * fb_width + x];
}

// 瞬間交換畫布！(消除閃爍的終極武器)
void gfx_swap_buffers() {
    if (fb_addr == 0) return;
    // 逐行將 back_buffer 複製到實體的 Framebuffer
    for (uint32_t y = 0; y < fb_height; y++) {
        memcpy(fb_addr + (y * fb_pitch), &back_buffer[y * fb_width], fb_width * 4);
    }
}

void draw_rect(int start_x, int start_y, int width, int height, uint32_t color) {
    for (int y = start_y; y < start_y + height; y++) {
        for (int x = start_x; x < start_x + width; x++) {
            put_pixel(x, y, color);
        }
    }
}

void draw_char(char c, int x, int y, uint32_t fg_color, uint32_t bg_color) {
    // 過濾掉我們字型庫沒有的控制字元或怪異字元
    if (c < 32 || c > 126) return;

    // 取得這個字母對應的 8 Bytes 像素點陣圖 (ASCII 32 對應陣列第 0 個)
    const uint8_t* glyph = font8x8[c - 32];

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            // 利用位元遮罩 (Bitmask) 檢查這個像素是 1 還是 0
            // 字型點陣通常是從左到右，最高位元 (0x80) 在最左邊
            if (glyph[row] & (0x80 >> col)) {
                put_pixel(x + col, y + row, fg_color); // 畫筆畫上字型顏色
            } else {
                put_pixel(x + col, y + row, bg_color); // 塗上背景色 (蓋掉原本的畫面)
            }
        }
    }
}

// 直接把滑鼠游標畫上去
void draw_cursor(int new_x, int new_y) {
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            if (cursor_bmp[i][j] == 1) put_pixel(new_x + j, new_y + i, 0xFFFFFF);
            else if (cursor_bmp[i][j] == 2) put_pixel(new_x + j, new_y + i, 0x000000);
        }
    }
}

void draw_string(int x, int y, const char* str, uint32_t fg_color, uint32_t bg_color) {
    int curr_x = x;
    for (int i = 0; str[i] != '\0'; i++) {
        draw_char(str[i], curr_x, y, fg_color, bg_color);
        curr_x += 8; // 每個字元寬 8 像素
    }
}

void draw_window(int x, int y, int width, int height, const char* title) {
    // 1. 視窗主體 (經典的亮灰色 0xC0C0C0)
    draw_rect(x, y, width, height, 0xC0C0C0);

    // 2. 視窗邊框 (簡單的立體陰影)
    // 上邊與左邊 (白色反光)
    draw_rect(x, y, width, 2, 0xFFFFFF);
    draw_rect(x, y, 2, height, 0xFFFFFF);
    // 下邊與右邊 (深灰色陰影)
    draw_rect(x, y + height - 2, width, 2, 0x808080);
    draw_rect(x + width - 2, y, 2, height, 0x808080);

    // 3. 標題列 (經典的深藍色 0x000080)
    // 留出邊框的空間 (x+2, y+2)，高度設為 18 像素
    draw_rect(x + 2, y + 2, width - 4, 18, 0x000080);

    // 4. 標題文字 (白色字，深藍色底)
    // 微調座標讓字體置中於標題列
    draw_string(x + 6, y + 7, title, 0xFFFFFF, 0x000080);
}


// 畫透明底色的字元 (給終端機疊加在背景上用)
void draw_char_transparent(char c, int x, int y, uint32_t fg_color) {
    if (c < 32 || c > 126) return;
    const uint8_t* glyph = font8x8[c - 32];
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (glyph[row] & (0x80 >> col)) {
                put_pixel(x + col, y + row, fg_color);
            }
        }
    }
}
