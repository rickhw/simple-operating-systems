#include "mbr.h"
#include "ata.h"
#include "tty.h"
#include "utils.h"
#include "kheap.h"

// lib/mbr.h 與 lib/mbr.c 記得都要把型別改成 uint32_t parse_mbr(void);

uint32_t parse_mbr(void) {
    uint8_t* buffer = (uint8_t*) kmalloc(512);
    ata_read_sector(0, buffer);

    if (buffer[510] != 0x55 || buffer[511] != 0xAA) {
        kfree(buffer);
        return 0; // 找不到 MBR
    }

    mbr_partition_t* partitions = (mbr_partition_t*)(buffer + 446);
    uint32_t target_lba = 0;

    for (int i = 0; i < 4; i++) {
        if (partitions[i].sector_count > 0) {
            kprintf("Partition %d found at LBA %d\n", i + 1, partitions[i].lba_start);
            // 記錄第一個找到的合法分區
            if (target_lba == 0) {
                target_lba = partitions[i].lba_start;
                // 注意：這裡我們偷懶省略了傳遞 sector_count，
                // 實務上也可以用 struct 把 start 和 count 一起傳回去
            }
        }
    }

    kfree(buffer);
    return target_lba;
}

// void parse_mbr(void) {
//     kprintf("--- Reading Master Boot Record (LBA 0) ---\n");

//     // 配置 512 bytes 緩衝區並讀取 LBA 0
//     uint8_t* buffer = (uint8_t*) kmalloc(512);
//     ata_read_sector(0, buffer);

//     // 1. 檢查硬碟最後面是不是標準的 MBR 簽名 (0x55 0xAA)
//     if (buffer[510] != 0x55 || buffer[511] != 0xAA) {
//         kprintf("[MBR] Error: Invalid Boot Signature. Not an MBR disk.\n");
//         kfree(buffer);
//         return;
//     }

//     kprintf("[MBR] Valid Signature 0x55AA found!\n");

//     // 2. 指標魔法：把記憶體位址直接跳到第 446 byte，並強制轉型成我們定義的 Struct 陣列！
//     mbr_partition_t* partitions = (mbr_partition_t*)(buffer + 446);

//     // 3. MBR 最多只能有 4 個主要分區
//     for (int i = 0; i < 4; i++) {
//         // 如果 sector_count 是 0，代表這個分區是空的
//         if (partitions[i].sector_count == 0) {
//             continue;
//         }

//         uint32_t size_mb = (partitions[i].sector_count * 512) / (1024 * 1024);

//         kprintf("Partition %d:\n", i + 1);
//         kprintf("  -> Type: 0x%x\n", partitions[i].type);
//         kprintf("  -> Start LBA: %d\n", partitions[i].lba_start);
//         kprintf("  -> Size: %d Sectors (approx %d MB)\n", partitions[i].sector_count, size_mb);
//     }

//     kfree(buffer);
// }
