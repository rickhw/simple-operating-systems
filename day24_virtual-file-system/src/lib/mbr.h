#ifndef MBR_H
#define MBR_H

#include <stdint.h>

// 單個分區紀錄的結構 (固定 16 bytes)
typedef struct {
    uint8_t  status;       // 0x80 代表這是可開機分區，0x00 代表不可開機
    uint8_t  chs_first[3]; // 舊版的 CHS 起始位址 (現代硬碟已廢棄，忽略)
    uint8_t  type;         // 分區格式 (例如 0x83 是 Linux, 0x0B 是 FAT32)
    uint8_t  chs_last[3];  // 舊版的 CHS 結束位址 (忽略)
    uint32_t lba_start;    // [極度重要] 這個分區真正的起始 LBA 磁區號碼！
    uint32_t sector_count; // 這個分區總共有幾個磁區 (Size = count * 512 bytes)
} __attribute__((packed)) mbr_partition_t;

// 解析 MBR 的函式
void parse_mbr(void);

#endif
