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
#include "elf.h" // [新增]

void kernel_main(void) {
    terminal_initialize();
    kprintf("=== OS Kernel Booting ===\n");

    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384);
    init_kheap();
    init_syscalls();

    uint32_t current_esp;
    __asm__ volatile ("mov %%esp, %0" : "=r" (current_esp));
    set_kernel_stack(current_esp);

    kprintf("Kernel subsystems initialized.\n\n");

    // === Day 18: 測試 ELF 解析器 ===
    kprintf("--- ELF Parser Test ---\n");

    // 偽造一份長度為 52 bytes 的假 ELF 檔案 (全部填 0)
    uint8_t fake_file[52] = {0};

    // 把這塊記憶體強制轉型成 ELF 標頭結構，方便我們修改
    elf32_ehdr_t* fake_elf = (elf32_ehdr_t*)fake_file;

    // 填寫合法的資訊騙過檢查器
    fake_elf->magic = ELF_MAGIC;    // 0x7F 'E' 'L' 'F'
    fake_elf->ident[0] = 1;         // 32-bit
    fake_elf->type = 2;             // Executable
    fake_elf->machine = 3;          // x86
    fake_elf->entry = 0x08048000;   // 假設這是應用程式的進入點 (Linux 的標準起點)

    // 把假檔案丟給解析器檢查！
    elf_check_supported(fake_elf);

    // 試試看故意搞破壞
    kprintf("\nNow corrupting the file...\n");
    fake_elf->machine = 8; // 故意改成 8 (MIPS 架構)
    elf_check_supported(fake_elf);

    __asm__ volatile ("sti");
    kprintf("\nSystem is ready.\n> ");

    while (1) { __asm__ volatile ("hlt"); }
}
