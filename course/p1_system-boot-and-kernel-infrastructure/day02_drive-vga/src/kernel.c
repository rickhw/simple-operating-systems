#include <stdint.h>
#include <stddef.h>

// 定義 VGA 記憶體起始位址
volatile uint16_t* vga_buffer = (uint16_t*)0xB8000;

// 定義螢幕的寬高
const size_t VGA_COLS = 80;
const size_t VGA_ROWS = 25;

// 函式：初始化終端機（清空螢幕）
void terminal_initialize(void) {
    for (size_t y = 0; y < VGA_ROWS; y++) {
        for (size_t x = 0; x < VGA_COLS; x++) {
            const size_t index = y * VGA_COLS + x;
            // 0x0F 代表黑底白字，' ' 是空白字元
            vga_buffer[index] = ((uint16_t)0x0F << 8) | ' ';
        }
    }
}

// 函式：在畫面上印出字串
void terminal_writestring(const char* data) {
    size_t index = 0;
    while (data[index] != '\0') {
        // 0x0A 代表黑底亮綠色
        vga_buffer[index] = ((uint16_t)0x0A << 8) | data[index];
        index++;
    }
}

// 這是我們作業系統的 C 語言主程式！
void kernel_main(void) {
    terminal_initialize();
    terminal_writestring("Hello, C Kernel World! I am running on x86!");
}
