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

    // === Day 22: ATA PIO 讀寫測試 ===
    kprintf("--- Hard Disk Write Test ---\n");

    // 準備兩塊 512 bytes 的緩衝區：一個用來寫，一個用來讀
    uint8_t* write_buffer = (uint8_t*) kmalloc(512);
    uint8_t* read_buffer  = (uint8_t*) kmalloc(512);

    // 把寫入緩衝區清空為 0
    memset(write_buffer, 0, 512);

    // 在寫入緩衝區填上我們的專屬印記
    char* msg = "Greetings from Simple OS! This data was WRITTEN by the Kernel!";
    memcpy(write_buffer, msg, 62); // 字串長度大約 62 bytes

    // 1. 寫入到 LBA 1 (第 2 個磁區)
    kprintf("Writing to LBA 1...\n");
    ata_write_sector(1, write_buffer);
    kprintf("Write completed.\n");

    // 2. 為了證明不是我們作弊，我們把 read_buffer 清空
    memset(read_buffer, 0, 512);

    // 3. 從 LBA 1 讀取剛才寫入的資料
    kprintf("Reading from LBA 1...\n");
    ata_read_sector(1, read_buffer);

    // 4. 印出來驗證！
    kprintf("Data on disk (LBA 1): %s\n", (char*)read_buffer);

    kfree(write_buffer);
    kfree(read_buffer);


    kprintf("\nSystem is ready.\n> ");
    while (1) { __asm__ volatile ("hlt"); }
}
