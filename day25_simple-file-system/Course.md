這幾天我們像是打通了任督二脈，硬體 I/O 與軟體抽象層（VFS）都已經準備就緒。

在分散式系統如 K8s 中，無論運算資源怎麼調度，最終都需要一個像 `etcd` 這樣的核心元件來儲存叢集的「絕對狀態與元資料 (Metadata)」。在硬碟的世界裡也是一樣的，面對廣闊無邊的磁區陣列，我們需要一個權威的「總目錄」來記錄哪個區塊存了什麼檔案。

歡迎來到 **Day 25：開天闢地 — 設計與格式化 SimpleFS (簡單檔案系統)**！

---

### Day 25 授課目標

1. **認識 Superblock (超級區塊)：** 這是檔案系統的 `etcd`。它永遠位在分區的第一個磁區，記錄了這個檔案系統的魔法數字、目錄位置與容量資訊。
2. **設計目錄結構：** 定義檔案實體 (File Entry)，包含檔名、大小與資料所在的 LBA。
3. **實作「格式化 (Format)」功能：** 在我們 Day 23 切出來的 LBA 2048 實體分區上，打下 SimpleFS 的地基！

---

### 實作步驟：定義與格式化 SimpleFS

為了保持系統的輕量與好理解，我們今天不實作龐大複雜的 FAT16，而是親手打造一個專屬的 **SimpleFS**。它的架構非常直觀：

* **第 1 個磁區 (LBA 0)：** Superblock（基本資訊）。
* **第 2 個磁區 (LBA 1)：** 根目錄表（存放檔案清單）。
* **後續磁區 (LBA 2+)：** 實際的檔案資料。

*(註：這裡的 LBA 0 是指「分區內」的相對偏移。如果分區從實體硬碟的 2048 開始，那 Superblock 就在實體的 2048。)*

#### 1. 定義 SimpleFS 資料結構 (`lib/simplefs.h`)

請建立 **`lib/simplefs.h`**。我們用精準的 C 語言結構來對齊 512 bytes 的磁區。

```c
#ifndef SIMPLEFS_H
#define SIMPLEFS_H

#include <stdint.h>

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

#endif

```

#### 2. 實作格式化邏輯 (`lib/simplefs.c`)

「格式化」聽起來很可怕，但對作業系統來說，格式化其實就只是**「把正確的 Superblock 和空白的目錄區塊寫入硬碟的特定位置」**而已。

建立 **`lib/simplefs.c`**：

```c
#include "simplefs.h"
#include "ata.h"
#include "tty.h"
#include "kheap.h"
#include "utils.h"

void simplefs_format(uint32_t partition_start_lba, uint32_t sector_count) {
    kprintf("[SimpleFS] Formatting partition starting at LBA %d...\n", partition_start_lba);

    // 1. 準備 Superblock
    sfs_superblock_t* sb = (sfs_superblock_t*) kmalloc(512);
    memset(sb, 0, 512);
    
    sb->magic = SIMPLEFS_MAGIC;
    sb->total_blocks = sector_count;
    sb->root_dir_lba = 1;     // 根目錄在分區的第 1 個相對 LBA (實體為 partition_start_lba + 1)
    sb->data_start_lba = 2;   // 資料區從第 2 個相對 LBA 開始

    // 寫入 Superblock 到分區的起點
    ata_write_sector(partition_start_lba, (uint8_t*)sb);
    kprintf("[SimpleFS] Superblock written.\n");

    // 2. 準備空白的根目錄 (填滿 0 即可，代表沒有任何檔案)
    uint8_t* empty_dir = (uint8_t*) kmalloc(512);
    memset(empty_dir, 0, 512);
    
    // 寫入根目錄到 Superblock 的下一個磁區
    ata_write_sector(partition_start_lba + sb->root_dir_lba, empty_dir);
    kprintf("[SimpleFS] Empty root directory created.\n");

    // 清理記憶體
    kfree(sb);
    kfree(empty_dir);
    
    kprintf("[SimpleFS] Format complete!\n");
}

```

