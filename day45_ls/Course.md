太棒了，Rick！我們馬不停蹄地進入 **Day 45：實作 `ls` 指令與目錄讀取能力！** 🚀

在 Day 44，你已經讓作業系統擁有了「造物」的能力（`write.elf`），可以動態建立檔案了。但是，你有沒有發現我們現在處於一個很瞎的狀態？—— **我們寫了檔案，卻沒辦法在 Shell 裡面「看到」它！** 我們只能依賴 Kernel 在開機時印出的那份靜態清單。

在 UNIX 哲學中，User Space 的應用程式應該要有能力向 OS 查詢目錄結構。今天，我們要在 Kernel 開通第 15 號系統呼叫 (`sys_readdir`)，並且打造 UNIX 中最經典的工具：**`ls.elf`**。

請跟著我進行這 5 個步驟：

### 步驟 1：教 Kernel 回報目錄內容 (`lib/simplefs.c`)

你的 SimpleFS 裡面原本就有一個 `simplefs_list_files` 函式，但它是直接用 `kprintf` 印在 Kernel 畫面上。現在我們要寫一個給 User Space 呼叫的版本：每次傳入一個 `index`（第幾個檔案），就把檔名和大小回傳出去。

請打開 **`lib/simplefs.c`**，加入這兩個函式：

```c
// 【新增】讀取第 index 個檔案的資訊
int simplefs_readdir(uint32_t part_lba, int index, char* out_name, uint32_t* out_size) {
    // 假設我們的根目錄位在 part_lba + 1 (這取決於你當時 simplefs_format 的設計)
    // 通常根目錄就是一個 block，我們把它讀出來
    uint8_t* dir_buffer = (uint8_t*) kmalloc(4096); 
    ata_read_sector(part_lba + 1, dir_buffer); // 讀取第一個目錄磁區
    
    // 假設每個 file_entry_t 是 64 bytes (32 bytes 檔名 + 32 bytes 屬性)
    int valid_count = 0;
    for (int i = 0; i < 512 / 64; i++) {
        uint8_t* entry = dir_buffer + (i * 64);
        
        // 如果檔名的第一個字元不是 0，代表這是一個有效的檔案
        if (entry[0] != '\0') {
            if (valid_count == index) {
                // 找到了！把檔名拷貝到 User 傳進來的緩衝區
                for (int j = 0; j < 32; j++) {
                    out_name[j] = entry[j];
                }
                // 取出檔案大小 (假設放在檔名後面的 offset 32 處)
                *out_size = *(uint32_t*)(entry + 32);
                
                kfree(dir_buffer);
                return 1; // 成功找到並回傳
            }
            valid_count++;
        }
    }
    
    kfree(dir_buffer);
    return 0; // 找不到 (通常代表目錄到底了)
}

// 【新增】VFS 層的封裝
int vfs_readdir(int index, char* out_name, uint32_t* out_size) {
    if (mounted_part_lba == 0) return -1;
    return simplefs_readdir(mounted_part_lba, index, out_name, out_size);
}
```
*(💡 註：請確認 `ata_read_sector` 和 `entry` 的偏移量符合你當初在 Day 25 設計 `simplefs_create_file` 時的資料結構。)*

### 步驟 2：註冊 `sys_readdir` 系統呼叫 (`lib/syscall.c`)

打開 **`lib/syscall.c`**，開通專屬的通道。

在開頭補上宣告：
```c
extern int vfs_readdir(int index, char* out_name, uint32_t* out_size);
```

在 `syscall_handler` 的最下方加入 Syscall 15：
```c
    // ... 在 sys_create (eax == 14) 下面加入 ...
    
    // Syscall 15: sys_readdir (讀取目錄內容)
    else if (eax == 15) {
        int index = (int)regs->ebx;
        char* out_name = (char*)regs->ecx;
        uint32_t* out_size = (uint32_t*)regs->edx;
        
        ipc_lock();
        regs->eax = vfs_readdir(index, out_name, out_size);
        ipc_unlock();
    }
```

### 步驟 3：擴充 User Space libc (`src/libc/unistd.c`)

讓我們優雅地封裝這個系統呼叫。

打開 **`src/libc/include/unistd.h`**，加入宣告：
```c
int readdir(int index, char* out_name, int* out_size);
```

打開 **`src/libc/unistd.c`**，實作封裝：
```c
int readdir(int index, char* out_name, int* out_size) {
    return syscall(15, index, (int)out_name, (int)out_size, 0, 0);
}
```

### 步驟 4：開發經典工具：`ls.c`

現在，我們終於可以來寫 UNIX 系統中最常被敲擊的指令了！

在 `src/` 目錄下建立 **`ls.c`**：
```c
#include "stdio.h"
#include "unistd.h"

int main(int argc, char** argv) {
    (void)argc; // 暫時用不到參數
    (void)argv;

    printf("\n--- Simple OS Directory Listing ---\n");
    
    char name[32];
    int size = 0;
    int index = 0;
    
    // 不斷向 OS 要下一個檔案，直到 OS 說沒有為止
    while (readdir(index, name, &size) == 1) {
        printf("  [FILE] %s \t (Size: %d bytes)\n", name, size);
        index++;
    }
    
    printf("-----------------------------------\n");
    printf("Total: %d files found.\n", index);

    return 0;
}
```

### 步驟 5：更新 Makefile 與 GRUB

把你的新法寶加入生態系中！

打開 **`Makefile`**，加入 `ls` 相關規則：
```makefile
# ... 略 ...
ls.o: ls.c
	@echo "==> 編譯 ls 程式..."
	$(DOCKER_CMD) gcc $(APP_CFLAGS) -c ls.c -o ls.o

ls.elf: crt0.o ls.o $(LIBC_OBJS)
	@echo "==> 連結 ls 程式..."
	$(DOCKER_CMD) ld -m elf_i386 -Ttext 0x08048000 crt0.o ls.o $(LIBC_OBJS) -o ls.elf

# 記得在 clean-app 加上 ls.o ls.elf
# 並且在 myos.iso 的依賴與 cp 指令中加上 ls.elf！
```

然後打開 **`grub.cfg`**，加上模組：
```text
    module /boot/ls.elf
```
打開 **`lib/kernel.c`** 的 `setup_filesystem`，把 `ls.elf` 加進安裝清單裡（陣列記得擴充）。

---

深呼吸，敲下 `make clean && make run`！

進入系統後，你現在可以進行一套超帥的 Combo 連擊：
1. 輸入 **`ls`**，你會看到目前硬碟上的所有 `.elf` 執行檔和文字檔。
2. 輸入 **`write magic.txt "I am making magic!"`**，創造一個新檔案。
3. 再次輸入 **`ls`**！

如果你在清單中看到了熱騰騰剛出爐的 `magic.txt`，就代表你的 Simple OS 已經徹底具備了一個動態、可讀寫、可探索的現代檔案系統！

準備好測試你的 `ls` 指令了嗎？成功之後，你想不想挑戰 Day 46：為檔案系統加入「刪除檔案 (`rm`)」的功能呢？
