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
#include "elf.h" // 需要用到 ELF 解析
#include "multiboot.h" // 需要用到 MBI 結構

// 記得把 kernel_main 加上 multiboot 的參數
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

    // Start of Day28
    // 1. 啟動儲存裝置與掛載檔案系統
    uint32_t part_lba = parse_mbr();
    if (part_lba != 0) {
        simplefs_mount(part_lba);

        // 把這兩行加回來，幫這顆新硬碟建檔案！
        simplefs_format(part_lba, 10000);
        char* data1 = "This is the content of the very first file ever created on Simple OS!";
        simplefs_create_file(part_lba, "hello.txt", data1, 70);
    }

    // 2. 載入並啟動 GRUB 帶來的應用程式
    if (mbd->mods_count > 0) {
        multiboot_module_t* mod = (multiboot_module_t*)mbd->mods_addr;
        elf32_ehdr_t* real_app = (elf32_ehdr_t*)mod->mod_start;

        // kprintf("\nLoading ELF Application...\n");
        // uint32_t entry_point = elf_load(real_app);

        // if (entry_point != 0) {
        //     kprintf("Dropping to Ring 3...\n\n");
        //     extern void enter_user_mode(uint32_t user_function_ptr);
        //     enter_user_mode(entry_point);
        // }
        kprintf("\nLoading ELF Application...\n");
        uint32_t entry_point = elf_load(real_app);

        if (entry_point != 0) {
            kprintf("Allocating User Stack and Dropping to Ring 3...\n\n");

            uint32_t user_stack_phys = pmm_alloc_page();
            map_page(0x083FF000, user_stack_phys, 7);
            uint32_t user_stack_ptr = 0x083FF000 + 4096;

            // --- [新增] 記錄 Kernel Stack 到 TSS ---
            uint32_t current_kernel_esp;
            __asm__ volatile("mov %%esp, %0" : "=r"(current_kernel_esp));
            set_kernel_stack(current_kernel_esp);
            // -------------------------------------

            extern void enter_user_mode(uint32_t entry_point, uint32_t user_stack);
            enter_user_mode(entry_point, user_stack_ptr);
        }
    }
    // End of Day28

    kprintf("\nSystem is ready.\n> ");
    while (1) { __asm__ volatile ("hlt"); }
}
