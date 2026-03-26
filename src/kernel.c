#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>   // [新增] 用於處理不定長度引數
#include <stdbool.h>  // [新增] 支援 bool 型別
#include "gdt.h"
#include "idt.h" // [新增]

volatile uint16_t* vga_buffer = (uint16_t*)0xB8000;
const size_t VGA_COLS = 80;
const size_t VGA_ROWS = 25;

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;

// ==========================================
// [新增] 1. 基礎標準函式庫 (libc) 實作
// ==========================================

// 記憶體複製 (將 src 複製 size 個 bytes 到 dst)
void* memcpy(void* dstptr, const void* srcptr, size_t size) {
    unsigned char* dst = (unsigned char*) dstptr;
    const unsigned char* src = (const unsigned char*) srcptr;
    for (size_t i = 0; i < size; i++) {
        dst[i] = src[i];
    }
    return dstptr;
}

// 記憶體填充 (將 buf 的前 size 個 bytes 填入單一數值 value)
void* memset(void* bufptr, int value, size_t size) {
    unsigned char* buf = (unsigned char*) bufptr;
    for (size_t i = 0; i < size; i++) {
        buf[i] = (unsigned char) value;
    }
    return bufptr;
}

// ==========================================
// [新增] 2. 硬體 I/O 通訊 (Inline Assembly)
// ==========================================

// outb: 向指定的硬體 Port 寫入 1 byte 的資料
// 這裡使用了 GCC 的內嵌組合語言語法
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

// ==========================================
// [新增] 3. 控制 VGA 硬體游標
// ==========================================
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

// ==========================================
// [修改] 4. 優化後的終端機操作
// ==========================================

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_COLS + x;
    vga_buffer[index] = ((uint16_t)color << 8) | c;
}

// [修改] 使用 memcpy 加速滾動！
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
// ==========================================
// [新增] 5. 字串與數字處理工具
// ==========================================

// 輔助函式：反轉字串
void reverse_string(char* str, int length) {
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

// 核心工具：整數轉字串 (itoa)
// value: 要轉換的數字, str: 存放結果的陣列, base: 進位制 (10或16)
void itoa(int value, char* str, int base) {
    int i = 0;
    bool is_negative = false;
    unsigned int uvalue; // [新增] 用來做運算的無號整數

    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    // 處理負數 (僅限十進位)
    if (value < 0 && base == 10) {
        is_negative = true;
        uvalue = (unsigned int)(-value);
    } else {
        // [關鍵] 如果是16進位，直接把位元當作無號整數看待 (完美對應記憶體的原始狀態)
        uvalue = (unsigned int)value;
    }

    // 逐位取出餘數 (改用 uvalue)
    while (uvalue != 0) {
        int rem = uvalue % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'A' : rem + '0';
        uvalue = uvalue / base;
    }

    if (is_negative) {
        str[i++] = '-';
    }

    str[i] = '\0';
    reverse_string(str, i);
}

// ==========================================
// [新增] 6. 核心專屬格式化輸出 (kprintf)
// ==========================================

// 簡易版的 printf
void kprintf(const char* format, ...) {
    va_list args;
    va_start(args, format); // 初始化不定參數列表

    for (size_t i = 0; format[i] != '\0'; i++) {
        // 如果不是 '%'，就當作一般字元直接印出
        if (format[i] != '%') {
            terminal_putchar(format[i]);
            continue;
        }

        // 遇到 '%'，看下一個字元是什麼來決定格式
        i++;
        switch (format[i]) {
            case 'd': { // 十進位整數
                int num = va_arg(args, int);
                char buffer[32];
                itoa(num, buffer, 10);
                terminal_writestring(buffer);
                break;
            }
            // case 'x': { // 十六進位整數
            //     int num = va_arg(args, int);
            //     char buffer[32];
            //     terminal_writestring("0x"); // 加上 0x 前綴
            //     itoa(num, buffer, 16);
            //     terminal_writestring(buffer);
            //     break;
            // }
            case 'x': { // 十六進位整數
                // [關鍵] 改用 unsigned int 取出參數
                unsigned int num = va_arg(args, unsigned int);
                char buffer[32];
                terminal_writestring("0x");
                itoa(num, buffer, 16);
                terminal_writestring(buffer);
                break;
            }
            case 's': { // 字串
                char* str = va_arg(args, char*);
                terminal_writestring(str);
                break;
            }
            case 'c': { // 單一字元
                // char 在變數傳遞時會被提升為 int
                char c = (char) va_arg(args, int);
                terminal_putchar(c);
                break;
            }
            case '%': { // 印出 '%' 本身
                terminal_putchar('%');
                break;
            }
            default: // 未知的格式，原樣印出
                terminal_putchar('%');
                terminal_putchar(format[i]);
                break;
        }
    }

    va_end(args); // 清理不定參數列表
}




// ==========================================
// [修改] 主程式測試
// ==========================================
void kernel_main(void) {
    terminal_initialize();

    kprintf("=== OS Kernel Booting ===\n");

    init_gdt();
    kprintf("GDT loaded successfully.\n");

    // 初始化神經系統 IDT
    init_idt();
    kprintf("IDT loaded successfully.\n");

    // [致命測試] 故意觸發除以零錯誤！
    kprintf("Testing Divide by Zero Exception...\n");
    int a = 10;
    // int b = 0;
    volatile int b = 0; // [關鍵修改] 加上 volatile
    int c = a / b; // CPU 執行到這行會立刻拋出 0 號中斷！

    // 如果 IDT 沒設定好，QEMU 會無限重啟。
    // 如果設定好了，這行永遠不會印出來，系統會被我們的 ISR 攔截！
    kprintf("Result: %d\n", c);
}
