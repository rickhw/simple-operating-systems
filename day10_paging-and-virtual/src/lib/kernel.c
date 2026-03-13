#include <stdint.h>
#include <stddef.h>
#include "tty.h"
#include "utils.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h" // [新增]

void kernel_main(void) {
    terminal_initialize();

    kprintf("=== OS Kernel Booting ===\n");

    init_gdt();
    kprintf("GDT loaded successfully.\n");

    // 初始化神經系統 IDT
    init_idt();
    kprintf("IDT loaded successfully.\n");

    // 啟動記憶體分頁
    init_paging();
    kprintf("Paging Enabled. Virtual Memory activated!\n");

    // [關鍵] 執行 sti (Set Interrupt Flag) 開啟全域中斷
    // 從這行開始，CPU 會開始接聽外部硬體的呼叫！
    __asm__ volatile ("sti");

    kprintf("Interrupts Enabled. Waiting for keyboard input...\n");

    // 讓核心進入無限休眠迴圈，有中斷來才會醒來做事
    while (1) {
        __asm__ volatile ("hlt");
    }
}
