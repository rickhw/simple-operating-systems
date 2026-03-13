#include <stdint.h>
#include <stddef.h>
#include "tty.h"
#include "utils.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "timer.h"
#include "pmm.h" // [新增]

void kernel_main(void) {
    terminal_initialize();
    kprintf("=== OS Kernel Booting ===\n");

    init_gdt();
    init_idt();
    init_paging();

    // 假設 GRUB 告訴我們系統有 16MB 的 RAM (16 * 1024 KB)
    // 實務上 GRUB 會透過 Multitool header 傳遞真實記憶體大小，這裡我們先 Hardcode
    init_pmm(16384);
    kprintf("Physical Memory Manager initialized.\n");

    // 測試：向系統索取兩塊 4KB 的記憶體
    void* page1 = pmm_alloc_page();
    void* page2 = pmm_alloc_page();

    // 印出它們的實體位址
    kprintf("Allocated Page 1 at: 0x%x\n", (uint32_t)page1);
    kprintf("Allocated Page 2 at: 0x%x\n", (uint32_t)page2);

    // 釋放第一塊，然後再索取一塊，看看會不會拿到剛剛釋放的！
    pmm_free_page(page1);
    kprintf("Freed Page 1.\n");

    void* page3 = pmm_alloc_page();
    kprintf("Allocated Page 3 at: 0x%x\n", (uint32_t)page3);

    // 我們先關閉 Timer 以免打擾畫面觀看
    // init_timer(100);

    __asm__ volatile ("sti");
    kprintf("System is ready.\n> ");

    while (1) { __asm__ volatile ("hlt"); }
}
