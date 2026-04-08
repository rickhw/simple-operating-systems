出現 [VFS] Error, 這裡是不是有什麼東西缺了？

src/kernel/include/vfs.h
```c
// Virtual File System: Virtual File System
#ifndef VFS_H
#define VFS_H

#include <stdint.h>

// 定義節點的類型 (Linux "萬物皆檔案" 的基礎)
#define FS_FILE        0x01   // 一般檔案
#define FS_DIRECTORY   0x02   // 目錄
#define FS_CHARDEVICE  0x03   // 字元裝置 (如鍵盤、終端機)
#define FS_BLOCKDEVICE 0x04   // 區塊裝置 (如硬碟)
#define FS_PIPE        0x05   // 管線 (Pipe)
#define FS_MOUNTPOINT  0x08   // 掛載點

// 前置宣告，因為函式指標會用到自己
struct fs_node;

// 定義「讀取」與「寫入」的函式指標型別 (這就是 Interface)
// 參數: 節點指標, 偏移量(offset), 讀寫大小(size), 緩衝區(buffer)
typedef uint32_t (*read_type_t)(struct fs_node*, uint32_t, uint32_t, uint8_t*);
typedef uint32_t (*write_type_t)(struct fs_node*, uint32_t, uint32_t, uint8_t*);
typedef void (*open_type_t)(struct fs_node*);
typedef void (*close_type_t)(struct fs_node*);

// Virtual File System 核心節點結構 (類似 Linux 的 inode)
typedef struct fs_node {
    char name[128];     // 檔案名稱
    uint32_t flags;     // 節點類型 (FS_FILE, FS_DIRECTORY 等)
    uint32_t length;    // 檔案大小 (bytes)
    uint32_t inode;     // 該檔案系統專用的 ID
    uint32_t impl;      // 給底層驅動程式偷藏資料用的數字

    // [多型] 綁定在這個檔案上的操作函式！
    read_type_t read;
    write_type_t write;
    open_type_t open;
    close_type_t close;

    struct fs_node *ptr; // 用來處理掛載點或符號連結
} fs_node_t;

// 系統全域的「根目錄」節點
extern fs_node_t *fs_root;

// Virtual File System 暴露給 Kernel 其他模組呼叫的通用 API
void vfs_open(fs_node_t *node);
void vfs_close(fs_node_t *node);

uint32_t vfs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
uint32_t vfs_write(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);

int vfs_create_file(uint32_t dir_lba, char* filename, char* content);
int vfs_readdir(uint32_t dir_lba, int index, char* out_name, uint32_t* out_size, uint32_t* out_type);
int vfs_delete_file(uint32_t dir_lba, char* filename);
int vfs_mkdir(uint32_t dir_lba, char* dirname);
int vfs_getcwd(uint32_t dir_lba, char* buffer);

#endif
```

---

src/kernel/drivers/fs/vfs.c
```c
// Virtual File System: Virtual File System
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

// Virtual File System 通用讀取函式
uint32_t vfs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    // 檢查這個節點有沒有實作讀取功能
    if (node->read != 0) {
        return node->read(node, offset, size, buffer);
    } else {
        kprintf("[VFS] Error: Node '%s' does not support read operation.\n", node->name);
        return 0;
    }
}

// Virtual File System 通用寫入函式
uint32_t vfs_write(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (node->write != 0) {
        return node->write(node, offset, size, buffer);
    } else {
        kprintf("[VFS] Error: Node '%s' does not support write operation.\n", node->name);
        return 0;
    }
}
```
