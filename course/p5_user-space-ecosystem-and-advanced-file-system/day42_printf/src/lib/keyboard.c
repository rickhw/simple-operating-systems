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

// --- 公開 API ---

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);
    if (!(scancode & 0x80)) {
        char ascii = kbdus[scancode];
        if (ascii != 0) {
            kbd_buffer[kbd_head] = ascii;
            kbd_head = (kbd_head + 1) % KBD_BUFFER_SIZE;
        }
    }
    outb(0x20, 0x20);
}

char keyboard_getchar() {
    while (kbd_head == kbd_tail) {
        __asm__ volatile("sti");
        schedule();  // 【關鍵】讓出 CPU，讓多工作業順利流轉
        __asm__ volatile("hlt");
    }
    __asm__ volatile("cli");
    char c = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUFFER_SIZE;
    __asm__ volatile("sti");
    return c;
}
