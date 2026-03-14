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
// #include "syscall.h"
// #include "elf.h"
// #include "multiboot.h"
#include "ata.h" // [Day21] add

void kernel_main(void) {
// void kernel_main(uint32_t magic, multiboot_info_t* mbd) {// [修改] 接收 boot.S 傳來的參數
    terminal_initialize();
    kprintf("=== OS Kernel Booting ===\n");

    // 驗證 GRUB 魔法數字
    // if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
    //     kprintf("PANIC: Invalid Multiboot Magic Number!\n");
    //     return;
    // }

    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384);
    init_kheap();
    // init_syscalls();
    __asm__ volatile ("sti"); // 開啟中斷

    kprintf("Kernel subsystems initialized.\n\n");

    // === Day 21: ATA PIO 讀取測試 ===
    kprintf("--- Hard Disk Read Test ---\n");

    // 配置一塊 512 bytes 的緩衝區來放資料
    uint8_t* sector_buffer = (uint8_t*) kmalloc(512);

    // 呼叫驅動程式，讀取第 0 號磁區 (LBA 0)
    kprintf("Reading LBA 0...\n");
    ata_read_sector(0, sector_buffer);

    // 把讀到的前 64 個字元當作字串印出來看看
    sector_buffer[64] = '\0'; // 加上結尾符號防爆
    kprintf("Data on disk: [%s]\n", (char*)sector_buffer);

    kfree(sector_buffer);

    kprintf("\nSystem is ready.\n> ");
    while (1) { __asm__ volatile ("hlt"); }
}
