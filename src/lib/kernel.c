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


void kernel_main(void) {
    terminal_initialize();
    kprintf("=== OS Kernel Booting ===\n");

    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384);
    init_kheap();
    __asm__ volatile ("sti");

    kprintf("Kernel subsystems initialized.\n\n");

    // === Day 25: SimpleFS 格式化與掛載測試 ===
    kprintf("--- Initializing File System ---\n");

    // 1. 解析 MBR，取得第一塊分區的起點 (預期是 2048)
    uint32_t part_lba = parse_mbr();

    if (part_lba != 0) {
        // 2. 在這個分區上格式化 SimpleFS！(假設分區大小夠大，這裡偷懶塞個假 size)
        simplefs_format(part_lba, 10000);

        // 3. 驗證：立刻把剛才寫入的 Superblock 讀出來檢查魔法數字
        kprintf("\n[Verification] Reading Superblock from LBA %d...\n", part_lba);
        sfs_superblock_t* verify_sb = (sfs_superblock_t*) kmalloc(512);
        ata_read_sector(part_lba, (uint8_t*)verify_sb);

        if (verify_sb->magic == SIMPLEFS_MAGIC) {
            kprintf("[Verification] SUCCESS! SimpleFS Magic Number (0x%x) matches!\n", verify_sb->magic);
        } else {
            kprintf("[Verification] FAILED. Read magic: 0x%x\n", verify_sb->magic);
        }
        kfree(verify_sb);

    } else {
        kprintf("No valid partition found on disk.\n");
    }
    // End of Day25

    kprintf("\nSystem is ready.\n> ");
    while (1) { __asm__ volatile ("hlt"); }
}
