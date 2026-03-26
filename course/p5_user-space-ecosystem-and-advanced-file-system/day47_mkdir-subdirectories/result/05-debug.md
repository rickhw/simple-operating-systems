
mkdir.elf 和 rm.elf 的問題修好了，回到 mkdir 這個功能的問題出現了。

1. `mkdir folder1`
2. `ls`

出現的 folder1 顯示是 [FILE] 類型, 應該是 [DIR]. 如圖.
我檢查相關的 code 看不出問題.

底下是 source code.

- lib/simplefs.c, lib/include/simplefs.h
- ls.c, mkdir.c
- libc/unistd.c, libc/include/unistd.h

---
lib/simplefs.c
```c
#include "simplefs.h"
#include "ata.h"
#include "tty.h"
#include "kheap.h"
#include "utils.h"

uint32_t mounted_part_lba = 0;

// 【擴建配置】
#define ROOT_DIR_SECTORS 8  // 根目錄佔用 8 個磁區
#define ROOT_DIR_BYTES   (ROOT_DIR_SECTORS * 512) // 4096 Bytes
#define MAX_FILES        (ROOT_DIR_BYTES / sizeof(sfs_file_entry_t)) // 64 個檔案！

// --- 內部輔助函數：讀寫整個根目錄 ---
static void read_root_dir(uint32_t part_lba, uint8_t* buffer) {
    for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
        ata_read_sector(part_lba + 1 + i, buffer + (i * 512));
    }
}
static void write_root_dir(uint32_t part_lba, uint8_t* buffer) {
    for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
        ata_write_sector(part_lba + 1 + i, buffer + (i * 512));
    }
}

// --- 公開 API ---

void simplefs_mount(uint32_t part_lba) {
    mounted_part_lba = part_lba;
    kprintf("[SimpleFS] Mounted at LBA [%d].\n", part_lba);
}

void simplefs_format(uint32_t partition_start_lba, uint32_t sector_count) {
    sfs_superblock_t* sb = (sfs_superblock_t*) kmalloc(512);
    memset(sb, 0, 512);

    sb->magic = SIMPLEFS_MAGIC;
    sb->total_blocks = sector_count;
    sb->root_dir_lba = 1;
    // 【修改】資料區從根目錄之後開始 (1 + 8 = 9)
    sb->data_start_lba = 1 + ROOT_DIR_SECTORS;

    ata_write_sector(partition_start_lba, (uint8_t*)sb);

    uint8_t* empty_dir = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    memset(empty_dir, 0, ROOT_DIR_BYTES);
    write_root_dir(partition_start_lba, empty_dir); // 寫入空白的巨型根目錄

    kfree(sb);
    kfree(empty_dir);
    kprintf("[SimpleFS] Format complete. Root Dir capacity: %d files.\n", MAX_FILES);
}

void simplefs_list_files(uint32_t part_lba) {
    kprintf("\n[SimpleFS] List Root Directory\n");
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_root_dir(part_lba, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;

    int file_count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] != '\0') {
            kprintf("- [%s]  (Size: [%d] bytes, LBA: [%d])\n",
                    entries[i].filename, entries[i].file_size, entries[i].start_lba);
            file_count++;
        }
    }
    if (file_count == 0) kprintf("(Directory is empty)\n");
    kprintf("-------------------------------\n");
    kfree(dir_buf);
}

fs_node_t* simplefs_find(char* filename) {
    if (mounted_part_lba == 0) return 0;
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_root_dir(mounted_part_lba, dir_buf);

    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;
    for (int i = 0; i < MAX_FILES; i++) {
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

int simplefs_create_file(uint32_t part_lba, char* filename, char* data, uint32_t size) {
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_root_dir(part_lba, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;

    int target_idx = -1;
    uint32_t next_data_lba = 1 + ROOT_DIR_SECTORS; // 避開根目錄

    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] == '\0') {
            if (target_idx == -1) target_idx = i;
        } else {
            if (strcmp(entries[i].filename, filename) == 0) target_idx = i; // 覆寫
            uint32_t file_sectors = (entries[i].file_size + 511) / 512;
            uint32_t file_end_lba = entries[i].start_lba + file_sectors;
            if (file_end_lba > next_data_lba) next_data_lba = file_end_lba;
        }
    }

    if (target_idx == -1) { kfree(dir_buf); return -1; }

    uint32_t sectors_needed = (size + 511) / 512;
    if (sectors_needed > 0) {
        for (uint32_t i = 0; i < sectors_needed; i++) {
            uint8_t temp[512] = {0};
            uint32_t copy_size = 512;
            if ((i * 512) + 512 > size) copy_size = size - (i * 512);
            memcpy(temp, data + (i * 512), copy_size);
            ata_write_sector(part_lba + next_data_lba + i, temp);
        }
    }

    memset(entries[target_idx].filename, 0, 32);
    strcpy(entries[target_idx].filename, filename);
    entries[target_idx].start_lba = next_data_lba;
    entries[target_idx].file_size = size;
    entries[target_idx].type = FS_FILE;

    write_root_dir(part_lba, dir_buf);
    kfree(dir_buf);
    return 0;
}

uint32_t simplefs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    uint32_t file_lba = node->impl + node->inode;
    uint32_t actual_size = size;
    if (offset >= node->length) return 0;
    if (offset + size > node->length) actual_size = node->length - offset;

    uint32_t start_sector = offset / 512;
    uint32_t end_sector = (offset + actual_size - 1) / 512;
    uint32_t sector_offset = offset % 512;

    uint8_t* temp_buf = (uint8_t*) kmalloc(512);
    uint32_t bytes_read = 0;

    for (uint32_t i = start_sector; i <= end_sector; i++) {
        ata_read_sector(file_lba + i, temp_buf);
        uint32_t copy_size = 512 - sector_offset;
        if (bytes_read + copy_size > actual_size) copy_size = actual_size - bytes_read;
        memcpy(buffer + bytes_read, temp_buf + sector_offset, copy_size);
        bytes_read += copy_size;
        sector_offset = 0;
    }
    kfree(temp_buf);
    return bytes_read;
}

int vfs_create_file(char* filename, char* content) {
    if (mounted_part_lba == 0) return -1;
    int len = strlen(content);
    return simplefs_create_file(mounted_part_lba, filename, content, len);
}

int simplefs_readdir(uint32_t part_lba, int index, char* out_name, uint32_t* out_size) {
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_root_dir(part_lba, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*)dir_buf;

    int valid_count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] != '\0') {
            if (valid_count == index) {
                for(int j=0; j<32; j++) out_name[j] = entries[i].filename[j];
                *out_size = entries[i].file_size;
                kfree(dir_buf);
                return 1;
            }
            valid_count++;
        }
    }
    kfree(dir_buf);
    return 0;
}

int vfs_readdir(int index, char* out_name, uint32_t* out_size) {
    if (mounted_part_lba == 0) return -1;
    return simplefs_readdir(mounted_part_lba, index, out_name, out_size);
}

int simplefs_delete_file(uint32_t part_lba, char* filename) {
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_root_dir(part_lba, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*)dir_buf;

    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] != '\0' && strcmp(entries[i].filename, filename) == 0) {
            entries[i].filename[0] = '\0';
            write_root_dir(part_lba, dir_buf);
            kfree(dir_buf);
            return 0;
        }
    }
    kfree(dir_buf);
    return -1;
}

int vfs_delete_file(char* filename) {
    if (mounted_part_lba == 0) return -1;
    return simplefs_delete_file(mounted_part_lba, filename);
}

int simplefs_mkdir(uint32_t part_lba, char* dirname) {
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_root_dir(part_lba, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*)dir_buf;

    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] == '\0') {
            strcpy(entries[i].filename, dirname);
            entries[i].file_size = 0;
            entries[i].type = FS_DIR;
            entries[i].start_lba = part_lba + 20 + i;

            write_root_dir(part_lba, dir_buf);
            kfree(dir_buf);
            return 0;
        }
    }
    kfree(dir_buf);
    return -1;
}

int vfs_mkdir(char* dirname) {
    if (mounted_part_lba == 0) return -1;
    return simplefs_mkdir(mounted_part_lba, dirname);
}
```

