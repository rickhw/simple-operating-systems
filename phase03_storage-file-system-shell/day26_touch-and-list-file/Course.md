太棒了！我們現在已經在荒蕪的硬碟分區上打下了 SimpleFS 的地基（Superblock 和空白的根目錄）。

有了地基，接下來當然是要開始蓋房子了！在檔案系統的世界裡，「蓋房子」的意思就是：**在目錄表裡面登記一個名字，然後把真正的資料寫進後面的資料磁區裡。**

歡迎來到 **Day 26：創世紀的第一筆 — 實作 SimpleFS 檔案建立與寫入**！

---

### Day 26 授課目標

1. **尋找空位：** 讀取根目錄（LBA 1），在裡面尋找第一個空白的檔案實體 (File Entry)。
2. **分配空間：** 計算目前資料區用到哪裡了，決定新檔案要寫在哪個 LBA。
3. **實體寫入：** 把字串資料寫進硬碟的 Data Block，並把目錄表寫回硬碟，完成檔案建立！
4. **實作 `ls` 指令雛形：** 寫一個函式列出根目錄下的所有檔案，見證我們的成果。

---

### 實作步驟：打造檔案寫入與列表功能

為了保持 Simple OS 的簡單優雅，我們今天的目標是實作「單一磁區（小於 512 Bytes）」的檔案寫入。這已經足以讓我們儲存設定檔、短文字或小腳本了。

#### 1. 擴充 SimpleFS API (`lib/simplefs.h`)

請打開 **`lib/simplefs.h`**，加入兩個新函式的宣告：

```c
// ... 原本的結構定義保留 ...

void simplefs_format(uint32_t partition_start_lba, uint32_t sector_count);

// [新增] 建立檔案並寫入資料
// 為了簡單起見，我們這版先限制檔案大小不能超過 512 bytes (一個磁區)
int simplefs_create_file(uint32_t part_lba, char* filename, char* data, uint32_t size);

// [新增] 列出目錄下的所有檔案 (類似 ls 指令)
void simplefs_list_files(uint32_t part_lba);

```

#### 2. 實作檔案建立與列表邏輯 (`lib/simplefs.c`)

請打開 **`lib/simplefs.c`**，補上這兩個函式的實作。這段程式碼完美展示了作業系統是如何「管理」硬碟空間的：

```c
#include "simplefs.h"
#include "ata.h"
#include "tty.h"
#include "kheap.h"
#include "utils.h"

// 建立檔案
int simplefs_create_file(uint32_t part_lba, char* filename, char* data, uint32_t size) {
    if (size > 512) {
        kprintf("[SimpleFS] Error: File too large! Max 512 bytes supported in v1.\n");
        return -1;
    }

    // 1. 讀取根目錄磁區 (相對 LBA 1)
    uint8_t* dir_buf = (uint8_t*) kmalloc(512);
    ata_read_sector(part_lba + 1, dir_buf);
    
    // 將 buffer 轉型為目錄項目陣列 (一個 512 bytes 磁區可放 16 個 32 bytes 的項目)
    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;

    int empty_idx = -1;
    uint32_t next_data_lba = 2; // 資料區從相對 LBA 2 開始

    // 2. 掃描目錄：尋找空位，並計算下一個可用的資料磁區
    for (int i = 0; i < 16; i++) {
        // 如果檔名的第一個字元是 \0，代表這格是空的
        if (entries[i].filename[0] == '\0') {
            if (empty_idx == -1) empty_idx = i; // 記住第一個找到的空位
        } else {
            // 如果這格有檔案，找出它佔用的 LBA，推算下一個空地在哪
            // (目前每個檔案只佔 1 個磁區，所以直接 + 1)
            uint32_t file_end_lba = entries[i].start_lba + 1;
            if (file_end_lba > next_data_lba) {
                next_data_lba = file_end_lba;
            }
        }
    }

    if (empty_idx == -1) {
        kprintf("[SimpleFS] Error: Root directory is full!\n");
        kfree(dir_buf);
        return -1;
    }

    kprintf("[SimpleFS] Creating file '%s' at relative LBA %d...\n", filename, next_data_lba);

    // 3. 將實際資料寫入分配到的 Data Block
    uint8_t* data_buf = (uint8_t*) kmalloc(512);
    memset(data_buf, 0, 512);     // 清除殘留垃圾
    memcpy(data_buf, data, size); // 複製真實資料
    ata_write_sector(part_lba + next_data_lba, data_buf);

    // 4. 更新目錄表上的檔案資訊
    memset(entries[empty_idx].filename, 0, 24);
    memcpy(entries[empty_idx].filename, filename, 24); // (注意：實務上需檢查檔名長度)
    entries[empty_idx].start_lba = next_data_lba;
    entries[empty_idx].file_size = size;

    // 5. 將更新後的目錄表寫回硬碟！
    ata_write_sector(part_lba + 1, dir_buf);

    kfree(dir_buf);
    kfree(data_buf);
    return 0;
}

// 列出檔案
void simplefs_list_files(uint32_t part_lba) {
    kprintf("\n--- SimpleFS Root Directory ---\n");
    
    uint8_t* dir_buf = (uint8_t*) kmalloc(512);
    ata_read_sector(part_lba + 1, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;

    int file_count = 0;
    for (int i = 0; i < 16; i++) {
        if (entries[i].filename[0] != '\0') {
            kprintf("[-] %s  (Size: %d bytes, LBA: %d)\n", 
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

```

