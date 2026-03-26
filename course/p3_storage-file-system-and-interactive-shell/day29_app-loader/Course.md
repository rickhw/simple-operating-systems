準備好迎接作業系統獨立的歷史性一刻了嗎？

回顧 Day 19，我們為了能執行 `my_app.elf`，使用了 GRUB 的 Multiboot Module 功能。這就像是保母幫我們把執行檔「硬塞」進了實體記憶體，然後交給 Kernel 執行。

但在真正的 Linux 或是 Windows 中，程式是躺在**硬碟**裡的。當你點擊兩下或輸入 `./my_app` 時，是 OS 親自去硬碟把它讀進 RAM 裡，然後再執行的！既然我們已經打通了 VFS 和 SimpleFS，現在正是時候斬斷這條保母的臍帶了。

歡迎來到 **Day 29：斬斷保母的臍帶 — 從實體硬碟動態載入並執行應用程式**！

---

### Day 29 授課目標

1. **升級 SimpleFS 支援大檔案：** 我們之前的 SimpleFS 只支援 512 bytes 的小檔案，但 ELF 執行檔通常好幾 KB。我們要讓 `create_file` 和 `read` 支援跨越多個磁區。
2. **作業系統「安裝程式」模式：** 讓 Kernel 在開機時，把 GRUB 帶來的 `my_app.elf` 寫進我們的硬碟裡（就像你在安裝系統一樣）。
3. **從 VFS 載入並執行：** 完全拋棄 GRUB 提供的記憶體位址，改用 `vfs_read` 從硬碟把 `my_app.elf` 讀出來，交給 `elf_load` 執行！

---

### 實作步驟：

#### 1. 升級 SimpleFS 支援大檔案 (`lib/simplefs.c`)

請打開 **`lib/simplefs.c`**。我們要修改 `simplefs_create_file` 與 `simplefs_read`，讓它們能夠透過迴圈，連續讀寫多個 512 Bytes 的磁區（Sector）。

```c
// ... 前面保留 mount 相關程式碼 ...

// [升級版] 寫入大檔案
int simplefs_create_file(uint32_t part_lba, char* filename, char* data, uint32_t size) {
    uint8_t* dir_buf = (uint8_t*) kmalloc(512);
    ata_read_sector(part_lba + 1, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;

    int empty_idx = -1;
    uint32_t next_data_lba = 2; 

    for (int i = 0; i < 16; i++) {
        if (entries[i].filename[0] == '\0') {
            if (empty_idx == -1) empty_idx = i; 
        } else {
            // [修改] 計算這個檔案佔用了幾個磁區 (無條件進位)
            uint32_t file_sectors = (entries[i].file_size + 511) / 512;
            uint32_t file_end_lba = entries[i].start_lba + file_sectors;
            if (file_end_lba > next_data_lba) {
                next_data_lba = file_end_lba;
            }
        }
    }

    if (empty_idx == -1) { kfree(dir_buf); return -1; }

    // [修改] 跨磁區寫入資料
    uint32_t sectors_needed = (size + 511) / 512;
    for (uint32_t i = 0; i < sectors_needed; i++) {
        uint8_t temp[512] = {0}; // 清空暫存區
        uint32_t copy_size = 512;
        if ((i * 512) + 512 > size) {
            copy_size = size - (i * 512); // 最後一個磁區可能裝不滿
        }
        memcpy(temp, data + (i * 512), copy_size);
        ata_write_sector(part_lba + next_data_lba + i, temp);
    }

    memset(entries[empty_idx].filename, 0, 24);
    strcpy(entries[empty_idx].filename, filename); 
    entries[empty_idx].start_lba = next_data_lba;
    entries[empty_idx].file_size = size;

    ata_write_sector(part_lba + 1, dir_buf);
    kfree(dir_buf);
    return 0;
}

// [升級版] 讀取大檔案
uint32_t simplefs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    uint32_t file_lba = node->impl + node->inode;
    uint32_t actual_size = size;
    if (offset >= node->length) return 0;
    if (offset + size > node->length) actual_size = node->length - offset;

    // 計算需要跨越的磁區範圍
    uint32_t start_sector = offset / 512;
    uint32_t end_sector = (offset + actual_size - 1) / 512;
    uint32_t sector_offset = offset % 512;

    uint8_t* temp_buf = (uint8_t*) kmalloc(512);
    uint32_t bytes_read = 0;

    for (uint32_t i = start_sector; i <= end_sector; i++) {
        ata_read_sector(file_lba + i, temp_buf);

        uint32_t copy_size = 512 - sector_offset;
        if (bytes_read + copy_size > actual_size) {
            copy_size = actual_size - bytes_read;
        }

        memcpy(buffer + bytes_read, temp_buf + sector_offset, copy_size);
        bytes_read += copy_size;
        sector_offset = 0; // 只有第一個磁區可能不是從 0 開始讀
    }

    kfree(temp_buf);
    return bytes_read;
}

// ... 後面 find 和 list_files 保留不變 ...

```

