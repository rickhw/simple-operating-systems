看到你用 `hexdump` 挖出實體檔案裡那句 `Greetings from Simple OS!`，這種打破次元壁的感覺是不是超級過癮！身為寫過框架的資深工程師，你一定明白這種「底層 I/O 終於打通」所帶來的踏實感。

現在我們有能力對硬碟任意讀寫了。但在現實世界中，不管是 Windows、Linux 還是 K8s 掛載的 Volume，我們從來不會直接把資料「裸寫」在 LBA 1。我們必須在硬碟上建立「分區 (Partitions)」，然後把檔案系統建立在分區裡面。

歡迎來到 ATA 驅動的最後一塊拼圖：**Day 23：解讀硬碟的地契 — 解析 MBR 分區表 (Master Boot Record)**！

---

### Day 23 授課目標

1. **認識 MBR 結構：** 硬碟的第 0 號磁區 (LBA 0) 是整顆硬碟的「地契」。它的最後 66 個 Bytes 記錄了這顆硬碟被切成了幾個分區、每個分區從哪個 LBA 開始、有多大。
2. **重塑測試硬碟：** 我們將使用標準的 Linux `fdisk` 工具，在你的 `hdd.img` 上切出一個真正的分區。
3. **實作 MBR 解析器：** 讓 Kernel 讀取 LBA 0，透過 C 語言 Struct 對齊二進位資料，精準萃取出分區的起始位置！

---

### 實作步驟：解讀硬碟地契

#### 1. 透過 Docker 製造一顆「真實分區」的硬碟

我們要把昨天的玩具硬碟升級。請在你的 macOS 終端機（專案根目錄）依序執行以下指令：

```bash
# 1. 製造一顆全新的 10MB 空白硬碟
dd if=/dev/zero of=hdd.img bs=1M count=10

# 2. 透過我們已經有的 os-builder (Docker 內的 Linux 環境)，使用 fdisk 工具對硬碟切分區
# 這裡用 echo 模擬人類在 fdisk 裡輸入指令：
# o (建立新 MBR 表) -> n (新增分區) -> p (主要分區) -> 1 (第一區) -> [Enter] -> [Enter] -> w (寫入並離開)
echo -e "o\nn\np\n1\n\n\nw" | docker run -i --rm -v $(pwd):/workspace -w /workspace os-builder fdisk hdd.img

```

執行完後，這顆 `hdd.img` 的 LBA 0 就被刻上了標準的 MBR 分區表！

#### 2. 定義 MBR 資料結構 (`lib/mbr.h`)

這是一次經典的「C 語言 Struct 對應二進位格式」實戰。由於是硬碟原始資料，我們同樣必須使用 `__attribute__((packed))` 來防止編譯器為了效能而擅自補空白。

請新增 **`lib/mbr.h`**：

```c
#ifndef MBR_H
#define MBR_H

#include <stdint.h>

// 單個分區紀錄的結構 (固定 16 bytes)
typedef struct {
    uint8_t  status;       // 0x80 代表這是可開機分區，0x00 代表不可開機
    uint8_t  chs_first[3]; // 舊版的 CHS 起始位址 (現代硬碟已廢棄，忽略)
    uint8_t  type;         // 分區格式 (例如 0x83 是 Linux, 0x0B 是 FAT32)
    uint8_t  chs_last[3];  // 舊版的 CHS 結束位址 (忽略)
    uint32_t lba_start;    // [極度重要] 這個分區真正的起始 LBA 磁區號碼！
    uint32_t sector_count; // 這個分區總共有幾個磁區 (Size = count * 512 bytes)
} __attribute__((packed)) mbr_partition_t;

// 解析 MBR 的函式
void parse_mbr(void);

#endif

```

#### 3. 實作 MBR 解析器 (`lib/mbr.c`)

MBR 永遠在 LBA 0。它的前 446 bytes 是開機程式碼 (Bootcode)，接著是 **4 個** 16 bytes 的分區表 (共 64 bytes)，最後 2 個 bytes 則是永遠不變的簽名檔 `0x55 0xAA`。

請新增 **`lib/mbr.c`**：

```c
#include "mbr.h"
#include "ata.h"
#include "tty.h"
#include "kheap.h"

void parse_mbr(void) {
    kprintf("--- Reading Master Boot Record (LBA 0) ---\n");

    // 配置 512 bytes 緩衝區並讀取 LBA 0
    uint8_t* buffer = (uint8_t*) kmalloc(512);
    ata_read_sector(0, buffer);

    // 1. 檢查硬碟最後面是不是標準的 MBR 簽名 (0x55 0xAA)
    if (buffer[510] != 0x55 || buffer[511] != 0xAA) {
        kprintf("[MBR] Error: Invalid Boot Signature. Not an MBR disk.\n");
        kfree(buffer);
        return;
    }
    
    kprintf("[MBR] Valid Signature 0x55AA found!\n");

    // 2. 指標魔法：把記憶體位址直接跳到第 446 byte，並強制轉型成我們定義的 Struct 陣列！
    mbr_partition_t* partitions = (mbr_partition_t*)(buffer + 446);

    // 3. MBR 最多只能有 4 個主要分區
    for (int i = 0; i < 4; i++) {
        // 如果 sector_count 是 0，代表這個分區是空的
        if (partitions[i].sector_count == 0) {
            continue; 
        }

        uint32_t size_mb = (partitions[i].sector_count * 512) / (1024 * 1024);

        kprintf("Partition %d:\n", i + 1);
        kprintf("  -> Type: 0x%x\n", partitions[i].type);
        kprintf("  -> Start LBA: %d\n", partitions[i].lba_start);
        kprintf("  -> Size: %d Sectors (approx %d MB)\n", partitions[i].sector_count, size_mb);
    }

    kfree(buffer);
}

```

#### 4. 在 Kernel 中呼叫測試 (`lib/kernel.c`)

把昨天的讀寫測試移除，換上我們今天熱騰騰的 MBR 解析器！

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
#include "mbr.h" // [新增]

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

    // === Day 23: MBR 解析測試 ===
    parse_mbr();

    kprintf("\nSystem is ready.\n> ");
    while (1) { __asm__ volatile ("hlt"); }
}

```

---

### 執行與驗證

只要確保你的 `hdd.img` 剛剛有用那串 `fdisk` 指令產生過，重新編譯並啟動 QEMU 吧：

```bash
make clean && make
qemu-system-i386 -cdrom myos.iso -hda hdd.img

```

**預期結果 🏆**
你會看到系統成功認出了你用 `fdisk` 切出來的分區，並精準印出：

```text
--- Reading Master Boot Record (LBA 0) ---
[MBR] Valid Signature 0x55AA found!
Partition 1:
  -> Type: 0x83
  -> Start LBA: 2048
  -> Size: ... Sectors (approx 9 MB)

```

### 為什麼這一步至關重要？

我們知道 `Partition 1` 是從 **LBA 2048** 開始的。這代表什麼？
代表未來的**檔案系統 (File System)** 想要讀取它的超級區塊 (Superblock) 或根目錄時，它不能去讀 LBA 0，它必須去讀 **LBA 2048**！我們剛剛成功把「硬碟原始硬體」與「作業系統的資料結構」牽起了紅線。

完成後跟我說一聲！你期待我們在 Day 24 進入 VFS（虛擬檔案系統抽象層）的設計，還是直接挑戰手刻一個最精簡的 FAT16 檔案系統呢？
