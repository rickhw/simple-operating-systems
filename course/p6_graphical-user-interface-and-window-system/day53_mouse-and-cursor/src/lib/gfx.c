#include "gfx.h"
#include "paging.h"
#include "tty.h"
#include "font8x8.h"

// 圖形引擎的私有狀態
static uint8_t* fb_addr = 0;
static uint32_t fb_pitch = 0;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint8_t  fb_bpp = 0;

// [Day53] add -- start
// 滑鼠游標系統 (Mouse Cursor System)
static int cursor_x = 400;
static int cursor_y = 300;
static uint32_t cursor_bg[100]; // 儲存游標底下的 10x10 背景像素

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
// [Day53] add -- end

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

        // （因為現在 kprintf 已經看不到畫面了，這個 Log 只會留在 serial port 裡）
        // kprintf("[GFX] Initialized: %dx%dx%d at 0x%x\n", fb_width, fb_height, fb_bpp, fb_addr);
    }
}

void put_pixel(int x, int y, uint32_t color) {
    if (fb_addr == 0) return;

    // 邊界保護 (非常重要，否則畫錯會 Page Fault！)
    if (x < 0 || (uint32_t)x >= fb_width || y < 0 || (uint32_t)y >= fb_height) return;

    uint32_t offset = (y * fb_pitch) + (x * (fb_bpp / 8));

    fb_addr[offset]     = color & 0xFF;         // Blue
    fb_addr[offset + 1] = (color >> 8) & 0xFF;  // Green
    fb_addr[offset + 2] = (color >> 16) & 0xFF; // Red
}

void draw_rect(int start_x, int start_y, int width, int height, uint32_t color) {
    for (int y = start_y; y < start_y + height; y++) {
        for (int x = start_x; x < start_x + width; x++) {
            put_pixel(x, y, color);
        }
    }
}


// [Day52] add -- start
// 用像素點陣畫出一個 ASCII 字元
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
// [Day52] add -- end


// [Day53] add -- start
// 【新增】取得螢幕上特定座標的像素顏色
uint32_t get_pixel(int x, int y) {
    if (fb_addr == 0 || x < 0 || (uint32_t)x >= fb_width || y < 0 || (uint32_t)y >= fb_height) return 0;
    uint32_t offset = (y * fb_pitch) + (x * (fb_bpp / 8));
    // 組合 BGRA 成為 32-bit 顏色
    uint32_t color = fb_addr[offset] | (fb_addr[offset + 1] << 8) | (fb_addr[offset + 2] << 16);
    return color;
}

// 【新增】畫出滑鼠游標 (並處理背景還原)
void draw_cursor(int new_x, int new_y) {
    // 1. 先把「舊位置」的背景還原
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            put_pixel(cursor_x + j, cursor_y + i, cursor_bg[i * 10 + j]);
        }
    }

    // 2. 更新最新座標
    cursor_x = new_x;
    cursor_y = new_y;

    // 3. 備份「新位置」的背景，然後畫上游標
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            // 備份背景
            cursor_bg[i * 10 + j] = get_pixel(cursor_x + j, cursor_y + i);

            // 畫游標
            if (cursor_bmp[i][j] == 1) put_pixel(cursor_x + j, cursor_y + i, 0xFFFFFF); // 白邊
            else if (cursor_bmp[i][j] == 2) put_pixel(cursor_x + j, cursor_y + i, 0x000000); // 黑底
        }
    }
}
// [Day53] add -- end
