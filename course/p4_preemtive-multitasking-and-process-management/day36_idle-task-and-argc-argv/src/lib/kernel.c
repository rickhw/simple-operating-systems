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
#include "task.h"
#include "multiboot.h"


void setup_filesystem(uint32_t part_lba, multiboot_info_t* mbd) {
    kprintf("[Kernel] Setting up SimpleFS environment...\n");
    simplefs_mount(part_lba);
    simplefs_format(part_lba, 10000);
    simplefs_create_file(part_lba, "hello.txt", "This is the content of the very first file ever created on Simple OS!\n", 70);

    // 【修改】動態讀取 GRUB 傳來的多個模組
    if (mbd->mods_count > 0) {
        multiboot_module_t* mod = (multiboot_module_t*)mbd->mods_addr;

        // 第一個模組必定是 Shell (my_app.elf)
        uint32_t app_size = mod[0].mod_end - mod[0].mod_start;
        kprintf("[Kernel] Installing Shell to HDD (Size: %d bytes)...\n", app_size);
        simplefs_create_file(part_lba, "my_app.elf", (char*)mod[0].mod_start, app_size);

        // 如果有第二個模組，那就是我們的 Echo 程式！
        if (mbd->mods_count > 1) {
            uint32_t echo_size = mod[1].mod_end - mod[1].mod_start;
            kprintf("[Kernel] Installing Echo to HDD (Size: %d bytes)...\n", echo_size);
            simplefs_create_file(part_lba, "echo.elf", (char*)mod[1].mod_start, echo_size);
        }
    }

    simplefs_list_files(part_lba);
    
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

    // [Day35] start
    // --- 應用程式載入與排程 ---
    kprintf("[Kernel] Fetching 'my_app.elf' from Virtual File System...\n");
    fs_node_t* app_node = simplefs_find("my_app.elf");

    if (app_node != 0) {
        uint8_t* app_buffer = (uint8_t*) kmalloc(app_node->length);
        vfs_read(app_node, 0, app_node->length, app_buffer);
        uint32_t entry_point = elf_load((elf32_ehdr_t*)app_buffer);

        if (entry_point != 0) {
            // kprintf("Creating ONE Initial User Task (Init Process)...\n\n");
            kprintf("Creating Initial Process...\n\n");

            init_multitasking();

            // 為唯一的 Shell 分配 User Stack
            uint32_t ustack1_phys = (uint32_t) pmm_alloc_page();
            map_page(0x083FF000, ustack1_phys, 7);

            // 建立 Ring 3 主任務
            create_user_task(entry_point, 0x083FF000 + 4096);

            kprintf("[Kernel] Kernel dropping to idle state (Ring0 -> Ring3).\n");

            // 【關鍵修復】Kernel 必須功成身退，把自己宣告死亡！
            // 否則排程器會以為 Kernel 還活著，就會一直把 CPU 切給 Kernel 的 hlt！
            exit_task();
        }
    } else {
        kprintf("[Kernel] Error: Shell (my_app.elf) not found on disk.\n");
    }
    // [Day35] end

    while (1) { __asm__ volatile ("hlt"); }
}
