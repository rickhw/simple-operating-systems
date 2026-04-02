
#ifndef SIMPLEFS_H
#define SIMPLEFS_H

#include <stdint.h>
#include "vfs.h"

// SimpleFS 的魔法數字 ("SFS!" 的十六進位)
#define SIMPLEFS_MAGIC 0x21534653

// 定義檔案型態
#define FS_FILE 0
#define FS_DIR  1

// Superblock 結構 (佔用一個 512 bytes 磁區，但只用前面幾個 bytes)
typedef struct {
    uint32_t magic;         // 證明這是一個 SimpleFS 分區
    uint32_t total_blocks;  // 分區總容量 (以 512 bytes 磁區為單位)
    uint32_t root_dir_lba;  // 根目錄所在的相對 LBA
    uint32_t data_start_lba;// 檔案資料區開始的相對 LBA
    uint8_t  padding[496];  // 補齊到 512 bytes
} __attribute__((packed)) sfs_superblock_t;

// 檔案目錄項目結構
typedef struct {
    char     filename[32];  // 檔案名稱 (包含結尾 \0)
    uint32_t start_lba;     // 檔案內容開始的相對 LBA
    uint32_t file_size;     // 檔案大小 (Bytes)
    uint32_t type;         // 【新增】檔案型態：0=檔案, 1=目錄 (4 bytes)
    uint32_t reserved[5];  // 保留空間，湊滿 64 bytes 完美對齊 (20 bytes)
} __attribute__((packed)) sfs_file_entry_t;

// disk utils
// 格式化指定的分區
void simplefs_format(uint32_t partition_start_lba, uint32_t sector_count);
void simplefs_mount(uint32_t part_lba);
void simplefs_list_files(void);

int simplefs_create_file(uint32_t dir_lba_rel, char* filename, char* data, uint32_t size);
uint32_t simplefs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);

// 用相對目錄位址 (dir_lba_rel)
fs_node_t* simplefs_find(uint32_t dir_lba_rel, char* filename);

#endif
