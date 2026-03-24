修改了 grub.cfg, kernel.c 後問題還是存在，然後我發現用 `write file1 data` 後，檔案也是沒有被寫出來，我猜是否在 filesystem 有什麼問題？

底下是相關的 code: lib/simplefs.c, lib/vfs.c, lib/include/vfs.h lib/pmm.c

---
lib/simplefs.c
```c
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


// [Day47] add -- start
// 【新增】建立目錄的底層實作
int simplefs_mkdir(uint32_t part_lba, char* dirname) {
    uint8_t* dir_buffer = (uint8_t*) kmalloc(4096);
    ata_read_sector(part_lba + 1, dir_buffer);

    sfs_file_entry_t* entries = (sfs_file_entry_t*)dir_buffer;
    int max_entries = 512 / sizeof(sfs_file_entry_t); // 512 / 64 = 8 個項目

    for (int i = 0; i < max_entries; i++) {
        if (entries[i].filename[0] == '\0') { // 找到空位了！
            strcpy(entries[i].filename, dirname);
            entries[i].file_size = 0;
            entries[i].type = FS_DIR; // 標記為目錄！

            // 給它分配一個新的磁區來存放它未來的子檔案清單
            // (簡單起見，我們借用一個尚未使用的 LBA，例如 part_lba + 20 + i)
            entries[i].start_lba = part_lba + 20 + i;

            ata_write_sector(part_lba + 1, dir_buffer); // 寫回硬碟
            kfree(dir_buffer);
            return 0; // 成功
        }
    }
    kfree(dir_buffer);
    return -1; // 目錄滿了
}

// VFS 封裝
int vfs_mkdir(char* dirname) {
    if (mounted_part_lba == 0) return -1;
    return simplefs_mkdir(mounted_part_lba, dirname);
}
// [Day47] add -- end
```
---
lib/vfs.c
```c
// VFS: Virtual File System
#include "vfs.h"
#include "tty.h"
#include "utils.h"

// 宣告全域的根節點 (目前還是空的，等具體檔案系統掛載後才會賦值)
fs_node_t *fs_root = 0;

// --- 公開 API ---

void vfs_open(fs_node_t *node) {
    if (node->open != 0) {
        node->open(node);
    }
}

void vfs_close(fs_node_t *node) {
    if (node->close != 0) {
        node->close(node);
    }
}

// VFS 通用讀取函式
uint32_t vfs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    // 檢查這個節點有沒有實作讀取功能
    if (node->read != 0) {
        return node->read(node, offset, size, buffer);
    } else {
        kprintf("[VFS] Error: Node '%s' does not support read operation.\n", node->name);
        return 0;
    }
}

// VFS 通用寫入函式
uint32_t vfs_write(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (node->write != 0) {
        return node->write(node, offset, size, buffer);
    } else {
        kprintf("[VFS] Error: Node '%s' does not support write operation.\n", node->name);
        return 0;
    }
}
```

---
lib/include/vfs.h
```c
// VFS: Virtual File System
#ifndef VFS_H
#define VFS_H

#include <stdint.h>

// 定義節點的類型 (這就是 Linux "萬物皆檔案" 的基礎)
#define FS_FILE        0x01   // 一般檔案
#define FS_DIRECTORY   0x02   // 目錄
#define FS_CHARDEVICE  0x03   // 字元裝置 (如鍵盤、終端機)
#define FS_BLOCKDEVICE 0x04   // 區塊裝置 (如硬碟)
#define FS_PIPE        0x05   // 管線 (Pipe)
#define FS_MOUNTPOINT  0x08   // 掛載點

// 前置宣告，因為函式指標會用到自己
struct fs_node;

// 定義「讀取」與「寫入」的函式指標型別 (這就是我們的 Interface)
// 參數: 節點指標, 偏移量(offset), 讀寫大小(size), 緩衝區(buffer)
typedef uint32_t (*read_type_t)(struct fs_node*, uint32_t, uint32_t, uint8_t*);
typedef uint32_t (*write_type_t)(struct fs_node*, uint32_t, uint32_t, uint8_t*);
typedef void (*open_type_t)(struct fs_node*);
typedef void (*close_type_t)(struct fs_node*);

// VFS 核心節點結構 (類似 Linux 的 inode)
typedef struct fs_node {
    char name[128];     // 檔案名稱
    uint32_t flags;     // 節點類型 (FS_FILE, FS_DIRECTORY 等)
    uint32_t length;    // 檔案大小 (bytes)
    uint32_t inode;     // 該檔案系統專用的 ID
    uint32_t impl;      // 給底層驅動程式偷藏資料用的數字

    // [多型魔法] 綁定在這個檔案上的操作函式！
    read_type_t read;
    write_type_t write;
    open_type_t open;
    close_type_t close;

    struct fs_node *ptr; // 用來處理掛載點或符號連結
} fs_node_t;

// 系統全域的「根目錄」節點
extern fs_node_t *fs_root;

// VFS 暴露給 Kernel 其他模組呼叫的通用 API
void vfs_open(fs_node_t *node);
void vfs_close(fs_node_t *node);

uint32_t vfs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
uint32_t vfs_write(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);

#endif
```


