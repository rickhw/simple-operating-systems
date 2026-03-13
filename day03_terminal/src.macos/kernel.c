#include <stdint.h>
#include <stddef.h>

volatile uint16_t* vga_buffer = (uint16_t*)0xB8000;
const size_t VGA_COLS = 80;
const size_t VGA_ROWS = 25;

// 系統狀態變數：追蹤目前的游標位置與預設顏色
size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;

// 輔助函式：設定特定位置的字元與顏色
void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_COLS + x;
    vga_buffer[index] = ((uint16_t)color << 8) | c;
}

// 輔助函式：螢幕往上滾動一行
void terminal_scroll() {
    // 將第 1~24 行的內容，搬移到第 0~23 行
    for (size_t y = 1; y < VGA_ROWS; y++) {
        for (size_t x = 0; x < VGA_COLS; x++) {
            const size_t src_index = y * VGA_COLS + x;
            const size_t dest_index = (y - 1) * VGA_COLS + x;
            vga_buffer[dest_index] = vga_buffer[src_index];
        }
    }
    // 清空最後一行 (第 24 行)
    for (size_t x = 0; x < VGA_COLS; x++) {
        terminal_putentryat(' ', terminal_color, x, VGA_ROWS - 1);
    }
    // 滾動後，游標回到最後一行的開頭
    terminal_row = VGA_ROWS - 1;
}

// 核心函式：處理單一字元的邏輯 (包含換行與邊界偵測)
void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        terminal_row++;
    } else {
        terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
        terminal_column++;
        // 如果到了畫面最右邊，自動換行
        if (terminal_column == VGA_COLS) {
            terminal_column = 0;
            terminal_row++;
        }
    }

    // 如果游標超出了最底層，觸發滾動
    if (terminal_row == VGA_ROWS) {
        terminal_scroll();
    }
}

// 更新版的字串列印函式
void terminal_writestring(const char* data) {
    size_t index = 0;
    while (data[index] != '\0') {
        terminal_putchar(data[index]);
        index++;
    }
}

void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = 0x0F; // 黑底白字
    for (size_t y = 0; y < VGA_ROWS; y++) {
        for (size_t x = 0; x < VGA_COLS; x++) {
            terminal_putentryat(' ', terminal_color, x, y);
        }
    }
}

void kernel_main(void) {
    terminal_initialize();

    // 測試換行功能
    terminal_writestring("Hello, System Architect!\n");
    terminal_writestring("Welcome to Day 3.\n");

    // 測試螢幕滾動功能 (印出 30 行，迫使畫面捲動)
    for (int i = 0; i < 30; i++) {
        terminal_color = 0x0A + (i % 5); // 讓顏色稍微變化一下
        terminal_writestring("Scrolling line testing...\n");
    }

    terminal_color = 0x0E; // 黃色
    terminal_writestring("Scroll test complete. The system is stable.\n");
}
