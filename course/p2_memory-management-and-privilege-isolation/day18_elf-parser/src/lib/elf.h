#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#include <stdbool.h>

// ELF 檔案開頭永遠是這四個字元： 0x7F, 'E', 'L', 'F'
// 在 Little-Endian 系統中，這組合成一個 32-bit 的整數如下：
#define ELF_MAGIC 0x464C457F

// 32 位元 ELF 標頭結構
typedef struct {
    uint32_t magic;      // 魔法數字 (0x7F 'E' 'L' 'F')
    uint8_t  ident[12];  // 架構資訊 (32/64 bit, Endianness 等)
    uint16_t type;       // 檔案類型 (1=Relocatable, 2=Executable)
    uint16_t machine;    // CPU 架構 (3=x86)
    uint32_t version;    // ELF 版本
    uint32_t entry;      // [極度重要] 程式的進入點 (Entry Point) 虛擬位址！
    uint32_t phoff;      // Program Header Table 的偏移量
    uint32_t shoff;      // Section Header Table 的偏移量
    uint32_t flags;      // 架構特定標籤
    uint16_t ehsize;     // 這個 ELF Header 本身的大小
    uint16_t phentsize;  // 每個 Program Header 的大小
    uint16_t phnum;      // Program Header 的數量
    uint16_t shentsize;  // 每個 Section Header 的大小
    uint16_t shnum;      // Section Header 的數量
    uint16_t shstrndx;   // 字串表索引
} __attribute__((packed)) elf32_ehdr_t;

// 宣告我們的解析函式
bool elf_check_supported(elf32_ehdr_t* header);

#endif
