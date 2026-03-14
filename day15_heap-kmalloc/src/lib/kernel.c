#include <stdint.h>
#include <stddef.h>
#include "tty.h"
#include "utils.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "timer.h"
#include "pmm.h"
#include "kheap.h" // [新增]

void kernel_main(void) {
    terminal_initialize();
    kprintf("=== OS Kernel Booting ===\n");

    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384);

    // [新增] 初始化 Kernel Heap
    init_kheap();
    kprintf("Kernel Heap initialized at 0xC0000000.\n");

    // 測試 1：要一個長度為 16 bytes 的字串空間
    char* str1 = (char*)kmalloc(16);
    // 測試 2：再要一個長度為 32 bytes 的字串空間
    char* str2 = (char*)kmalloc(32);

    kprintf("Allocated str1 at: 0x%x\n", (uint32_t)str1);
    kprintf("Allocated str2 at: 0x%x\n", (uint32_t)str2);

    // 寫入資料測試
    memcpy(str1, "Hello kmalloc!", 15);
    str1[15] = '\0';
    kprintf("Data in str1: %s\n", str1);

    // 釋放記憶體
    kfree(str1);
    kfree(str2);
    kprintf("Memory freed successfully.\n");

    __asm__ volatile ("sti");
    kprintf("\nSystem is ready.\n> ");

    while (1) { __asm__ volatile ("hlt"); }
}
