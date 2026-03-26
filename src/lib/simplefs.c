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

// 【新增】提供給 Syscall 呼叫的友善封裝
int vfs_create_file(char* filename, char* content) {
    if (mounted_part_lba == 0) return -1;

    int len = strlen(content);
    // 呼叫我們之前寫好的底層建立檔案功能
    simplefs_create_file(mounted_part_lba, filename, content, len);
    return 0;
}


// [Day45] add -- start
// 【新增】讀取第 index 個檔案的資訊
int simplefs_readdir(uint32_t part_lba, int index, char* out_name, uint32_t* out_size) {
    // 假設我們的根目錄位在 part_lba + 1 (這取決於你當時 simplefs_format 的設計)
    // 通常根目錄就是一個 block，我們把它讀出來
    uint8_t* dir_buffer = (uint8_t*) kmalloc(4096);
    ata_read_sector(part_lba + 1, dir_buffer); // 讀取第一個目錄磁區

    // 【轉換】直接把一整塊記憶體轉型成結構陣列！
    sfs_file_entry_t* entries = (sfs_file_entry_t*)dir_buffer;
    int max_entries = 512 / sizeof(sfs_file_entry_t);

    int valid_count = 0;
    for (int i = 0; i < max_entries; i++) {
        if (entries[i].filename[0] != '\0') {
            if (valid_count == index) {
                // 現在編譯器會精準抓到正確的 offset，再也不會讀到隔壁的字串了！
                for(int j=0; j<32; j++) out_name[j] = entries[i].filename[j];
                *out_size = entries[i].file_size;

                kfree(dir_buffer);
                return 1;
            }
            valid_count++;
        }
    }
    kfree(dir_buffer);
    return 0;
}

// 【新增】VFS 層的封裝
int vfs_readdir(int index, char* out_name, uint32_t* out_size) {
    if (mounted_part_lba == 0) return -1;
    return simplefs_readdir(mounted_part_lba, index, out_name, out_size);
}

// [Day45] add -- end


// [Day46] add -- start
// 【新增】實作檔案刪除邏輯
int simplefs_delete_file(uint32_t part_lba, char* filename) {
    uint8_t* dir_buffer = (uint8_t*) kmalloc(4096);
    ata_read_sector(part_lba + 1, dir_buffer); // 讀取目錄磁區

    sfs_file_entry_t* entries = (sfs_file_entry_t*)dir_buffer;
    int max_entries = 512 / sizeof(sfs_file_entry_t);

    for (int i = 0; i < max_entries; i++) {
        // 如果這個格子有檔案，而且名字完全符合
        if (entries[i].filename[0] != '\0' && strcmp(entries[i].filename, filename) == 0) {
            // 【死亡宣告】把檔名第一個字元變成 0，代表這個格子空出來了！
            entries[i].filename[0] = '\0';

            // 【極度重要】把修改後的目錄，重新寫回實體硬碟！
            ata_write_sector(part_lba + 1, dir_buffer);

            kfree(dir_buffer);
            return 0; // 刪除成功
        }
    }

    kfree(dir_buffer);
    return -1; // 找不到檔案
}

// 【新增】VFS 層的封裝
int vfs_delete_file(char* filename) {
    if (mounted_part_lba == 0) return -1;
    return simplefs_delete_file(mounted_part_lba, filename);
}
// [Day46] add -- end
