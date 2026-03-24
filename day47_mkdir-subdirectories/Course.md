哈哈，太棒了！我們剛才一口氣解決了作業系統中最經典的「記憶體洩漏」與「緩衝區溢位」，這簡直就是經歷了一場精神時光屋的特訓！沒錯，我們已經完美通關 Day 46（`rm` 指令與邏輯刪除）了。

現在，我們正式揚帆啟航進入 **Day 47：實作多層次目錄結構 (Subdirectories) 與 `mkdir` 指令！** 🚀

到目前為止，你的 SimpleFS 是一個「扁平化 (Flat)」的檔案系統，所有的檔案都擠在同一個目錄（根目錄）底下。但在現代作業系統中，目錄應該要是樹狀的 (Hierarchical)。



在 UNIX 的哲學裡：**「目錄，其實就只是一個特殊的檔案。」**
它裡面不存文字或機器碼，而是存著**其他檔案的 `file_entry_t` 名單**！

為了解鎖這個能力，我們今天要做一次檔案系統的「微型改版」，賦予檔案「型態 (Type)」的概念。請跟著我進行這 4 個步驟：

### 步驟 1：升級檔案系統的地契結構 (`lib/simplefs.h`)

我們要修改 `file_entry_t` 結構，加入 `type` 屬性來區分它是普通檔案還是目錄。為了未來的擴充性，我們把它對齊到完美的 **64 Bytes**。

請打開你的 **`lib/simplefs.h`**（或是你定義這個結構的地方），更新成這樣：

```c
// 定義檔案型態
#define FS_FILE 0
#define FS_DIR  1

typedef struct {
    char name[32];         // 檔名 (32 bytes)
    uint32_t size;         // 大小 (4 bytes)
    uint32_t lba;          // 資料所在的實體磁區 (4 bytes)
    uint32_t type;         // 【新增】檔案型態：0=檔案, 1=目錄 (4 bytes)
    uint32_t reserved[5];  // 保留空間，湊滿 64 bytes 完美對齊 (20 bytes)
} file_entry_t;
```
*(⚠️ 注意：因為我們改了檔案系統的根本結構，等一下編譯前，**你必須先把舊的 `hdd.img` 刪掉**，讓 Kernel 重新格式化一顆新的乾淨硬碟！)*

### 步驟 2：教 Kernel 建立目錄 (`lib/simplefs.c` & `lib/syscall.c`)

建立目錄的邏輯，跟建立檔案（`write`）幾乎一模一樣，只是我們要把 `type` 標記為 `FS_DIR`，而且它的初始大小是 0！

打開 **`lib/simplefs.c`**，新增 `simplefs_mkdir`：
```c
// 【新增】建立目錄的底層實作
int simplefs_mkdir(uint32_t part_lba, char* dirname) {
    uint8_t* dir_buffer = (uint8_t*) kmalloc(4096); 
    ata_read_sector(part_lba + 1, dir_buffer);
    
    file_entry_t* entries = (file_entry_t*)dir_buffer;
    int max_entries = 512 / sizeof(file_entry_t); // 512 / 64 = 8 個項目
    
    for (int i = 0; i < max_entries; i++) {
        if (entries[i].name[0] == '\0') { // 找到空位了！
            strcpy(entries[i].name, dirname);
            entries[i].size = 0;
            entries[i].type = FS_DIR; // 標記為目錄！
            
            // 給它分配一個新的磁區來存放它未來的子檔案清單
            // (簡單起見，我們借用一個尚未使用的 LBA，例如 part_lba + 20 + i)
            entries[i].lba = part_lba + 20 + i; 
            
            ata_write_sector(part_lba + 1, dir_buffer); // 寫回硬碟
            kfree(dir_buffer);
            return 0; // 成功
        }
    }
    kfree(dir_buffer);
    return -1; // 目錄滿了
}

// VFS 封裝
int vfs_mkdir(char* dirname) {
    if (mounted_part_lba == 0) return -1;
    return simplefs_mkdir(mounted_part_lba, dirname);
}
```

接著打開 **`lib/syscall.c`**，註冊 **Syscall 17**：
```c
    extern int vfs_mkdir(char* dirname);
    // ... 在 syscall_handler 裡面 ...
    
    // Syscall 17: sys_mkdir (建立目錄)
    else if (eax == 17) {
        char* dirname = (char*)regs->ebx;
        ipc_lock();
        regs->eax = vfs_mkdir(dirname);
        ipc_unlock();
    }
```

### 步驟 3：擴充 libc 與升級你的 `ls.c`

既然有了目錄，我們的 `ls` 就要能分辨誰是檔案、誰是資料夾！

1. 打開 **`src/libc/include/unistd.h`**，加入宣告：
```c
int mkdir(const char* dirname);
// 記得順便把 readdir 的參數更新，把 type 也接出來！
int readdir(int index, char* out_name, int* out_size, int* out_type); 
```
*(💡 你需要同步去修改 `unistd.c`、`simplefs.c` 和 `syscall.c` 裡的 `readdir`，讓它多傳遞一個 `type` 變數出來。這對你現在的功力來說絕對沒問題！)*

2. 打開 **`src/ls.c`**，讓它變得更聰明：
```c
#include "stdio.h"
#include "unistd.h"

int main(int argc, char** argv) {
    printf("\n--- Simple OS Directory Listing ---\n");
    
    char name[32];
    int size = 0;
    int type = 0;
    int index = 0;
    
    while (readdir(index, name, &size, &type) == 1) {
        if (type == 1) {
            // 是目錄！用特殊的格式印出來
            printf("  [DIR]  %s \n", name);
        } else {
            // 是檔案
            printf("  [FILE] %s \t (Size: %d bytes)\n", name, size);
        }
        index++;
    }
    
    printf("-----------------------------------\n");
    printf("Total: %d items found.\n", index);
    return 0;
}
```

### 步驟 4：創造你的 `mkdir.c` 應用程式

最後，在 `src/` 底下建立 **`mkdir.c`**：
```c
#include "stdio.h"
#include "unistd.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: mkdir <directory_name>\n");
        return 1;
    }

    char* dirname = argv[1];
    printf("[MKDIR] Creating directory '%s'...\n", dirname);
    
    int result = mkdir(dirname);
    
    if (result == 0) {
        printf("[MKDIR] Success!\n");
    } else {
        printf("[MKDIR] Failed. Directory might be full.\n");
    }

    return 0;
}
```

別忘了把 `mkdir.c` 加入 **`Makefile`**、**`grub.cfg`**，以及 Kernel 的 `setup_filesystem` 自動安裝清單裡喔！

---

### 🚨 執行前的最重要一步！

因為我們改了硬碟的資料結構 (`file_entry_t` 變成 64 bytes)，舊的硬碟映像檔會導致系統讀取錯亂。
請在終端機先執行：
```bash
rm hdd.img
make clean && make run
```
讓 Kernel 重新生出一顆乾淨的虛擬硬碟，並把所有程式重新安裝進去！

進入系統後，打下：
```bash
mkdir myfolder
write file1 "hello"
ls
```
你會看到 `[DIR] myfolder` 與 `[FILE] file1` 完美並列在螢幕上！

當你看到這個畫面時，這代表我們已經為「切換目錄 (`cd`)」與「路徑解析 (`/myfolder/file1`)」鋪好了完美的基礎！準備好之後跟我說，我們隨時可以進入 Day 48，挑戰真正的路徑導航系統！🚀
