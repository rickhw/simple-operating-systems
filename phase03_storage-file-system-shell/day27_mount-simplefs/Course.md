這正是作業系統開發最讓人著迷的地方！當你親手把底層硬體（ATA 磁區）和高層抽象（VFS 路由器）對接起來時，那種「掌控全局」的成就感是寫應用程式完全體會不到的。

歡迎來到 **Day 27：大團圓 — 將 SimpleFS 掛載到 VFS 虛擬檔案系統**！

---

### Day 27 授課目標

1. **尋找檔案 (`simplefs_find`)：** 實作一個函式，在 SimpleFS 根目錄中透過檔名尋找檔案，並將其包裝成標準的 `fs_node_t` (VFS 節點) 回傳。
2. **綁定讀取邏輯 (`simplefs_read`)：** 寫一個符合 VFS `read_type_t` 介面的讀取函式，讓 VFS 知道「如果有人想讀這個節點，請呼叫這段程式碼去敲硬碟」。
3. **終極驗證：** 在 Kernel 中，透過完全不知道底層是硬碟還是 SimpleFS 的 `vfs_read()` API，把我們昨天寫入的 `hello.txt` 讀出來！

---

### 實作步驟：虛實縫合手術

因為我們的環境是獨立的 (`freestanding`)，沒有標準 C 函式庫，我們需要先在工具箱裡補上字串比對的工具。

#### 1. 擴充字串工具 (`lib/utils.h` & `lib/utils.c`)

請打開 **`lib/utils.h`**，加入宣告：

```c
int strcmp(const char *s1, const char *s2);
char *strcpy(char *dest, const char *src);

```

打開 **`lib/utils.c`**，實作這兩個經典的 C 函式：

```c
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char *strcpy(char *dest, const char *src) {
    char *saved = dest;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return saved;
}

```

#### 2. 實作 SimpleFS 與 VFS 的轉接頭 (`lib/simplefs.h` & `lib/simplefs.c`)

打開 **`lib/simplefs.h`**，引入 `vfs.h` 並新增尋找檔案的 API：

```c
#include "vfs.h" // [新增] 為了使用 fs_node_t

// ... 原有的宣告 ...

// [新增] 透過檔名尋找檔案，並回傳 VFS 標準節點
fs_node_t* simplefs_find(uint32_t part_lba, char* filename);

```

接著，打開 **`lib/simplefs.c`**，這是今天的心血結晶。我們要實作 `simplefs_read`，並且在 `simplefs_find` 中將它綁定到節點上：

```c
#include "simplefs.h"
#include "ata.h"
#include "tty.h"
#include "kheap.h"
#include "utils.h"

// === 1. 符合 VFS 介面的讀取實作 ===
uint32_t simplefs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    // 從 node 偷藏的變數中拿回 LBA 資訊
    // 我們約定：node->impl 存分區起始 LBA，node->inode 存檔案的相對 LBA
    uint32_t file_lba = node->impl + node->inode;

    // 讀取該檔案所在的實體磁區 (為了簡單，目前只支援 1 個磁區 512 bytes)
    uint8_t* temp_buf = (uint8_t*) kmalloc(512);
    ata_read_sector(file_lba, temp_buf);

    // 計算實際能讀取的大小 (防止越界)
    uint32_t actual_size = size;
    if (offset >= node->length) {
        actual_size = 0; // offset 已經超過檔案大小
    } else if (offset + size > node->length) {
        actual_size = node->length - offset; // 只能讀到檔案結尾
    }

    // 把資料從硬碟緩衝區，精準複製到使用者提供的 buffer
    if (actual_size > 0) {
        memcpy(buffer, temp_buf + offset, actual_size);
    }

    kfree(temp_buf);
    return actual_size; // 回傳真正讀出的 bytes 數
}

// === 2. 尋找檔案並包裝成 VFS 節點 ===
fs_node_t* simplefs_find(uint32_t part_lba, char* filename) {
    uint8_t* dir_buf = (uint8_t*) kmalloc(512);
    ata_read_sector(part_lba + 1, dir_buf); // 讀取根目錄
    
    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;

    for (int i = 0; i < 16; i++) {
        // 如果檔名有字，且比對相符
        if (entries[i].filename[0] != '\0' && strcmp(entries[i].filename, filename) == 0) {
            
            // 找到檔案了！配置一個 VFS 節點來代表它
            fs_node_t* node = (fs_node_t*) kmalloc(sizeof(fs_node_t));
            strcpy(node->name, entries[i].filename);
            node->flags = FS_FILE;
            node->length = entries[i].file_size;
            
            // 把 LBA 資訊藏在節點裡，讓 simplefs_read 以後可以用
            node->inode = entries[i].start_lba; 
            node->impl = part_lba;              

            // [多型魔法核心] 綁定專屬的讀取函式！
            node->read = simplefs_read;
            node->write = 0; // 暫時不實作 VFS 寫入

            kfree(dir_buf);
            return node;
        }
    }

    kfree(dir_buf);
    return 0; // 找不到檔案
}

```