---
lib/inlcude/simplefs.h
```c
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

// 列出目錄下的所有檔案 (類似 ls 指令)
void simplefs_list_files(uint32_t part_lba);
fs_node_t* simplefs_find(char* filename);

// operate for files
// 建立檔案並寫入資料
// 為了簡單起見，我們這版先限制檔案大小不能超過 512 bytes (一個磁區)
int simplefs_create_file(uint32_t part_lba, char* filename, char* data, uint32_t size);
uint32_t simplefs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);

#endif
```

---
mkdir.c
```c
#include "stdio.h"
#include "unistd.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: mkdir <directory_name>\n");
        return 1;
    }

    char* dirname = argv[1];
    printf("[MKDIR] Creating directory '%s'...\n", dirname);

    int result = mkdir(dirname);

    if (result == 0) {
        printf("[MKDIR] Success!\n");
    } else {
        printf("[MKDIR] Failed. Directory might be full.\n");
    }

    return 0;
}
```

---
ls.c
```c
#include "stdio.h"
#include "unistd.h"

int main(int argc, char** argv) {
    printf("\n--- Simple OS Directory Listing ---\n");

    char name[32];
    int size = 0;
    int type = 0;
    int index = 0;

    while (readdir(index, name, &size, &type) == 1) {
        if (type == 1) {
            // 是目錄！用特殊的格式印出來
            printf("  [DIR]  %s \n", name);
        } else {
            // 是檔案
            printf("  [FILE] %s \t (Size: %d bytes)\n", name, size);
        }
        index++;
    }

    printf("-----------------------------------\n");
    printf("Total: %d items found.\n", index);
    return 0;
}
```