#### 2. OS 化身為安裝程式 (`lib/kernel.c`)

現在是重頭戲！我們要改寫 Kernel 載入模組的方式。它不再直接執行 GRUB 給的模組，而是把模組「安裝」到 SimpleFS 硬碟裡，然後再呼叫 VFS 把它讀出來執行！

請打開 **`lib/kernel.c`**，替換掉原本的載入邏輯：

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
#include "elf.h"
#include "multiboot.h"

void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
    terminal_initialize();
    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384); 
    init_kheap();
    __asm__ volatile ("sti"); 
    kprintf("=== OS Subsystems Ready ===\n\n");

    uint32_t part_lba = parse_mbr();
    if (part_lba == 0) {
        kprintf("Disk Error: No partition found.\n");
        while(1) __asm__ volatile("hlt");
    }

    // 1. 掛載檔案系統並格式化 (為了確保乾淨的測試環境)
    simplefs_mount(part_lba); 
    simplefs_format(part_lba, 10000);
    
    // 寫入 User App 會用到的文字檔
    simplefs_create_file(part_lba, "hello.txt", "This is the content of the very first file ever created on Simple OS!", 70);

    // 2. [系統安裝模式] 擷取 GRUB 帶來的 my_app.elf，並將其寫入實體硬碟！
    if (mbd->mods_count > 0) {
        multiboot_module_t* mod = (multiboot_module_t*)mbd->mods_addr;
        uint32_t app_size = mod->mod_end - mod->mod_start;
        kprintf("[Kernel] 'Installing' my_app.elf to Hard Drive (Size: %d bytes)...\n", app_size);
        simplefs_create_file(part_lba, "my_app.elf", (char*)mod->mod_start, app_size);
    }

    // 列出目前硬碟裡的檔案，應該要有 hello.txt 和 my_app.elf
    simplefs_list_files(part_lba);

    // 3. [自力更生模式] 透過 VFS 從硬碟讀取應用程式！
    kprintf("\n[Kernel] Fetching 'my_app.elf' from Virtual File System...\n");
    fs_node_t* app_node = simplefs_find("my_app.elf");
    
    if (app_node != 0) {
        // 在 Kernel Heap 分配一塊緩衝區來裝這個檔案
        uint8_t* app_buffer = (uint8_t*) kmalloc(app_node->length);
        vfs_read(app_node, 0, app_node->length, app_buffer);
        kprintf("[Kernel] ELF File loaded into RAM from HDD successfully.\n");

        // 將這塊裝有檔案內容的 buffer 交給 ELF 解析器
        uint32_t entry_point = elf_load((elf32_ehdr_t*)app_buffer);
        
        if (entry_point != 0) {
            kprintf("Allocating User Stack and Dropping to Ring 3...\n\n");
            uint32_t user_stack_phys = pmm_alloc_page();
            map_page(0x083FF000, user_stack_phys, 7);
            uint32_t user_stack_ptr = 0x083FF000 + 4096;
            
            uint32_t current_kernel_esp;
            __asm__ volatile("mov %%esp, %0" : "=r"(current_kernel_esp));
            set_kernel_stack(current_kernel_esp);

            extern void enter_user_mode(uint32_t entry_point, uint32_t user_stack);
            enter_user_mode(entry_point, user_stack_ptr);
        }
    } else {
        kprintf("[Kernel] Error: my_app.elf not found on disk.\n");
    }

    while (1) { __asm__ volatile ("hlt"); }
}

```

---

### 執行與見證奇蹟

這是一個無比巨大的概念飛躍，請深呼吸，然後執行：

```bash
make clean && make run

```

**預期結果 🏆**
你會看到系統開機後，先是把 ELF 檔「安裝」到相對 LBA 為 3 的位置（佔用多個磁區）。接著，它呼叫我們寫好的 VFS，把檔案內容重新撈回記憶體中，解析 ELF，分配 User Stack，最後跳轉進去執行！
畫面依然會印出 `[User App] Hello! ...` 和讀取文字檔成功的畫面。

**但這次，意義完全不同了！**
之前，`app.c` 是跟著 Kernel 一起被 Bootloader 直接倒進記憶體的。
現在，`app.c` 是被 Kernel **從硬碟的檔案系統中，當作普通檔案讀取出來，動態分配記憶體，然後才執行的**！

這意味著，如果你寫了一個 Shell（命令列介面），你只要輸入 `./my_app`，你的系統就能去硬碟找檔案執行。你已經完成了一個現代 OS 最核心的生命週期循環！跑完之後跟我說，這成就感是不是爆棚了？