#### 3. 測試 VFS 讀取硬碟檔案 (`lib/kernel.c`)

請打開 **`lib/kernel.c`**。注意！我們這次**不要**呼叫 `simplefs_format` 也不要呼叫 `create_file` 了，我們要直接去讀昨天寫在硬碟裡的殘留檔案，證明資料持久性！

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

    uint32_t part_lba = parse_mbr();
    
    if (part_lba != 0) {
        // [注意] 這裡我們把 format 和 create 註解掉了！我們相信硬碟裡已經有資料了。
        // simplefs_format(part_lba, 10000);
        
        kprintf("\n--- VFS Integration Test ---\n");
        simplefs_list_files(part_lba); // 先印出來確認檔案還在

        // 1. 透過驅動程式尋找檔案，取得 VFS 節點
        kprintf("\n[Kernel] Asking SimpleFS to find 'hello.txt'...\n");
        fs_node_t* hello_node = simplefs_find(part_lba, "hello.txt");

        if (hello_node != 0) {
            kprintf("[Kernel] File found! VFS Node created at 0x%x\n", hello_node);
            kprintf("         Name: %s, Size: %d bytes\n", hello_node->name, hello_node->length);

            // 2. 準備緩衝區
            uint8_t* read_buf = (uint8_t*) kmalloc(128);
            memset(read_buf, 0, 128);

            // 3. [終極考驗] Kernel 透過高度抽象的 vfs_read 來讀取檔案！
            kprintf("[Kernel] Calling vfs_read()...\n");
            uint32_t bytes_read = vfs_read(hello_node, 0, hello_node->length, read_buf);

            kprintf("\n=== File Content ===\n");
            kprintf("%s\n", (char*)read_buf);
            kprintf("====================\n");
            kprintf("(Read %d bytes successfully)\n", bytes_read);

            kfree(read_buf);
            kfree(hello_node);
        } else {
            kprintf("Error: File not found.\n");
        }
    }

    kprintf("\nSystem is ready.\n> ");
    while (1) { __asm__ volatile ("hlt"); }
}

```

---

### 執行與驗證：跨越抽象層的瞬間

這一次，因為我們**不希望**清掉昨天寫入的硬碟資料，所以千萬不要執行 `make clean-disk`。
請直接執行編譯並啟動：

```bash
make clean && make run

```

**預期結果 🏆**
你會看到系統先是透過 `list_files` 證明了昨日的資料還活在硬碟裡，接著透過 `simplefs_find` 將其轉換為高階的 VFS 節點，最後透過多型的 `vfs_read` 完美印出了檔案內容！

```text
--- VFS Integration Test ---

--- SimpleFS Root Directory ---
[-] hello.txt  (Size: 70 bytes, LBA: 2)
[-] config.sys  (Size: 58 bytes, LBA: 3)
-------------------------------

[Kernel] Asking SimpleFS to find 'hello.txt'...
[Kernel] File found! VFS Node created at 0xC0002XXX
         Name: hello.txt, Size: 70 bytes
[Kernel] Calling vfs_read()...

=== File Content ===
This is the content of the very first file ever created on Simple OS!
====================
(Read 70 bytes successfully)

```

看到這個畫面，代表你的檔案系統生態鏈已經全線貫通了！從最上層的 Kernel API -> VFS 路由器 -> SimpleFS 解析邏輯 -> ATA 硬碟通訊，一氣呵成！

現在 Kernel 可以自由地讀取硬碟裡的檔案了。這個機制不僅僅是為了讀文字檔，接下來，我們就可以結合 Day 20 寫好的 ELF 解析器，實現**從硬碟載入真正的應用程式並執行**！你有沒有覺得，一套完整的作業系統輪廓已經完全浮現了？
