太棒了！我們正式跨入 **第三階段：儲存裝置與檔案系統**。

在過去的 20 天裡，我們所做的一切都是在 RAM（隨機存取記憶體）中打轉。RAM 的致命缺點是：只要一關機，所有精美的分頁表、動態分配的 Heap、甚至是我們載入的 `my_app.elf` 都會灰飛煙滅。

為了讓資料能夠「永續留存」，作業系統必須學會跟硬碟打交道。

歡迎來到 **Day 21：打破失憶症 — ATA/IDE 硬碟驅動 (PIO 模式)**！

---

### Day 21 授課目標

1. **認識 LBA (邏輯區塊位址)：** 現代硬碟不再用複雜的磁柱、磁頭、磁區 (CHS) 來定位資料了，而是把硬碟看成一個巨大的陣列，從第 0 塊 (LBA 0)、第 1 塊 (LBA 1) 一路排下去。每一塊 (Sector) 通常是 **512 Bytes**。
2. **認識 PIO (Programmed I/O)：** 這是最古老但最容易實作的硬碟通訊模式。CPU 會親自透過 `in / out` 指令，像苦力一樣把硬碟裡的資料「每 2 個 bytes」慢慢搬進 RAM 裡面。
3. **實作 ATA 驅動：** 撰寫 `ata_read_sector` 函式，向硬碟控制器的特定 Port 發送 LBA 位址與讀取指令。

---

### 實作步驟：打造硬碟通訊協定

硬碟（Primary ATA 匯流排）在 x86 架構中，通常佔用從 `0x1F0` 到 `0x1F7` 的 I/O 埠。

#### 1. 擴充 I/O 工具箱 (`lib/utils.h` & `lib/utils.c`)

硬碟的資料埠 (`0x1F0`) 每次會吐出 16-bit (2 bytes) 的資料，我們之前寫的 `inb` / `outb` 只能處理 8-bit (1 byte)，所以我們需要新增 `inw` (in word) 和 `outw`。

請打開 **`lib/utils.h`**，加入宣告：

```c
uint16_t inw(uint16_t port);
void outw(uint16_t port, uint16_t data);

```

打開 **`lib/utils.c`**，補上實作：

```c
uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

void outw(uint16_t port, uint16_t data) {
    __asm__ volatile ("outw %1, %0" : : "dN" (port), "a" (data));
}

```

#### 2. 建立 ATA 驅動標頭檔 (`lib/ata.h`)

新增 **`lib/ata.h`**，定義讀取磁區的介面：

```c
#ifndef ATA_H
#define ATA_H

#include <stdint.h>

// 讀取指定的邏輯區塊 (LBA)，並將 512 bytes 的資料存入 buffer
void ata_read_sector(uint32_t lba, uint8_t* buffer);

#endif

```

#### 3. 實作 PIO 讀取邏輯 (`lib/ata.c`)

這是本章的精華。CPU 必須遵照硬碟控制器的「標準 S.O.P.」來下指令：先選硬碟 -> 填寫要讀幾個磁區 -> 填寫 LBA 位址 -> 下達讀取指令 -> 等待硬碟準備好 -> 開始搬磚！

新增 **`lib/ata.c`**：