---

lib/pmm.c
```c
// PMM: Physical Memory Management
#include "pmm.h"
#include "utils.h"
#include "tty.h"

// 假設我們最多支援 4GB RAM (總共 1,048,576 個 Frames)
// 1048576 bits / 32 bits (一個 uint32_t) = 32768 個陣列元素
#define BITMAP_SIZE 32768
uint32_t memory_bitmap[BITMAP_SIZE];

uint32_t max_frames = 0; // 系統實際擁有的可用 Frame 數量

// --- 內部輔助函式：操作特定的 Bit ---

// 將第 frame_idx 個 bit 設為 1 (代表已使用)
static void bitmap_set(uint32_t frame_idx) {
    uint32_t array_idx = frame_idx / 32;
    uint32_t bit_offset = frame_idx % 32;
    memory_bitmap[array_idx] |= (1 << bit_offset);
}

// 將第 frame_idx 個 bit 設為 0 (代表釋放)
static void bitmap_clear(uint32_t frame_idx) {
    uint32_t array_idx = frame_idx / 32;
    uint32_t bit_offset = frame_idx % 32;
    memory_bitmap[array_idx] &= ~(1 << bit_offset);
}

// 檢查第 frame_idx 個 bit 是否為 1
static bool bitmap_test(uint32_t frame_idx) {
    uint32_t array_idx = frame_idx / 32;
    uint32_t bit_offset = frame_idx % 32;
    return (memory_bitmap[array_idx] & (1 << bit_offset)) != 0;
}

// 尋找第一個為 0 的 bit (第一塊空地)
static int bitmap_find_first_free() {
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        // 如果這個整數不是 0xFFFFFFFF (代表裡面至少有一個 bit 是 0)
        if (memory_bitmap[i] != 0xFFFFFFFF) {
            for (uint32_t j = 0; j < 32; j++) {
                uint32_t frame_idx = i * 32 + j;
                if (frame_idx >= max_frames) return -1; // 已經超出實際記憶體容量

                if (!bitmap_test(frame_idx)) {
                    return frame_idx;
                }
            }
        }
    }
    return -1; // 記憶體全滿 (Out of Memory)
}

// --- 公開 API ---

void init_pmm(uint32_t mem_size_kb) {
    // 總 KB 數 / 4 = 總共有多少個 4KB 的 Frames
    max_frames = mem_size_kb / 4;

    // 一開始先把所有的記憶體都標記為「已使用」(填滿 1)
    // 這是為了安全，避免我們不小心分配到不存在的硬體空間
    memset(memory_bitmap, 0xFF, sizeof(memory_bitmap));

    // 然後，我們只把「真實存在」的記憶體標記為「可用」(設為 0)
    for (uint32_t i = 0; i < max_frames; i++) {
        bitmap_clear(i);
    }

    // [極度重要] 保留系統前 4MB (0 ~ 1024 個 Frames) 不被分配！
    // 因為這 4MB 已經被我們的 Kernel 程式碼、VGA 記憶體、IDT/GDT 給佔用了
    for (uint32_t i = 0; i < 1024; i++) {
        bitmap_set(i);
    }
}

void* pmm_alloc_page(void) {
    int free_frame = bitmap_find_first_free();
    if (free_frame == -1) {
        kprintf("PANIC: Out of Physical Memory!\n");
        return NULL; // OOM
    }

    // 標記為已使用
    bitmap_set(free_frame);

    // 計算出實際的物理位址 (Frame 索引 * 4096)
    uint32_t phys_addr = free_frame * PMM_FRAME_SIZE;
    return (void*)phys_addr;
}

void pmm_free_page(void* ptr) {
    uint32_t phys_addr = (uint32_t)ptr;
    uint32_t frame_idx = phys_addr / PMM_FRAME_SIZE;

    bitmap_clear(frame_idx);
}
```
