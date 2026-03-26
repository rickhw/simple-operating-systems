// 2D 繪圖引擎庫
#ifndef GFX_H
#define GFX_H

#include <stdint.h>
#include "multiboot.h"

// 初始化圖形引擎
void init_gfx(multiboot_info_t* mbd);

// 核心繪圖 API
void put_pixel(int x, int y, uint32_t color);

// [Day 52] 我們即將加入的進階 API
void draw_rect(int x, int y, int width, int height, uint32_t color);

// [Day 52] 教圖形引擎畫字
void draw_char(char c, int x, int y, uint32_t fg_color, uint32_t bg_color);


uint32_t get_pixel(int x, int y);   // [Day53]
void draw_cursor(int x, int y);     // [Day53]

void gfx_swap_buffers(void);        // [Day55]
void draw_char_transparent(char c, int x, int y, uint32_t fg_color);    // [Day55]


#endif
