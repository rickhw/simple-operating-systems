#include <stdint.h>
#include <stddef.h>
// #include <stdarg.h>
// #include <stdbool.h>

#include "keyboard.h"
#include "io.h"
#include "utils.h"

// 美式鍵盤的 Scancode 對應表 (無 Shift 狀態)
// 例如: 按下 'A' 的 Scancode 是 0x1E (十進位 30)，kbd_US[30] 就是 'a'
const unsigned char kbd_US[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,
  '*',  0,  ' ',  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0, '-',   0,   0,   0, '+',   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);

    // 如果最高位元是 0 (代表按下按鍵)
    if (!(scancode & 0x80)) {
        // 從對應表中抓出字元
        char c = kbd_US[scancode];

        // 如果這個字元存在 (不是 0)，就把它印出來！
        if (c != 0) {
            // [注意] 這裡我們先借用 kprintf 來印單一字元
            kprintf("%c", c);
        }
    }

    // 發送 EOI 給 PIC
    outb(0x20, 0x20);
}
