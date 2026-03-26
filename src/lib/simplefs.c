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

// [Day26] add -- start
// 建立檔案
int simplefs_create_file(uint32_t part_lba, char* filename, char* data, uint32_t size) {
    if (size > 512) {
        kprintf("[SimpleFS] Error: File too large! Max 512 bytes supported in v1.\n");
        return -1;
    }

    // 1. 讀取根目錄磁區 (相對 LBA 1)
    uint8_t* dir_buf = (uint8_t*) kmalloc(512);
    ata_read_sector(part_lba + 1, dir_buf);

    // 將 buffer 轉型為目錄項目陣列 (一個 512 bytes 磁區可放 16 個 32 bytes 的項目)
    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;

    int empty_idx = -1;
    uint32_t next_data_lba = 2; // 資料區從相對 LBA 2 開始

    // 2. 掃描目錄：尋找空位，並計算下一個可用的資料磁區
    for (int i = 0; i < 16; i++) {
        // 如果檔名的第一個字元是 \0，代表這格是空的
        if (entries[i].filename[0] == '\0') {
            if (empty_idx == -1) empty_idx = i; // 記住第一個找到的空位
        } else {
            // 如果這格有檔案，找出它佔用的 LBA，推算下一個空地在哪
            // (目前每個檔案只佔 1 個磁區，所以直接 + 1)
            uint32_t file_end_lba = entries[i].start_lba + 1;
            if (file_end_lba > next_data_lba) {
                next_data_lba = file_end_lba;
            }
        }
    }

    if (empty_idx == -1) {
        kprintf("[SimpleFS] Error: Root directory is full!\n");
        kfree(dir_buf);
        return -1;
    }

    kprintf("[SimpleFS] Creating file '%s' at relative LBA %d...\n", filename, next_data_lba);

    // 3. 將實際資料寫入分配到的 Data Block
    uint8_t* data_buf = (uint8_t*) kmalloc(512);
    memset(data_buf, 0, 512);     // 清除殘留垃圾
    memcpy(data_buf, data, size); // 複製真實資料
    ata_write_sector(part_lba + next_data_lba, data_buf);

    // 4. 更新目錄表上的檔案資訊
    memset(entries[empty_idx].filename, 0, 24);
    memcpy(entries[empty_idx].filename, filename, 24); // (注意：實務上需檢查檔名長度)
    entries[empty_idx].start_lba = next_data_lba;
    entries[empty_idx].file_size = size;

    // 5. 將更新後的目錄表寫回硬碟！
    ata_write_sector(part_lba + 1, dir_buf);

    kfree(dir_buf);
    kfree(data_buf);
    return 0;
}

// 列出檔案
void simplefs_list_files(uint32_t part_lba) {
    kprintf("\n--- SimpleFS Root Directory ---\n");

    uint8_t* dir_buf = (uint8_t*) kmalloc(512);
    ata_read_sector(part_lba + 1, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;

    int file_count = 0;
    for (int i = 0; i < 16; i++) {
        if (entries[i].filename[0] != '\0') {
            kprintf("[-] %s  (Size: %d bytes, LBA: %d)\n",
                    entries[i].filename,
                    entries[i].file_size,
                    entries[i].start_lba);
            file_count++;
        }
    }

    if (file_count == 0) {
        kprintf("(Directory is empty)\n");
    }
    kprintf("-------------------------------\n");

    kfree(dir_buf);
}
// [Day26] add -- end