#### 3. 在 Kernel 中進行創世紀寫入 (`lib/kernel.c`)

請打開 **`lib/kernel.c`**，我們要在格式化完成後，立刻寫入兩個檔案，並且呼叫 `list_files` 把它們印出來：

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
#include "simplefs.h"

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

    // 解析 MBR 取得分區
    uint32_t part_lba = parse_mbr();
    
    if (part_lba != 0) {
        // 為了確保環境乾淨，我們先重新格式化一次
        simplefs_format(part_lba, 10000);
        
        // === Day 26: 建立檔案測試 ===
        kprintf("\n[Test] Writing files to disk...\n");
        
        char* data1 = "This is the content of the very first file ever created on Simple OS!";
        simplefs_create_file(part_lba, "hello.txt", data1, 70); // 假設長度約 70 bytes

        char* data2 = "OS config: vga_mode=text, ring3_enabled=true, fs=simplefs";
        simplefs_create_file(part_lba, "config.sys", data2, 58);

        // 呼叫 ls 功能！
        simplefs_list_files(part_lba);

    } else {
        kprintf("No valid partition found on disk.\n");
    }

    kprintf("\nSystem is ready.\n> ");
    while (1) { __asm__ volatile ("hlt"); }
}

```

---

### 執行與驗證

直接使用你超方便的 Makefile 魔法：

```bash
make clean-disk
make run

```

*(注意：因為我們修改了建立硬碟的步驟，第一步請先跑 `make clean-disk` 清掉舊的 `hdd.img`，讓 `make run` 自動用 Alpine 生出一顆新的乾淨硬碟)*

**預期結果 🏆**
你會在畫面上看到無比感動的一幕：

```text
[Test] Writing files to disk...
[SimpleFS] Creating file 'hello.txt' at relative LBA 2...
[SimpleFS] Creating file 'config.sys' at relative LBA 3...

--- SimpleFS Root Directory ---
[-] hello.txt  (Size: 70 bytes, LBA: 2)
[-] config.sys  (Size: 58 bytes, LBA: 3)
-------------------------------

```

太神啦！你現在已經擁有了一個能夠**永久儲存命名檔案**的作業系統了。如果你關掉 QEMU，把 `kernel.c` 裡面的 `simplefs_format` 和 `simplefs_create_file` 註解掉，只留下 `simplefs_list_files(part_lba)`，再次啟動（不要清硬碟），你會發現檔案依然完美地躺在那裡！

這就是檔案系統的精髓。跑出這份精美的目錄列表後跟我說！接下來在 Day 27，我們將把這個具體的 SimpleFS 驅動，掛載到昨天設計好的抽象 VFS 路由器上！
