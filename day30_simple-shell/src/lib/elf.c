#include "elf.h"
#include "tty.h"
#include "paging.h"
#include "pmm.h"
#include "utils.h"

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

uint32_t elf_load(elf32_ehdr_t* ehdr) {
    // 1. 使用我們寫好的檢查函式
    if (!elf_check_supported(ehdr)) {
        return 0;
    }

    // 2. 找到 Program Header Table 的起點 (使用簡化版的 phoff)
    elf32_phdr_t* phdr = (elf32_phdr_t*) ((uint8_t*)ehdr + ehdr->phoff);

    // 3. 掃描每一個 Program Header (使用簡化版的 phnum)
    for (int i = 0; i < ehdr->phnum; i++) {

        // 如果這個區段標記為 PT_LOAD (需要被載入到記憶體)
        if (phdr[i].type == 1) {
            uint32_t virt_addr = phdr[i].vaddr;
            uint32_t mem_size = phdr[i].memsz;
            uint32_t file_size = phdr[i].filesz;
            uint32_t file_offset = phdr[i].offset;

            // 分配並映射記憶體分頁
            uint32_t num_pages = (mem_size + 4095) / 4096;
            for (uint32_t j = 0; j < num_pages; j++) {
                // 強制轉型消除 warning
                uint32_t phys_addr = (uint32_t) pmm_alloc_page();
                map_page(virt_addr + (j * 4096), phys_addr, 7);
            }

            // 【終極修復點：複製資料】
            uint8_t* src = (uint8_t*)ehdr + file_offset;
            uint8_t* dest = (uint8_t*)virt_addr;

            // 複製檔案裡實際有的資料
            memcpy(dest, src, file_size);

            // 將未初始化的變數 (BSS 段) 清零
            if (mem_size > file_size) {
                memset(dest + file_size, 0, mem_size - file_size);
            }
        }
    }

    return ehdr->entry; // 回傳進入點
}
