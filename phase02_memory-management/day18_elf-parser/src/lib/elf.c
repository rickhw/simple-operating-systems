#include "elf.h"
#include "tty.h"

// 檢查這是不是一個我們支援的 x86 32-bit ELF 執行檔
bool elf_check_supported(elf32_ehdr_t* header) {
    if (!header) return false;

    // 1. 檢查魔法數字 (Magic Number)
    if (header->magic != ELF_MAGIC) {
        kprintf("[ELF] Error: Invalid Magic Number. Not an ELF file.\n");
        return false;
    }

    // 2. 檢查是否為 32 位元 (ident[0] == 1 代表 32-bit)
    if (header->ident[0] != 1) {
        kprintf("[ELF] Error: Not a 32-bit ELF file.\n");
        return false;
    }

    // 3. 檢查是否為可執行檔 (Type 2 = Executable)
    if (header->type != 2) {
        kprintf("[ELF] Error: Not an executable file.\n");
        return false;
    }

    // 4. 檢查是否為 x86 架構 (Machine 3 = x86)
    if (header->machine != 3) {
        kprintf("[ELF] Error: Not compiled for x86 CPU.\n");
        return false;
    }

    kprintf("[ELF] Valid x86 32-bit Executable!\n");
    kprintf("[ELF] Entry Point is at: 0x%x\n", header->entry);

    return true;
}
