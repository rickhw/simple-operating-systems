#include "gfx.h"
#include "paging.h"
#include "tty.h"

// 圖形引擎的私有狀態
static uint8_t* fb_addr = 0;
static uint32_t fb_pitch = 0;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint8_t  fb_bpp = 0;

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
