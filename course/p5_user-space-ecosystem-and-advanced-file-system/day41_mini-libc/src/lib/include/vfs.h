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
