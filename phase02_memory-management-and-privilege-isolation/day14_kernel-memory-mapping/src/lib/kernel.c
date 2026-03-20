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

    /// -----------
    kprintf("Testing Virtual Memory Mapping...\n");

    // 1. 向地政事務所要一塊實體地皮 (通常會拿到 0x400000)
    void* phys_mem = pmm_alloc_page();
    kprintf("Got physical frame at: 0x%x\n", (uint32_t)phys_mem);

    // 2. 決定一個超遠的虛擬位址 (2GB 的位置)
    uint32_t virt_addr = 0x80000000;

    // 3. 施展虛實縫合魔法！(3 代表 Present | Read/Write)
    map_page(virt_addr, (uint32_t)phys_mem, 3);
    kprintf("Mapped Virtual 0x%x to Physical 0x%x\n", virt_addr, (uint32_t)phys_mem);

    // 4. 驗證時刻：直接對這個虛擬位址寫入字串！
    // 如果映射失敗，這裡會立刻觸發 Page Fault 當機重啟
    char* test_str = (char*)virt_addr;
    test_str[0] = 'H';
    test_str[1] = 'e';
    test_str[2] = 'l';
    test_str[3] = 'l';
    test_str[4] = 'o';
    test_str[5] = ' ';
    test_str[6] = 'V';
    test_str[7] = 'M';
    test_str[8] = '!';
    test_str[9] = '\0';

    // 5. 從虛擬位址把字串讀出來印在畫面上
    kprintf("Read from 0x80000000: %s\n", test_str);

    /// -----------

    __asm__ volatile ("sti");
    kprintf("\nSystem is ready. Start typing!\n> ");

    while (1) { __asm__ volatile ("hlt"); }
}