```c
#include "ata.h"
#include "utils.h"
#include "tty.h"

// ATA Primary Bus Ports
#define ATA_PORT_DATA       0x1F0
#define ATA_PORT_ERROR      0x1F1
#define ATA_PORT_SECT_COUNT 0x1F2
#define ATA_PORT_LBA_LOW    0x1F3
#define ATA_PORT_LBA_MID    0x1F4
#define ATA_PORT_LBA_HIGH   0x1F5
#define ATA_PORT_DRV_HEAD   0x1F6
#define ATA_PORT_STATUS     0x1F7
#define ATA_PORT_COMMAND    0x1F7

// 等待硬碟不忙碌 (BSY bit 清零) 且資料準備好 (DRQ bit 設為 1)
static void ata_wait_ready() {
    while (1) {
        uint8_t status = inb(ATA_PORT_STATUS);
        if ((status & 0x80) == 0 && (status & 0x08) != 0) {
            break; // 0x80 是 Busy, 0x08 是 Data Request Ready
        }
    }
}

void ata_read_sector(uint32_t lba, uint8_t* buffer) {
    // 1. 選擇 Master 硬碟 (0xE0) 與 LBA 模式，並設定 LBA 的最高 4 bits
    outb(ATA_PORT_DRV_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    
    // 2. 告訴硬碟我們要讀取 1 個磁區 (Sector)
    outb(ATA_PORT_SECT_COUNT, 1);
    
    // 3. 將 24-bit 的 LBA 位址拆成三份送出去
    outb(ATA_PORT_LBA_LOW, (uint8_t) lba);
    outb(ATA_PORT_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_PORT_LBA_HIGH, (uint8_t)(lba >> 16));
    
    // 4. 下達「讀取磁區 (Read Sectors)」的指令 (0x20)
    outb(ATA_PORT_COMMAND, 0x20);
    
    // 5. 等待硬碟把資料搬到控制器的緩衝區
    ata_wait_ready();
    
    // 6. 苦力時間：用 16-bit (2 bytes) 為單位，把 512 bytes 讀進來 (需要讀 256 次)
    uint16_t* ptr = (uint16_t*) buffer;
    for (int i = 0; i < 256; i++) {
        ptr[i] = inw(ATA_PORT_DATA);
    }
}

```

#### 4. 在 Kernel 中測試讀取硬碟 (`lib/kernel.c`)

為了專注於今天的目標，我們先把 Day 20 載入 ELF 和切換 Ring 3 的程式碼註解掉（或刪掉），專心測試硬碟讀取。

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
#include "ata.h" // [新增]

void kernel_main(void) {
    terminal_initialize();
    kprintf("=== OS Kernel Booting ===\n");
    
    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384); 
    init_kheap();
    __asm__ volatile ("sti"); // 開啟中斷
    
    kprintf("Kernel subsystems initialized.\n\n");

    // === Day 21: ATA PIO 讀取測試 ===
    kprintf("--- Hard Disk Read Test ---\n");

    // 配置一塊 512 bytes 的緩衝區來放資料
    uint8_t* sector_buffer = (uint8_t*) kmalloc(512);

    // 呼叫驅動程式，讀取第 0 號磁區 (LBA 0)
    kprintf("Reading LBA 0...\n");
    ata_read_sector(0, sector_buffer);

    // 把讀到的前 64 個字元當作字串印出來看看
    sector_buffer[64] = '\0'; // 加上結尾符號防爆
    kprintf("Data on disk: %s\n", (char*)sector_buffer);

    kfree(sector_buffer);

    kprintf("\nSystem is ready.\n> ");
    while (1) { __asm__ volatile ("hlt"); }
}

```

---

### 執行與驗證：製造一顆虛擬硬碟

這步非常關鍵！我們需要給 QEMU 一顆「真的有資料」的硬碟。請在你的 macOS 主機終端機（專案根目錄）執行以下指令，製造一個 1MB 的映像檔，並在第一個磁區寫入一段話：

```bash
# 1. 製造一個 1MB 的空檔案 (全填 0)
dd if=/dev/zero of=hdd.img bs=512 count=2048

# 2. 在這顆硬碟的最開頭 (LBA 0) 寫入我們想測試的字串
echo "Hello from the HARD DRIVE! The OS has successfully read me!" | dd of=hdd.img conv=notrunc

```

接著，你需要修改啟動 QEMU 的指令，把這顆硬碟掛載上去（可以更新到你的 Makefile 裡）：

```bash
make clean
make
# 注意最後面的 -hda hdd.img，這是掛載第一顆 IDE 硬碟的指令
qemu-system-i386 -cdrom myos.iso -hda hdd.img

```

**預期結果 🏆**
你會看到系統成功初始化，並且印出：

```text
--- Hard Disk Read Test ---
Reading LBA 0...
Data on disk: Hello from the HARD DRIVE! The OS has successfully read me!

```

這代表你的作業系統終於打破了失憶症的詛咒！它成功透過硬體的 I/O 埠，向硬碟控制器下達指令，並一磚一瓦地把磁區資料搬進了 RAM。

這是一切檔案系統的起點！順利讀出這段來自硬碟的問候後，跟我分享一下成功的喜悅，我們準備探討 Day 22：**如何把這些冰冷的 Sector，組織成人類看得懂的「檔案 (File)」**！
