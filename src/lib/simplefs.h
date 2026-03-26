// [Day25] add -- start
#ifndef SIMPLEFS_H
#define SIMPLEFS_H

#include <stdint.h>
#include "vfs.h" // [Day27][新增] 為了使用 fs_node_t

// SimpleFS 的魔法數字 ("SFS!" 的十六進位)
#define SIMPLEFS_MAGIC 0x21534653

// Superblock 結構 (佔用一個 512 bytes 磁區，但只用前面幾個 bytes)
typedef struct {
    uint32_t magic;         // 證明這是一個 SimpleFS 分區
    uint32_t total_blocks;  // 分區總容量 (以 512 bytes 磁區為單位)
    uint32_t root_dir_lba;  // 根目錄所在的相對 LBA
    uint32_t data_start_lba;// 檔案資料區開始的相對 LBA
    uint8_t  padding[496];  // 補齊到 512 bytes
} __attribute__((packed)) sfs_superblock_t;

// 檔案目錄項目結構 (剛好 32 bytes，一個 512 bytes 磁區可以塞 16 個檔案)
typedef struct {
    char     filename[24];  // 檔案名稱 (包含結尾 \0)
    uint32_t start_lba;     // 檔案內容開始的相對 LBA
    uint32_t file_size;     // 檔案大小 (Bytes)
} __attribute__((packed)) sfs_file_entry_t;

// 格式化指定的分區
void simplefs_format(uint32_t partition_start_lba, uint32_t sector_count);

// [Day26][新增] 建立檔案並寫入資料
// 為了簡單起見，我們這版先限制檔案大小不能超過 512 bytes (一個磁區)
int simplefs_create_file(uint32_t part_lba, char* filename, char* data, uint32_t size);

// [Day26][新增] 列出目錄下的所有檔案 (類似 ls 指令)
void simplefs_list_files(uint32_t part_lba);


// fs_node_t* simplefs_find(uint32_t part_lba, char* filename); // [Day27][新增] // [Day28] delete
fs_node_t* simplefs_find(char* filename); // [Day28] add

void simplefs_mount(uint32_t part_lba); // [Day28] add
uint32_t simplefs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);

#endif
// [Day25] add -- end
