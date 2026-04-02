#include <stdint.h>
#include <stddef.h>
#include "keyboard.h"
#include "io.h"
#include "tty.h"
#include "utils.h"
#include "task.h"

#define KBD_BUFFER_SIZE 256
volatile char kbd_buffer[KBD_BUFFER_SIZE];
volatile int kbd_head = 0;
volatile int kbd_tail = 0;

// 一般狀態的鍵盤對應表
unsigned char kbdus[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,
  '*',  0,  ' ',  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0, '-',   0,   0,   0, '+',   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

// 【Day 68 課前補充】按住 Shift 時的鍵盤對應表！
unsigned char kbdus_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
  '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',   0,
  '*',  0,  ' ',  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0, '-',   0,   0,   0, '+',   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

static int shift_pressed = 0;

// --- 公開 API ---

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);

    // 1. 攔截 Shift 的狀態改變
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
    }
    else if (scancode == 0xAA || scancode == 0xB6) {
        shift_pressed = 0;
    }
    // 2. 一般的按鍵按下 (Make Code)
    else if (!(scancode & 0x80)) {
        char ascii = 0;

        // 【修改】查表邏輯變得超級乾淨！直接根據 Shift 狀態選表！
        if (shift_pressed) {
            ascii = kbdus_shift[scancode];
        } else {
            ascii = kbdus[scancode];
        }

        // 把最終決定的字元塞進 Buffer 交給 User App
        if (ascii != 0) {
            extern int gui_handle_keyboard(char c); // 宣告外部的 GUI 路由器

            // 【Day 77 修改】先問 GUI 要不要攔截這個按鍵？
            if (gui_handle_keyboard(ascii)) {
                // GUI 吃掉了，什麼都不做！
            } else {
                // GUI 不要，走原本的 Terminal 邏輯
                kbd_buffer[kbd_head] = ascii;
                kbd_head = (kbd_head + 1) % KBD_BUFFER_SIZE;
            }
        }
    }

    // 通知 PIC 中斷處理完畢
    outb(0x20, 0x20);
}

char keyboard_getchar() {
    while (kbd_head == kbd_tail) {
        __asm__ volatile("sti");
        schedule();  // 讓出 CPU
        __asm__ volatile("hlt");
    }
    __asm__ volatile("cli");
    char c = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUFFER_SIZE;
    __asm__ volatile("sti");
    return c;
}
