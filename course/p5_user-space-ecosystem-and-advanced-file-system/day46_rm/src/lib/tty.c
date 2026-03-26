// TTY: TeleTYpewriter
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include "tty.h"
#include "io.h"
#include "utils.h"

volatile uint16_t* vga_buffer = (uint16_t*)0xB8000;
const size_t VGA_COLS = 80;
const size_t VGA_ROWS = 25;

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;

// --- 公開 API ---

void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = 0x0F;
    for (size_t y = 0; y < VGA_ROWS; y++) {
        for (size_t x = 0; x < VGA_COLS; x++) {
            terminal_putentryat(' ', terminal_color, x, y);
        }
    }
    update_cursor(terminal_column, terminal_row);
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        terminal_row++;
    } else {
        terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
        terminal_column++;
        if (terminal_column == VGA_COLS) {
            terminal_column = 0;
            terminal_row++;
        }
    }
    if (terminal_row == VGA_ROWS) {
        terminal_scroll();
    }

    // [新增] 每次印出字元或換行後，更新硬體游標的位置！
    update_cursor(terminal_column, terminal_row);
}

void terminal_writestring(const char* data) {
    size_t index = 0;
    while (data[index] != '\0') {
        terminal_putchar(data[index]);
        index++;
    }
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_COLS + x;
    vga_buffer[index] = ((uint16_t)color << 8) | c;
}

void terminal_scroll() {
    // 1. 把第 1~24 行的資料，直接「整塊」複製到第 0~23 行的位置
    // 每個字元佔 2 bytes，所以大小是 (VGA_ROWS - 1) * VGA_COLS * 2
    size_t bytes_to_copy = (VGA_ROWS - 1) * VGA_COLS * 2;
    memcpy((void*)vga_buffer, (void*)(vga_buffer + VGA_COLS), bytes_to_copy);

    // 2. 清空最後一行 (使用我們原本的迴圈，或者你未來也可以進階寫個 memclr)
    for (size_t x = 0; x < VGA_COLS; x++) {
        terminal_putentryat(' ', terminal_color, x, VGA_ROWS - 1);
    }
    terminal_row = VGA_ROWS - 1;
}

// 控制 VGA 硬體游標
void update_cursor(int x, int y) {
    // 游標位置是一維陣列的索引 (0 ~ 1999)
    uint16_t pos = y * VGA_COLS + x;

    // VGA 控制器的 Port 0x3D4 是「索引暫存器」，用來告訴 VGA 我們要設定什麼
    // VGA 控制器的 Port 0x3D5 是「資料暫存器」，用來寫入實際的值

    // 設定游標位置的低 8 bits (暫存器 0x0F)
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t) (pos & 0xFF));

    // 設定游標位置的高 8 bits (暫存器 0x0E)
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}
