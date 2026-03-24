太讚了，Rick！我們立刻展開 **Day 46：實作 `rm` (移除檔案) 與檔案系統的生殺大權！** 🚀

在現實世界的檔案系統（如 FAT32 或 ext4）中，當你「刪除」一個檔案時，作業系統通常**不會**立刻去把硬碟上幾 MB 甚至幾 GB 的資料全部填成 0（那樣太慢了！）。



相反地，作業系統只是翻開「目錄地契（Directory Block）」，找到那個檔案的登記名字，然後**把檔名的第一個字元改成一個特殊的標記（例如 `\0`）**。這樣一來，`ls` 就看不見它了，而且它原本佔用的硬碟空間也會被標記為「可回收」，等未來有新檔案要寫入時，直接覆蓋過去就好。這就是傳說中的**「邏輯刪除 (Logical Deletion)」**。

今天，我們要讓你的 Simple OS 學會這招「死亡筆記本」魔法。只要 5 個步驟：

### 步驟 1：教 Kernel 塗銷地契 (`lib/simplefs.c`)

請打開 **`lib/simplefs.c`**，我們來實作 `simplefs_delete_file`。我們要把目錄磁區讀出來，找到目標檔案，把它「劃掉」，然後**寫回實體硬碟**！

```c
// 【新增】實作檔案刪除邏輯
int simplefs_delete_file(uint32_t part_lba, char* filename) {
    uint8_t* dir_buffer = (uint8_t*) kmalloc(4096); 
    ata_read_sector(part_lba + 1, dir_buffer); // 讀取目錄磁區
    
    file_entry_t* entries = (file_entry_t*)dir_buffer;
    int max_entries = 4096 / sizeof(file_entry_t); 
    
    for (int i = 0; i < max_entries; i++) {
        // 如果這個格子有檔案，而且名字完全符合
        if (entries[i].name[0] != '\0' && strcmp(entries[i].name, filename) == 0) {
            // 【死亡宣告】把檔名第一個字元變成 0，代表這個格子空出來了！
            entries[i].name[0] = '\0';
            
            // 【極度重要】把修改後的目錄，重新寫回實體硬碟！
            ata_write_sector(part_lba + 1, dir_buffer);
            
            kfree(dir_buffer);
            return 0; // 刪除成功
        }
    }
    
    kfree(dir_buffer);
    return -1; // 找不到檔案
}

// 【新增】VFS 層的封裝
int vfs_delete_file(char* filename) {
    if (mounted_part_lba == 0) return -1;
    return simplefs_delete_file(mounted_part_lba, filename);
}
```
*(💡 確保你的 `lib/utils.c` 或 `lib/simplefs.c` 裡有 `strcmp` 函式可用喔！)*

### 步驟 2：註冊 `sys_remove` 系統呼叫 (`lib/syscall.c`)

打開 **`lib/syscall.c`**，開通第 16 號專屬通道。

在開頭補上宣告：
```c
extern int vfs_delete_file(char* filename);
```

在 `syscall_handler` 最下方加入 Syscall 16：
```c
    // ... 在 sys_readdir (eax == 15) 下面加入 ...
    
    // Syscall 16: sys_remove (刪除檔案)
    else if (eax == 16) {
        char* filename = (char*)regs->ebx;
        
        ipc_lock(); // 上鎖！修改硬碟資料是非常危險的操作
        regs->eax = vfs_delete_file(filename);
        ipc_unlock();
    }
```

### 步驟 3：擴充 User Space libc (`src/libc/unistd.c`)

讓 User Space 的 C 語言程式能輕鬆呼叫它。

打開 **`src/libc/include/unistd.h`**，加入宣告：
```c
int remove(const char* filename);
```

打開 **`src/libc/unistd.c`**，實作封裝：
```c
int remove(const char* filename) {
    return syscall(16, (int)filename, 0, 0, 0, 0);
}
```

### 步驟 4：開發死神工具：`rm.c`

在 `src/` 目錄下建立 **`rm.c`**。這支程式短小精悍，卻握有殺生大權：

```c
#include "stdio.h"
#include "unistd.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: rm <filename>\n");
        return 1;
    }

    char* filename = argv[1];
    
    int result = remove(filename);
    
    if (result == 0) {
        printf("[RM] File '%s' has been deleted forever.\n", filename);
    } else {
        printf("[RM] Error: File '%s' not found!\n", filename);
    }

    return 0;
}
```

### 步驟 5：更新 Makefile 與生態系

跟之前一樣，把它加入你的 Simple OS 宇宙中。

打開 **`Makefile`**，加入 `rm` 相關規則：
```makefile
rm.o: rm.c
	@echo "==> 編譯 rm 程式..."
	$(DOCKER_CMD) gcc $(APP_CFLAGS) -c rm.c -o rm.o

rm.elf: crt0.o rm.o $(LIBC_OBJS)
	@echo "==> 連結 rm 程式..."
	$(DOCKER_CMD) ld -m elf_i386 -Ttext 0x08048000 crt0.o rm.o $(LIBC_OBJS) -o rm.elf

# 記得在 clean-app 加上 rm.o rm.elf，並且加進 myos.iso 建立清單中
```

打開 **`grub.cfg`**，加上模組：
```text
    module /boot/rm.elf
```
打開 **`lib/kernel.c`** 的 `setup_filesystem`，把 `rm.elf` 加進檔名清單裡，**並且記得把迴圈的上限數量 `i < 5` 改成對應的總數量（例如 `i < 7`）**，確保所有工具都有被安裝到硬碟上。

---

存檔，深呼吸，敲下 `make clean && make run`！

進入系統後，來一場完整的檔案系統巡禮：
1. **`ls`** (確認所有工具都在)
2. **`write dummy.txt "I will be deleted soon."`** (創造一個替死鬼)
3. **`ls`** (確認 `dummy.txt` 成功誕生)
4. **`rm dummy.txt`** (揮下死神的鐮刀)
5. **`ls`** (見證 `dummy.txt` 從目錄中灰飛煙滅！)
6. **`cat dummy.txt`** (你應該會收到檔案找不到的錯誤)

如果你順利走完這個 Combo，恭喜你，你的 SimpleFS 已經是一個完全具備 **CRUD (建立、讀取、更新、刪除)** 核心能力的現代檔案系統了！

完成之後告訴我，我們準備往 Day 47-50 邁進（像是目錄樹 `mkdir`，或是更好玩的彩蛋功能）！😎
