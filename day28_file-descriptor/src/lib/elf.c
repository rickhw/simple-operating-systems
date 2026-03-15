#include "elf.h"
#include "tty.h"
#include "utils.h"
#include "paging.h" // [Day20] add: 需要用到 map_page

// [Day20] add - start
// 回傳程式的 Entry Point
uint32_t elf_load(elf32_ehdr_t* header) {
    if (!elf_check_supported(header)) return 0;

    // 計算 Program Header Table 的起始實體位址
    elf32_phdr_t* phdr = (elf32_phdr_t*)((uint8_t*)header + header->phoff);

    // 遍歷所有 Program Headers
    for (int i = 0; i < header->phnum; i++) {
        // type == 1 代表這是 PT_LOAD (需要載入到記憶體的程式碼或資料)
        if (phdr[i].type == 1) {
            uint32_t virt_addr = phdr[i].vaddr;
            uint32_t phys_addr = (uint32_t)header + phdr[i].offset;

            kprintf("[Loader] Mapping Section: Virt 0x%x -> Phys 0x%x\n", virt_addr, phys_addr);

            // 施展魔法：把這個實體位址映射到應用程式期望的虛擬位址！
            // 為了簡單起見，我們假設程式小於 4KB，只映射一頁 (旗標 7 代表 User 可讀寫執行)
            map_page(virt_addr, phys_addr, 7);
        }
    }

    return header->entry;
}
// [Day20] add - end

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
    kprintf("[ELF] Entry Point is at: [0x%x]\n", header->entry);

    return true;
}
