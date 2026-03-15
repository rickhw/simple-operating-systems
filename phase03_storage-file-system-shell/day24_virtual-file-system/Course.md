看到你把基礎建設（Makefile）弄得這麼優雅，這完全是成熟系統工程師的風範！自動化建置是作業系統開發中保持心智健康的關鍵。

現在，我們已經有能力讀取硬碟，也知道怎麼找到分區（MBR）了。但在作業系統的世界裡，我們絕對不會讓應用程式直接下達「請去讀 LBA 2048 的第 5 個 byte」這種指令。我們需要目錄、檔名、權限，也就是所謂的 **「檔案 (File)」**。

但是，世界上有各種不同的檔案系統格式（FAT16, ext4, NTFS），甚至在 Linux 中，連鍵盤、螢幕、終端機都被當作「檔案」來處理 (`/dev/tty`)。為了讓 Kernel 能用**同一套標準**來操作所有東西，我們需要一層超強的設計模式。

歡迎來到 **Day 24：萬物皆檔案 — 虛擬檔案系統 (VFS) 抽象層**！

---

### Day 24 授課目標

1. **認識 VFS (Virtual File System)：** 它是所有具體檔案系統（FAT, ext2...）的「介面 (Interface)」。
2. **C 語言的物件導向 (Polymorphism)：** 作業系統的核心是用 C 寫的，沒有 `class` 或 `interface` 關鍵字。我們將使用 **「函式指標 (Function Pointers)」** 來實現多型！
3. **實作 VFS 核心結構：** 定義 `vfs_node_t`，並撰寫通用的 `vfs_read` 和 `vfs_write`。

---

### 理論揭秘：用 C 語言刻出「介面 (Interface)」

在 Java 或 Go 裡，你可能會定義一個 `Reader` 介面。在 Linux 核心中，這個介面叫做 `inode` (或是我們接下來要寫的 `fs_node`)。

這個結構體裡面不只有檔案的大小、名字，更重要的是它包含了 **函式指標**。
當應用程式呼叫 Syscall 要讀取檔案時，Kernel 不管這檔案是在硬碟、光碟還是記憶體裡，它只做一件事：`node->read(...)`。具體要怎麼讀，由掛載這個節點的底層驅動程式自己決定。

---

### 實作步驟：打造 VFS 介面

#### 1. 定義 VFS 結構 (`lib/vfs.h`)

請建立 **`lib/vfs.h`**。這裡我們要定義檔案節點的類型，以及最核心的 `fs_node` 結構。

```c
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
uint32_t vfs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
uint32_t vfs_write(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
void vfs_open(fs_node_t *node);
void vfs_close(fs_node_t *node);

#endif

```

#### 2. 實作 VFS 路由中心 (`lib/vfs.c`)

VFS 層本身「不實作」任何真正的讀寫邏輯，它只是一個**路由器**。它的工作是檢查該節點有沒有綁定對應的函式，有的話就幫忙呼叫。

建立 **`lib/vfs.c`**：

```c
#include "vfs.h"
#include "tty.h"

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

```

#### 3. 測試 VFS 多型架構 (`lib/kernel.c`)

我們現在還沒寫 FAT16，所以我們要寫一個「假驅動程式 (Dummy Device)」，把它掛載到 VFS 上，看看 VFS 能不能正確呼叫我們綁定的函式。

打開 **`lib/kernel.c`**：

```c
#include "tty.h"
#include "utils.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "timer.h"
#include "pmm.h"
#include "kheap.h"
#include "ata.h"
#include "mbr.h"
#include "vfs.h" // [新增]

// === 1. 撰寫一個假的底層驅動程式讀取邏輯 ===
uint32_t dummy_read_func(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    // 假裝我們從某個硬體讀出了資料
    char* msg = "Hello from the Dummy Device via VFS Magic!";
    memcpy(buffer, msg, 43);
    kprintf("[Dummy Driver] Read requested for node: %s (Offset: %d)\n", node->name, offset);
    return 43; // 回傳讀取的 bytes 數
}

void kernel_main(void) {
    terminal_initialize();
    kprintf("=== OS Kernel Booting ===\n");
    
    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384); 
    init_kheap();
    __asm__ volatile ("sti"); 
    
    kprintf("Kernel subsystems initialized.\n\n");

    // === Day 24: VFS 抽象層測試 ===
    kprintf("--- VFS Abstraction Test ---\n");

    // 2. 建立一個虛擬節點 (把它想像成 /dev/dummy)
    fs_node_t* dummy_node = (fs_node_t*) kmalloc(sizeof(fs_node_t));
    memcpy(dummy_node->name, "dummy_device", 13);
    dummy_node->flags = FS_CHARDEVICE; // 標記為字元裝置
    
    // [關鍵魔法] 將 VFS 的 read 指標，綁定到我們寫好的 dummy_read_func！
    dummy_node->read = dummy_read_func;
    dummy_node->write = 0; // 不支援寫入

    // 3. 應用程式 (或 Kernel) 來要資料了！它不需要知道這是什麼裝置，統一呼叫 vfs_read
    uint8_t* read_buf = (uint8_t*) kmalloc(128);
    
    kprintf("Kernel calling vfs_read()...\n");
    uint32_t bytes_read = vfs_read(dummy_node, 0, 100, read_buf);
    
    read_buf[bytes_read] = '\0'; // 確保字串結尾
    kprintf("VFS returned: %s\n", (char*)read_buf);

    // 4. 測試呼叫不支援的 write
    kprintf("\nKernel calling vfs_write()...\n");
    vfs_write(dummy_node, 0, 10, read_buf); // 這應該要被 VFS 優雅地擋下來並印出 Error

    kprintf("\nSystem is ready.\n> ");
    while (1) { __asm__ volatile ("hlt"); }
}

```

---

### 執行與驗證

編譯並執行（硬碟 `hdd.img` 繼續掛著沒關係，我們這次測試全在 RAM 裡）：

```bash
make clean && make run

```

**預期結果 🏆**
畫面上會精準地印出多型呼叫的過程：

```text
--- VFS Abstraction Test ---
Kernel calling vfs_read()...
[Dummy Driver] Read requested for node: dummy_device (Offset: 0)
VFS returned: Hello from the Dummy Device via VFS Magic!

Kernel calling vfs_write()...
[VFS] Error: Node 'dummy_device' does not support write operation.

```

看到這個畫面，代表你的系統已經成功解鎖了**驅動程式掛載**的能力！
這就是 Linux 中 `struct file_operations` 的核心思想。未來我們實作 FAT16 時，只要把 `fat16_read` 函式綁定上去，同一支 `vfs_read` API 就能無縫讀取隨身碟裡的照片了！

覺得這種用 C 語言實作抽象介面的手法巧妙嗎？這套 VFS 系統跑通後跟我說，我們明天準備向真正的檔案系統發起挑戰！
