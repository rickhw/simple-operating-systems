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

    // Start of Day26
    // 解析 MBR 取得分區
    uint32_t part_lba = parse_mbr();

    if (part_lba != 0) {
        // 為了確保環境乾淨，我們先重新格式化一次
        simplefs_format(part_lba, 10000);

        // === Day 26: 建立檔案測試 ===
        kprintf("\n[Test] Writing files to disk...\n");

        char* data1 = "This is the content of the very first file ever created on Simple OS!";
        simplefs_create_file(part_lba, "hello.txt", data1, 70); // 假設長度約 70 bytes

        char* data2 = "OS config: vga_mode=text, ring3_enabled=true, fs=simplefs";
        simplefs_create_file(part_lba, "config.sys", data2, 58);

        // 呼叫 ls 功能！
        simplefs_list_files(part_lba);

    } else {
        kprintf("No valid partition found on disk.\n");
    }
    // End of Day26

    kprintf("\nSystem is ready.\n> ");
    while (1) { __asm__ volatile ("hlt"); }
}
