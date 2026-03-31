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

unsigned char kbdus[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,
  '*',  0,  ' ',  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0, '-',   0,   0,   0, '+',   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

// 在檔案開頭宣告一個全域變數，記住 Shift 目前是否被按著
static int shift_pressed = 0;

// --- 公開 API ---

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);

    // 1. 攔截 Shift 的狀態改變 (只更新狀態，不塞入 Buffer)
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
    }
    else if (scancode == 0xAA || scancode == 0xB6) {
        shift_pressed = 0;
    }
    // 2. 如果是一般的按鍵按下 (Make Code，最高位為 0)
    else if (!(scancode & 0x80)) {
        char ascii = 0;

        if (scancode == 0x28) {
            // 如果是引號鍵，根據 Shift 狀態決定是雙引號或單引號
            ascii = shift_pressed ? '"' : '\'';
        } else {
            // 查表取得基本字元
            ascii = kbdus[scancode];

            // 💡 額外附贈：支援 Shift 輸出大寫字母！
            if (shift_pressed && ascii >= 'a' && ascii <= 'z') {
                ascii -= 32; // ASCII 碼中小寫轉大寫剛好差 32
            }
        }

        // 把最終決定的字元塞進 Buffer 交給 User App
        if (ascii != 0) {
            kbd_buffer[kbd_head] = ascii;
            kbd_head = (kbd_head + 1) % KBD_BUFFER_SIZE;
        }
    }

    // 【極度重要】不管剛剛發生什麼事，都必須通知 PIC 中斷處理完畢！
    outb(0x20, 0x20);
}

char keyboard_getchar() {
    while (kbd_head == kbd_tail) {
        __asm__ volatile("sti");
        schedule();  // 讓出 CPU，讓多工作業順利流轉
        __asm__ volatile("hlt");
    }
    __asm__ volatile("cli");
    char c = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUFFER_SIZE;
    __asm__ volatile("sti");
    return c;
}
