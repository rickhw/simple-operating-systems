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
#include "task.h"       // Day 31/32 加入的多工作業
#include "multiboot.h"

// [重構] 將檔案系統的初始化與安裝過程獨立出來
void setup_filesystem(uint32_t part_lba, multiboot_info_t* mbd) {
    kprintf("[Kernel] Setting up SimpleFS environment...\n");

    // 1. 掛載與格式化
    simplefs_mount(part_lba);
    simplefs_format(part_lba, 10000);

    // 2. 建立測試文字檔
    simplefs_create_file(part_lba, "hello.txt", "This is the content of the very first file ever created on Simple OS!\n", 70);

    // 3. 模擬系統安裝：從 GRUB 模組把 my_app.elf 寫入實體硬碟
    if (mbd->mods_count > 0) {
        multiboot_module_t* mod = (multiboot_module_t*)mbd->mods_addr;
        uint32_t app_size = mod->mod_end - mod->mod_start;
        kprintf("[Kernel] 'Installing' Shell to HDD (Size: %d bytes)...\n", app_size);
        simplefs_create_file(part_lba, "my_app.elf", (char*)mod->mod_start, app_size);
    }

    // 印出目錄結構確認
    kprintf("\n--- SimpleFS Root Directory ---\n");
    simplefs_list_files(part_lba);
    kprintf("-------------------------------\n\n");
}

void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
    (void)magic; // 忽略未使用的警告

    terminal_initialize();
    kprintf("=== Simple OS Booting ===\n");

    // --- 系統底層基礎建設 ---
    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384);
    init_kheap();
    __asm__ volatile ("sti");

    kprintf("=== OS Subsystems Ready ===\n\n");

    // --- 儲存與檔案系統 ---
    uint32_t part_lba = parse_mbr();
    if (part_lba == 0) {
        kprintf("Disk Error: No partition found.\n");
        while(1) __asm__ volatile("hlt");
    }

    // 呼叫我們重構好的函式！
    setup_filesystem(part_lba, mbd);

    // --- 應用程式載入與排程 ---
    kprintf("[Kernel] Fetching 'my_app.elf' from Virtual File System...\n");
    fs_node_t* app_node = simplefs_find("my_app.elf");

    if (app_node != 0) {
        uint8_t* app_buffer = (uint8_t*) kmalloc(app_node->length);
        vfs_read(app_node, 0, app_node->length, app_buffer);
        uint32_t entry_point = elf_load((elf32_ehdr_t*)app_buffer);

        if (entry_point != 0) {
            kprintf("Creating TWO independent User Tasks (Ring 3)...\n\n");

            init_multitasking();

            // 為 App 1 分配專屬 User Stack (0x083FF000)
            // [修復 Warning] 加上 (uint32_t) 強制轉型
            uint32_t ustack1_phys = (uint32_t) pmm_alloc_page();
            map_page(0x083FF000, ustack1_phys, 7);

            // 為 App 2 分配專屬 User Stack
            // 【關鍵修復】從 0x084FF000 改為 0x083FE000！
            // 讓它和 App 1 待在同一個安全的 4MB 區域內，避開跨界分配的 Bug！
            uint32_t ustack2_phys = (uint32_t) pmm_alloc_page();
            map_page(0x083FE000, ustack2_phys, 7);

            // 建立兩個 Ring 3 任務
            create_user_task(entry_point, 0x083FF000 + 4096);
            create_user_task(entry_point, 0x083FE000 + 4096); // 對應新位址

            kprintf("Kernel dropping to idle state. Have fun!\n");
            schedule();
        }
    } else {
        kprintf("[Kernel] Error: Shell (my_app.elf) not found on disk.\n");
    }

    while (1) { __asm__ volatile ("hlt"); }
}
