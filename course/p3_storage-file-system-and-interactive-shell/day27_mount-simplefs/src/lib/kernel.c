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
    // uint32_t part_lba = parse_mbr();

    // if (part_lba != 0) {
    //     // 為了確保環境乾淨，我們先重新格式化一次
    //     simplefs_format(part_lba, 10000);

    //     // === Day 26: 建立檔案測試 ===
    //     kprintf("\n[Test] Writing files to disk...\n");

    //     char* data1 = "This is the content of the very first file ever created on Simple OS!";
    //     simplefs_create_file(part_lba, "hello.txt", data1, 70); // 假設長度約 70 bytes

    //     char* data2 = "OS config: vga_mode=text, ring3_enabled=true, fs=simplefs";
    //     simplefs_create_file(part_lba, "config.sys", data2, 58);

    //     // 呼叫 ls 功能！
    //     simplefs_list_files(part_lba);

    // } else {
    //     kprintf("No valid partition found on disk.\n");
    // }
    // End of Day26

    // Start of Day27
    uint32_t part_lba = parse_mbr();

    if (part_lba != 0) {
        // [注意] 這裡我們把 format 和 create 註解掉了！我們相信硬碟裡已經有資料了。
        // simplefs_format(part_lba, 10000);

        kprintf("\n--- VFS Integration Test ---\n");
        simplefs_list_files(part_lba); // 先印出來確認檔案還在

        // 1. 透過驅動程式尋找檔案，取得 VFS 節點
        kprintf("\n[Kernel] Asking SimpleFS to find 'hello.txt'...\n");
        fs_node_t* hello_node = simplefs_find(part_lba, "hello.txt");

        if (hello_node != 0) {
            kprintf("[Kernel] File found! VFS Node created at 0x%x\n", hello_node);
            kprintf("         Name: %s, Size: %d bytes\n", hello_node->name, hello_node->length);

            // 2. 準備緩衝區
            uint8_t* read_buf = (uint8_t*) kmalloc(128);
            memset(read_buf, 0, 128);

            // 3. [終極考驗] Kernel 透過高度抽象的 vfs_read 來讀取檔案！
            kprintf("[Kernel] Calling vfs_read()...\n");
            uint32_t bytes_read = vfs_read(hello_node, 0, hello_node->length, read_buf);

            kprintf("\n=== File Content ===\n");
            kprintf("%s\n", (char*)read_buf);
            kprintf("====================\n");
            kprintf("(Read %d bytes successfully)\n", bytes_read);

            kfree(read_buf);
            kfree(hello_node);
        } else {
            kprintf("Error: File not found.\n");
        }
    }
    // End of Day27

    kprintf("\nSystem is ready.\n> ");
    while (1) { __asm__ volatile ("hlt"); }
}
