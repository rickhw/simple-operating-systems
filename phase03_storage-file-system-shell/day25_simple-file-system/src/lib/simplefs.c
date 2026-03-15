// [Day25] add -- start
#include "simplefs.h"
#include "ata.h"
#include "tty.h"
#include "kheap.h"
#include "utils.h"

void simplefs_format(uint32_t partition_start_lba, uint32_t sector_count) {
    kprintf("[SimpleFS] Formatting partition starting at LBA %d...\n", partition_start_lba);

    // 1. 準備 Superblock
    sfs_superblock_t* sb = (sfs_superblock_t*) kmalloc(512);
    memset(sb, 0, 512);

    sb->magic = SIMPLEFS_MAGIC;
    sb->total_blocks = sector_count;
    sb->root_dir_lba = 1;     // 根目錄在分區的第 1 個相對 LBA (實體為 partition_start_lba + 1)
    sb->data_start_lba = 2;   // 資料區從第 2 個相對 LBA 開始

    // 寫入 Superblock 到分區的起點
    ata_write_sector(partition_start_lba, (uint8_t*)sb);
    kprintf("[SimpleFS] Superblock written.\n");

    // 2. 準備空白的根目錄 (填滿 0 即可，代表沒有任何檔案)
    uint8_t* empty_dir = (uint8_t*) kmalloc(512);
    memset(empty_dir, 0, 512);

    // 寫入根目錄到 Superblock 的下一個磁區
    ata_write_sector(partition_start_lba + sb->root_dir_lba, empty_dir);
    kprintf("[SimpleFS] Empty root directory created.\n");

    // 清理記憶體
    kfree(sb);
    kfree(empty_dir);

    kprintf("[SimpleFS] Format complete!\n");
}
// [Day25] add -- end
