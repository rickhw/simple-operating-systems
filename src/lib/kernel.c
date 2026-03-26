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
#include "ata.h"
#include "mbr.h"
#include "vfs.h"
#include "simplefs.h"
#include "elf.h"
#include "multiboot.h"

void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
// void kernel_main(void) {
    terminal_initialize();
    kprintf("=== OS Kernel Booting ===\n");

    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384);
    init_kheap();

    init_multitasking();    // [Day31][新增] 啟動多工作業

    __asm__ volatile ("sti");

    kprintf("Kernel subsystems initialized.\n\n");

    // Start of Day30
    // 宣告兩個無窮迴圈的任務函數
    extern void thread_A();
    extern void thread_B();

    // 建立兩個分身任務
    create_kernel_thread(thread_A);
    create_kernel_thread(thread_B);

    kprintf("Starting Multitasking Demo! (Press Ctrl+C to exit QEMU later)\n");
    // End of Day30

    kprintf("\nSystem is ready.\n> ");
    while (1) { __asm__ volatile ("hlt"); }
}

// 測試用執行緒 A
void thread_A() {
    while(1) {
        kprintf("Thread A; ");
        // 為了讓人眼能看清楚，我們用空迴圈稍微拖慢速度
        for(volatile int i = 0; i < 5000000; i++);
    }
}

// 測試用執行緒 B
void thread_B() {
    while(1) {
        kprintf("Thread B; ");
        for(volatile int i = 0; i < 5000000; i++);
    }
}
