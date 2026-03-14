太俐落了！把暫時用不到的模組（GRUB 模組載入、User App）先移除或註解掉，保持測試環境的純粹，這是非常專業的系統開發與除錯好習慣。

昨天我們成功讓 Simple OS 讀取了硬碟上的資料，打破了 RAM 斷電即失憶的限制。但一個只能「讀」的系統是無法建構生態系的，應用程式需要存檔、系統需要寫 Log。

歡迎來到 **Day 22：留下你的印記 — ATA/IDE 硬碟寫入與資料持久化**！

---

### Day 22 授課目標

1. **實作 ATA 寫入 (`ata_write_sector`)：** 邏輯與讀取非常相似，但資料流向相反（從 RAM 到硬碟）。
2. **認識 Cache Flush (快取寫回)：** 現代硬碟為了效能，通常會先把寫入的資料放在硬碟內部的 RAM 快取裡。我們必須多下一道 `0xE7` 指令，強迫硬碟立刻把資料刻錄到實體磁區上。
3. **上帝的驗證：** 讓 Simple OS 寫入資料到虛擬硬碟 (`hdd.img`) 後，我們跳出虛擬機，在你的 macOS 主機上用指令直接檢查那個檔案，證明 OS 真的修改了物理世界（主機）的資料！

---

### 實作步驟：賦予 OS 寫入硬碟的權力

#### 1. 擴充 ATA 驅動介面 (`lib/ata.h`)

請打開 `lib/ata.h`，加入寫入函式的宣告：

```c
#ifndef ATA_H
#define ATA_H

#include <stdint.h>

void ata_read_sector(uint32_t lba, uint8_t* buffer);

// [新增] 寫入 512 bytes 的資料到指定的邏輯區塊 (LBA)
void ata_write_sector(uint32_t lba, uint8_t* buffer);

#endif

```

#### 2. 實作 PIO 寫入邏輯 (`lib/ata.c`)

寫入的步驟跟讀取幾乎一模一樣，只有三個地方不同：

1. 下達的指令代碼是 `0x30` (Write Sectors) 而不是 `0x20`。
2. 我們使用 `outw` 把資料推出去，而不是用 `inw` 讀進來。
3. 寫完 256 個 word (512 bytes) 後，要下達 `0xE7` 指令強制磁碟機把快取寫入碟片。

打開 `lib/ata.c`，在檔案最下方加入：

```c
void ata_write_sector(uint32_t lba, uint8_t* buffer) {
    // 1. 選擇 Master 硬碟 (0xE0) 與 LBA 模式
    outb(ATA_PORT_DRV_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    
    // 2. 告訴硬碟我們要寫入 1 個磁區
    outb(ATA_PORT_SECT_COUNT, 1);
    
    // 3. 傳送 LBA 位址
    outb(ATA_PORT_LBA_LOW, (uint8_t) lba);
    outb(ATA_PORT_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_PORT_LBA_HIGH, (uint8_t)(lba >> 16));
    
    // 4. [關鍵] 下達「寫入磁區 (Write Sectors)」的指令 (0x30)
    outb(ATA_PORT_COMMAND, 0x30);
    
    // 5. 等待硬碟準備好接收資料
    ata_wait_ready();
    
    // 6. 苦力時間：用 16-bit 為單位，把 RAM 裡的 512 bytes 推給硬碟控制器
    uint16_t* ptr = (uint16_t*) buffer;
    for (int i = 0; i < 256; i++) {
        outw(ATA_PORT_DATA, ptr[i]);
    }
    
    // 7. [極度重要] 下達 Cache Flush 指令 (0xE7)
    // 確保硬碟不會把資料放在快取裡偷懶，強迫它立刻寫入實體磁碟
    outb(ATA_PORT_COMMAND, 0xE7);
    
    // 等待寫入動作徹底完成
    ata_wait_ready();
}

```

#### 3. 測試寫入與驗證 (`lib/kernel.c`)

我們要在 LBA 1（第 2 個磁區，位移 512 bytes 處）寫入一段全新的字串，然後立刻把它讀出來驗證。

打開 `lib/kernel.c`：

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

    // === Day 22: ATA PIO 讀寫測試 ===
    kprintf("--- Hard Disk Write Test ---\n");

    // 準備兩塊 512 bytes 的緩衝區：一個用來寫，一個用來讀
    uint8_t* write_buffer = (uint8_t*) kmalloc(512);
    uint8_t* read_buffer  = (uint8_t*) kmalloc(512);

    // 把寫入緩衝區清空為 0
    memset(write_buffer, 0, 512);
    
    // 在寫入緩衝區填上我們的專屬印記
    char* msg = "Greetings from Simple OS! This data was WRITTEN by the Kernel!";
    memcpy(write_buffer, msg, 62); // 字串長度大約 62 bytes

    // 1. 寫入到 LBA 1 (第 2 個磁區)
    kprintf("Writing to LBA 1...\n");
    ata_write_sector(1, write_buffer);
    kprintf("Write completed.\n");

    // 2. 為了證明不是我們作弊，我們把 read_buffer 清空
    memset(read_buffer, 0, 512);

    // 3. 從 LBA 1 讀取剛才寫入的資料
    kprintf("Reading from LBA 1...\n");
    ata_read_sector(1, read_buffer);

    // 4. 印出來驗證！
    kprintf("Data on disk (LBA 1): %s\n", (char*)read_buffer);

    kfree(write_buffer);
    kfree(read_buffer);

    kprintf("\nSystem is ready.\n> ");
    while (1) { __asm__ volatile ("hlt"); }
}

```

*(注意：在 `memcpy` 那裡，如果你的 `utils.c` 還沒有實作 `memcpy`，請補上它，或者直接寫個 `for` 迴圈把 `msg` 的字元一個個塞進 `write_buffer` 裡。)*

---

### 執行與驗證：跨越虛擬機的實體證據

同樣執行編譯與啟動指令：

```bash
make clean
make
qemu-system-i386 -cdrom myos.iso -hda hdd.img

```

在 QEMU 的畫面上，你應該會順利看到：

```text
Data on disk (LBA 1): Greetings from Simple OS! This data was WRITTEN by the Kernel!

```

**但是，這還不夠震撼！我們要跳出 Matrix 來驗證。**

請關閉 QEMU。回到你的 macOS 終端機（專案根目錄下），我們要直接偷看 `hdd.img` 這個檔案的內部二進位資料。
請輸入這個指令：

```bash
# 用 hexdump 顯示檔案內容，跳過前 512 bytes (也就是 LBA 0)，並顯示接下來的幾行
hexdump -C -s 512 -n 128 hdd.img

```

你會在你的 Mac 終端機看到類似這樣的輸出：

```text
00000200  47 72 65 65 74 69 6e 67  73 20 66 72 6f 6d 20 53  |Greetings from S|
00000210  69 6d 70 6c 65 20 4f 53  21 20 54 68 69 73 20 64  |imple OS! This d|
00000220  61 74 61 20 77 61 73 20  57 52 49 54 54 45 4e 20  |ata was WRITTEN |
...

```

這代表什麼？這代表你親手寫出的作業系統，成功驅動了虛擬機底層的模擬硬體，並且實實在在地把資料寫進了你 macOS 實體硬碟上的一個檔案裡！

這是 OS 具備「狀態持久化」能力的終極證明。

準備好實作 `memcpy` 並跑出這個震撼的 Hexdump 結果了嗎？完成後跟我分享！接下來，我們就可以開始構思 **Day 23：將這些 512 bytes 的區塊，組織成有名字的「檔案」 (SimpleFS 雛形)** 了！
