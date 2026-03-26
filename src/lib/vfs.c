#include "vfs.h"
#include "tty.h"
#include "utils.h"

// 宣告全域的根節點 (目前還是空的，等具體檔案系統掛載後才會賦值)
fs_node_t *fs_root = 0;

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
