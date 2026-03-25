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
typedef struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;

    // ==========================================
    // 【新增】Multiboot Framebuffer 資訊
    // ==========================================
    uint64_t framebuffer_addr;   // 畫布的實體記憶體起點
    uint32_t framebuffer_pitch;  // 每一橫列的位元組數 (Bytes per line)
    uint32_t framebuffer_width;  // 畫布寬度 (Pixels)
    uint32_t framebuffer_height; // 畫布高度 (Pixels)
    uint8_t  framebuffer_bpp;    // 色彩深度 (Bits per pixel，例如 32)
    uint8_t  framebuffer_type;
    uint8_t  color_info[6];
} __attribute__((packed)) multiboot_info_t;

#endif
