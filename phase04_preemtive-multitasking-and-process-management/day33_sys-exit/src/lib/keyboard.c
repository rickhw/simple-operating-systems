#include <stdint.h>
#include <stddef.h>

#include "keyboard.h"
#include "io.h"
#include "tty.h"
#include "utils.h"

// 建立一個 256 bytes 的環狀緩衝區
#define KBD_BUFFER_SIZE 256
volatile char kbd_buffer[KBD_BUFFER_SIZE];
volatile int kbd_head = 0; // 寫入指標
volatile int kbd_tail = 0; // 讀取指標

// 美式鍵盤的 Scancode 對應表 (無 Shift 狀態)
unsigned char kbdus[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,
  '*',  0,  ' ',  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0, '-',   0,   0,   0, '+',   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);

    // 如果最高位元是 0 (代表按下按鍵)
    if (!(scancode & 0x80)) {
        char ascii = kbdus[scancode];
        if (ascii != 0) {
            // 把字元存進緩衝區，並移動寫入指標
            kbd_buffer[kbd_head] = ascii;
            kbd_head = (kbd_head + 1) % KBD_BUFFER_SIZE;
        }
    }

    // 發送 EOI 給 PIC，告訴它中斷處理完畢
    outb(0x20, 0x20);
}

// 讓 Kernel 或 Syscall 讀取字元
// 讓 Kernel 或 Syscall 讀取字元
char keyboard_getchar() {
    // 如果緩衝區是空的，就讓 CPU 進入休眠 (hlt)，直到下一次中斷 (按鍵) 喚醒它！
    while (kbd_head == kbd_tail) {
        // 【關鍵修復】解除中斷封印！
        // Syscall 預設會關閉中斷，我們必須手動開啟 (sti)，否則 CPU 會永遠睡死在 hlt！
        __asm__ volatile("sti");

        // 進入休眠，等待按鍵中斷喚醒
        __asm__ volatile("hlt");
    }

    char c = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUFFER_SIZE;
    return c;
}
