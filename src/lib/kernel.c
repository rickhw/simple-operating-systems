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
#include "syscall.h"
#include "elf.h"
#include "multiboot.h" // [新增]


// void kernel_main(void) {
void kernel_main(uint32_t magic, multiboot_info_t* mbd) {// [修改] 接收 boot.S 傳來的參數
    terminal_initialize();
    kprintf("=== OS Kernel Booting ===\n");

    // 1. 驗證 GRUB 魔法數字
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        kprintf("PANIC: Invalid Multiboot Magic Number!\n");
        return;
    }

    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384);
    init_kheap();
    init_syscalls();

    uint32_t current_esp;
    __asm__ volatile ("mov %%esp, %0" : "=r" (current_esp));
    set_kernel_stack(current_esp);

    kprintf("Kernel initialized.\n\n");

    // === Day 19: 簽收 GRUB 模組 ===
    kprintf("--- GRUB Multiboot Delivery ---\n");
    kprintf("MBI Flags: [0x%x]\n", mbd->flags); // [新增] 看看 GRUB 到底給了什麼權限

    // 檢查 MBI 結構的 flags 第 3 個 bit (代表模組資訊是否有效)
    if (mbd->flags & (1 << 3)) {
        kprintf("Modules count: [%d]\n", mbd->mods_count);

        if (mbd->mods_count > 0) {
            // 取得模組陣列的指標
            multiboot_module_t* mod = (multiboot_module_t*)mbd->mods_addr;

            kprintf("Module 1 loaded at physical address: [0x%x] to [0x%x]\n", mod->mod_start, mod->mod_end);
            kprintf("Module size: [%d] bytes\n", mod->mod_end - mod->mod_start);

            // [精彩時刻] 把這塊實體記憶體當作 ELF 檔案，交給昨天的解析器！
            elf32_ehdr_t* real_app = (elf32_ehdr_t*)mod->mod_start;

            kprintf("\nPassing module to ELF Parser...\n");
            elf_check_supported(real_app);
        }
    } else {
        kprintf("No modules were loaded by GRUB.\n");
    }

    __asm__ volatile ("sti");
    kprintf("\nSystem is ready.\n> ");

    while (1) { __asm__ volatile ("hlt"); }

}
