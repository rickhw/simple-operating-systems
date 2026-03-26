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
#include "vfs.h" // [新增]

// === 1. 撰寫一個假的底層驅動程式讀取邏輯 ===
uint32_t dummy_read_func(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    // 假裝我們從某個硬體讀出了資料
    char* msg = "Hello from the Dummy Device via VFS Magic!";
    memcpy(buffer, msg, 43);
    kprintf("[Dummy Driver] Read requested for node: %s (Offset: %d)\n", node->name, offset);
    return 43; // 回傳讀取的 bytes 數
}

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

    // === Day 24: VFS 抽象層測試 ===
    kprintf("--- VFS Abstraction Test ---\n");

    // 2. 建立一個虛擬節點 (把它想像成 /dev/dummy)
    fs_node_t* dummy_node = (fs_node_t*) kmalloc(sizeof(fs_node_t));
    memcpy(dummy_node->name, "dummy_device", 13);
    dummy_node->flags = FS_CHARDEVICE; // 標記為字元裝置

    // [關鍵魔法] 將 VFS 的 read 指標，綁定到我們寫好的 dummy_read_func！
    dummy_node->read = dummy_read_func;
    dummy_node->write = 0; // 不支援寫入

    // 3. 應用程式 (或 Kernel) 來要資料了！它不需要知道這是什麼裝置，統一呼叫 vfs_read
    uint8_t* read_buf = (uint8_t*) kmalloc(128);

    kprintf("Kernel calling vfs_read()...\n");
    uint32_t bytes_read = vfs_read(dummy_node, 0, 100, read_buf);

    read_buf[bytes_read] = '\0'; // 確保字串結尾
    kprintf("VFS returned: %s\n", (char*)read_buf);

    // 4. 測試呼叫不支援的 write
    kprintf("\nKernel calling vfs_write()...\n");
    vfs_write(dummy_node, 0, 10, read_buf); // 這應該要被 VFS 優雅地擋下來並印出 Error
    // End of Day24

    kprintf("\nSystem is ready.\n> ");
    while (1) { __asm__ volatile ("hlt"); }
}