---
libc/unistd.c
```c
#include "unistd.h"
#include "syscall.h"

int fork() { return syscall(8, 0, 0, 0, 0, 0); }
int exec(char* filename, char** argv) { return syscall(9, (int)filename, (int)argv, 0, 0, 0); }
int wait(int pid) { return syscall(10, pid, 0, 0, 0, 0); }
void yield() { syscall(6, 0, 0, 0, 0, 0); }
void exit() { syscall(7, 0, 0, 0, 0, 0); while(1); } // 死迴圈防止意外返回

void send(char* msg) { syscall(11, (int)msg, 0, 0, 0, 0); }
int recv(char* buffer) { return syscall(12, (int)buffer, 0, 0, 0, 0); }

void* sbrk(int increment) {
    return (void*)syscall(13, increment, 0, 0, 0, 0); // 呼叫 Syscall 13
}

int create_file(const char* filename, const char* content) {
    return syscall(14, (int)filename, (int)content, 0, 0, 0);
}

int readdir(int index, char* out_name, int* out_size, int* out_type) {
    return syscall(15, index, (int)out_name, (int)out_size, (int)out_type, 0);
}

int remove(const char* filename) {
    return syscall(16, (int)filename, 0, 0, 0, 0);
}

int mkdir(const char* filename) {
    return syscall(17, (int)filename, 0, 0, 0, 0);
}
```

---
libc/include/unistd.h
```c
#ifndef _UNISTD_H
#define _UNISTD_H

int fork();
int exec(char* filename, char** argv);
int wait(int pid);
void yield();
void exit();
void send(char* msg);
int recv(char* buffer);
void* sbrk(int increment);

int create_file(const char* filename, const char* content);
int readdir(int index, char* out_name, int* out_size, int* out_type);
int remove(const char* filename);

int mkdir(const char* dirname);


#endif
```
