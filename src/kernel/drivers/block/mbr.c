/**
 * @file src/kernel/drivers/block/mbr.c
 * @brief Main logic and program flow for mbr.c.
 *
 * This file handles the operations and logic associated with mbr.c.
 */

#include "mbr.h"
#include "ata.h"
#include "tty.h"
#include "utils.h"
#include "kheap.h"

// --- 公開 API ---

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
