#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>

// GRUB 載入完畢後，eax 暫存器一定會存放這個魔法數字，用來證明它是合法的 Bootloader
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

// 模組 (Module) 的結構
typedef struct {
    uint32_t mod_start;  // 這個檔案被 GRUB 載入到的起始實體位址
    uint32_t mod_end;    // 這個檔案的結束位址
    uint32_t string;     // 模組名稱字串的位址
    uint32_t reserved;
} multiboot_module_t;

// Multiboot 資訊結構 (MBI)
// 這裡面包含非常多硬體資訊，我們目前只需要關心 flags, mods_count 和 mods_addr
typedef struct {
    uint32_t flags;        // 旗標，用來判斷哪些欄位是有效的
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;   // GRUB 幫我們載入了幾個模組 (檔案)
    uint32_t mods_addr;    // 模組資訊陣列的起始實體位址
    // ... 為了維持簡單，後面還有很多欄位我們先省略不寫 ...
} multiboot_info_t;

#endif