#### 3. 將 MBR 解析與格式化串接 (`lib/kernel.c`)

昨天我們寫了 `parse_mbr()` 來印出分區資訊。今天我們稍微修改它，讓它回傳找到的第一個可用分區的 LBA，交給 SimpleFS 去格式化！

請打開 **`lib/mbr.c`**，把 `parse_mbr` 的回傳型別從 `void` 改成 `uint32_t`，並在找到 `Type: 0x83` (Linux 分區) 時回傳它的 LBA：

```c
// lib/mbr.h 與 lib/mbr.c 記得都要把型別改成 uint32_t parse_mbr(void);

uint32_t parse_mbr(void) {
    uint8_t* buffer = (uint8_t*) kmalloc(512);
    ata_read_sector(0, buffer);

    if (buffer[510] != 0x55 || buffer[511] != 0xAA) {
        kfree(buffer);
        return 0; // 找不到 MBR
    }
    
    mbr_partition_t* partitions = (mbr_partition_t*)(buffer + 446);
    uint32_t target_lba = 0;

    for (int i = 0; i < 4; i++) {
        if (partitions[i].sector_count > 0) {
            kprintf("Partition %d found at LBA %d\n", i + 1, partitions[i].lba_start);
            // 記錄第一個找到的合法分區
            if (target_lba == 0) {
                target_lba = partitions[i].lba_start;
                // 注意：這裡我們偷懶省略了傳遞 sector_count，
                // 實務上也可以用 struct 把 start 和 count 一起傳回去
            }
        }
    }

    kfree(buffer);
    return target_lba;
}

```

接著打開 **`lib/kernel.c`**：

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
#include "vfs.h"
#include "simplefs.h" // [新增]

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

    // === Day 25: SimpleFS 格式化與掛載測試 ===
    kprintf("--- Initializing File System ---\n");

    // 1. 解析 MBR，取得第一塊分區的起點 (預期是 2048)
    uint32_t part_lba = parse_mbr();
    
    if (part_lba != 0) {
        // 2. 在這個分區上格式化 SimpleFS！(假設分區大小夠大，這裡偷懶塞個假 size)
        simplefs_format(part_lba, 10000);
        
        // 3. 驗證：立刻把剛才寫入的 Superblock 讀出來檢查魔法數字
        kprintf("\n[Verification] Reading Superblock from LBA %d...\n", part_lba);
        sfs_superblock_t* verify_sb = (sfs_superblock_t*) kmalloc(512);
        ata_read_sector(part_lba, (uint8_t*)verify_sb);
        
        if (verify_sb->magic == SIMPLEFS_MAGIC) {
            kprintf("[Verification] SUCCESS! SimpleFS Magic Number (0x%x) matches!\n", verify_sb->magic);
        } else {
            kprintf("[Verification] FAILED. Read magic: 0x%x\n", verify_sb->magic);
        }
        kfree(verify_sb);

    } else {
        kprintf("No valid partition found on disk.\n");
    }

    kprintf("\nSystem is ready.\n> ");
    while (1) { __asm__ volatile ("hlt"); }
}

```

---

### 執行與見證奇蹟

請直接執行：

```bash
make clean && make run

```

**預期結果 🏆**
畫面上會顯示：

```text
Partition 1 found at LBA 2048
[SimpleFS] Formatting partition starting at LBA 2048...
[SimpleFS] Superblock written.
[SimpleFS] Empty root directory created.
[SimpleFS] Format complete!

[Verification] Reading Superblock from LBA 2048...
[Verification] SUCCESS! SimpleFS Magic Number (0x21534653) matches!

```

你成功在硬碟的荒地裡，用 C 語言結構體畫出了格局！現在這個分區不再是一堆無意義的 Bytes，它擁有了能夠被作業系統理解的靈魂。

這套自製的 SimpleFS 架構順利上線了嗎？如果成功了，Day 26 我們就可以寫一支 API，把第一份真正的「檔案」寫進這個目錄表裡了！
