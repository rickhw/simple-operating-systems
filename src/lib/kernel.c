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
#include "elf.h"
#include "multiboot.h"

void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
// void kernel_main(void) {
    terminal_initialize();
    kprintf("=== OS Kernel Booting ===\n");

    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384);
    init_kheap();
    __asm__ volatile ("sti");

    kprintf("Kernel subsystems initialized.\n\n");

    // Start of Day29
    uint32_t part_lba = parse_mbr();
    if (part_lba == 0) {
        kprintf("Disk Error: No partition found.\n");
        while(1) __asm__ volatile("hlt");
    }

    // 1. 掛載檔案系統並格式化 (為了確保乾淨的測試環境)
    simplefs_mount(part_lba);
    simplefs_format(part_lba, 10000);

    // 寫入 User App 會用到的文字檔
    simplefs_create_file(part_lba, "hello.txt", "This is the content of the very first file ever created on Simple OS!", 70);

    // 2. [系統安裝模式] 擷取 GRUB 帶來的 my_app.elf，並將其寫入實體硬碟！
    if (mbd->mods_count > 0) {
        multiboot_module_t* mod = (multiboot_module_t*)mbd->mods_addr;
        uint32_t app_size = mod->mod_end - mod->mod_start;
        kprintf("[Kernel] 'Installing' my_app.elf to Hard Drive (Size: %d bytes)...\n", app_size);
        simplefs_create_file(part_lba, "my_app.elf", (char*)mod->mod_start, app_size);
    }

    // 列出目前硬碟裡的檔案，應該要有 hello.txt 和 my_app.elf
    simplefs_list_files(part_lba);

    // 3. [自力更生模式] 透過 VFS 從硬碟讀取應用程式！
    kprintf("\n[Kernel] Fetching 'my_app.elf' from Virtual File System...\n");
    fs_node_t* app_node = simplefs_find("my_app.elf");

    if (app_node != 0) {
        // 在 Kernel Heap 分配一塊緩衝區來裝這個檔案
        uint8_t* app_buffer = (uint8_t*) kmalloc(app_node->length);
        vfs_read(app_node, 0, app_node->length, app_buffer);
        kprintf("[Kernel] ELF File loaded into RAM from HDD successfully.\n");

        // 將這塊裝有檔案內容的 buffer 交給 ELF 解析器
        uint32_t entry_point = elf_load((elf32_ehdr_t*)app_buffer);

        if (entry_point != 0) {
            kprintf("Allocating User Stack and Dropping to Ring 3...\n\n");
            uint32_t user_stack_phys = pmm_alloc_page();
            map_page(0x083FF000, user_stack_phys, 7);
            uint32_t user_stack_ptr = 0x083FF000 + 4096;

            uint32_t current_kernel_esp;
            __asm__ volatile("mov %%esp, %0" : "=r"(current_kernel_esp));
            set_kernel_stack(current_kernel_esp);

            extern void enter_user_mode(uint32_t entry_point, uint32_t user_stack);
            enter_user_mode(entry_point, user_stack_ptr);
        }
    } else {
        kprintf("[Kernel] Error: my_app.elf not found on disk.\n");
    }
    // End of Day29

    kprintf("\nSystem is ready.\n> ");
    while (1) { __asm__ volatile ("hlt"); }
}
