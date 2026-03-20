#include <stdint.h>
#include <stddef.h>
#include "tty.h"
#include "utils.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "timer.h"
#include "pmm.h"
#include "kheap.h"
#include "syscall.h"

void kernel_main(void) {
    terminal_initialize();
    kprintf("=== OS Kernel Booting ===\n");

    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384);
    init_kheap();
    init_syscalls();    // [新增]

    __asm__ volatile ("sti");

    kprintf("Testing System Calls...\n");

    // 測試 Syscall 1：印出數字 99
    // 透過 inline assembly 塞入暫存器，並觸發 0x80 中斷
    __asm__ volatile (
        "mov $1, %%eax\n"     // eax = 1 (Syscall Number)
        "mov $99, %%ebx\n"    // ebx = 99 (arg1)
        "int $0x80\n"         // 敲打防彈玻璃！
        : : : "eax", "ebx"
    );

    // 測試 Syscall 2：印出字串
    char* my_msg = "Hello from Syscall!";
    __asm__ volatile (
        "mov $2, %%eax\n"              // eax = 2
        "mov %0, %%ebx\n"              // ebx = my_msg 的指標
        "int $0x80\n"
        : : "r" (my_msg) : "eax", "ebx"
    );

    kprintf("\nSystem is ready.\n> ");

    while (1) { __asm__ volatile ("hlt"); }
}
