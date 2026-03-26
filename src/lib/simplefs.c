#include "simplefs.h"
#include "ata.h"
#include "tty.h"
#include "kheap.h"
#include "utils.h"

uint32_t mounted_part_lba = 0; // 記錄目前掛載的分區起點


// --- 公開 API ---

void simplefs_mount(uint32_t part_lba) {
    mounted_part_lba = part_lba;
    kprintf("[SimpleFS] Mounted at LBA [%d].\n", part_lba);
}

void simplefs_format(uint32_t partition_start_lba, uint32_t sector_count) {
    // kprintf("[SimpleFS] Formatting partition starting at LBA [%d].\n", partition_start_lba);

    // 1. 準備 Superblock
    sfs_superblock_t* sb = (sfs_superblock_t*) kmalloc(512);
    memset(sb, 0, 512);

    sb->magic = SIMPLEFS_MAGIC;
    sb->total_blocks = sector_count;
    sb->root_dir_lba = 1;     // 根目錄在分區的第 1 個相對 LBA (實體為 partition_start_lba + 1)
    sb->data_start_lba = 2;   // 資料區從第 2 個相對 LBA 開始

    // 寫入 Superblock 到分區的起點
    ata_write_sector(partition_start_lba, (uint8_t*)sb);
    // kprintf("[SimpleFS] 1) Superblock written; ");

    // 2. 準備空白的根目錄 (填滿 0 即可，代表沒有任何檔案)
    uint8_t* empty_dir = (uint8_t*) kmalloc(512);
    memset(empty_dir, 0, 512);

    // 寫入根目錄到 Superblock 的下一個磁區
    ata_write_sector(partition_start_lba + sb->root_dir_lba, empty_dir);
    // kprintf("2) Empty root directory created.\n");

    // 清理記憶體
    kfree(sb);
    kfree(empty_dir);

    kprintf("[SimpleFS] Format complete, at LBA [%d].\n", partition_start_lba);
}

// 列出檔案
void simplefs_list_files(uint32_t part_lba) {
    kprintf("\n[SimpleFS] List Root Directory\n");

    uint8_t* dir_buf = (uint8_t*) kmalloc(512);
    ata_read_sector(part_lba + 1, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;

    int file_count = 0;
    for (int i = 0; i < 16; i++) {
        if (entries[i].filename[0] != '\0') {
            kprintf("- [%s]  (Size: [%d] bytes, LBA: [%d])\n",
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

// [修改] 把原本的第一個參數拿掉，直接使用 mounted_part_lba
fs_node_t* simplefs_find(char* filename) {
    if (mounted_part_lba == 0) return 0; // 還沒掛載！

    uint8_t* dir_buf = (uint8_t*) kmalloc(512);
    ata_read_sector(mounted_part_lba + 1, dir_buf);
    // ... 下面的尋找邏輯完全不變，只需要把 part_lba 換成 mounted_part_lba

    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;
    for (int i = 0; i < 16; i++) {
        if (entries[i].filename[0] != '\0' && strcmp(entries[i].filename, filename) == 0) {
            fs_node_t* node = (fs_node_t*) kmalloc(sizeof(fs_node_t));
            strcpy(node->name, entries[i].filename);
            node->flags = 1;
            node->length = entries[i].file_size;
            node->inode = entries[i].start_lba;
            node->impl = mounted_part_lba;
            node->read = simplefs_read;
            node->write = 0;
            kfree(dir_buf);
            return node;
        }
    }
    kfree(dir_buf);
    return 0;
}

// 寫入檔案
int simplefs_create_file(uint32_t part_lba, char* filename, char* data, uint32_t size) {
    uint8_t* dir_buf = (uint8_t*) kmalloc(512);
    ata_read_sector(part_lba + 1, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;

    int empty_idx = -1;
    uint32_t next_data_lba = 2;

    for (int i = 0; i < 16; i++) {
        if (entries[i].filename[0] == '\0') {
            if (empty_idx == -1) empty_idx = i;
        } else {
            // [修改] 計算這個檔案佔用了幾個磁區 (無條件進位)
            uint32_t file_sectors = (entries[i].file_size + 511) / 512;
            uint32_t file_end_lba = entries[i].start_lba + file_sectors;
            if (file_end_lba > next_data_lba) {
                next_data_lba = file_end_lba;
            }
        }
    }

    if (empty_idx == -1) { kfree(dir_buf); return -1; }

    // 跨磁區寫入資料
    uint32_t sectors_needed = (size + 511) / 512;
    for (uint32_t i = 0; i < sectors_needed; i++) {
        uint8_t temp[512] = {0}; // 清空暫存區
        uint32_t copy_size = 512;
        if ((i * 512) + 512 > size) {
            copy_size = size - (i * 512); // 最後一個磁區可能裝不滿
        }
        memcpy(temp, data + (i * 512), copy_size);
        ata_write_sector(part_lba + next_data_lba + i, temp);
    }

    memset(entries[empty_idx].filename, 0, 24);
    strcpy(entries[empty_idx].filename, filename);
    entries[empty_idx].start_lba = next_data_lba;
    entries[empty_idx].file_size = size;

    ata_write_sector(part_lba + 1, dir_buf);
    kfree(dir_buf);
    return 0;
}

// 讀取大檔案
uint32_t simplefs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    uint32_t file_lba = node->impl + node->inode;
    uint32_t actual_size = size;
    if (offset >= node->length) return 0;
    if (offset + size > node->length) actual_size = node->length - offset;

    // 計算需要跨越的磁區範圍
    uint32_t start_sector = offset / 512;
    uint32_t end_sector = (offset + actual_size - 1) / 512;
    uint32_t sector_offset = offset % 512;

    uint8_t* temp_buf = (uint8_t*) kmalloc(512);
    uint32_t bytes_read = 0;

    for (uint32_t i = start_sector; i <= end_sector; i++) {
        ata_read_sector(file_lba + i, temp_buf);

        uint32_t copy_size = 512 - sector_offset;
        if (bytes_read + copy_size > actual_size) {
            copy_size = actual_size - bytes_read;
        }

        memcpy(buffer + bytes_read, temp_buf + sector_offset, copy_size);
        bytes_read += copy_size;
        sector_offset = 0; // 只有第一個磁區可能不是從 0 開始讀
    }

    kfree(temp_buf);
    return bytes_read;
}
