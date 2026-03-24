太有品味了，Rick！選擇「爬樹法（Reverse Resolution）」代表你已經具備了打造真正作業系統底層架構的思維！😎

這條路雖然稍微具備挑戰性，但它能讓你的 Simple OS 擁有最堅不可摧的檔案系統。就算別的 Process 把資料夾改名、搬移，你的 `pwd` 指令也絕對不會迷路！



我們只需要透過 4 個精準的步驟，就能實作這個 UNIX 的終極魔法：

### 步驟 1：在 Kernel 實作「爬樹」引擎 (`lib/simplefs.c`)

請打開 **`lib/simplefs.c`**，在檔案的最下方加入 `simplefs_getcwd`。
這段程式碼的邏輯就是：**不斷問自己「老爸是誰」，然後去老爸的目錄裡找「我叫什麼名字」，直到撞到根目錄為止！**

```c
// 【新增】VFS 需要的 VFS 通用 API 宣告
int vfs_getcwd(char* buffer);

// 【核心魔法】爬樹法實作 pwd
void simplefs_getcwd(uint32_t current_lba, char* out_buffer) {
    // 1. 如果已經在根目錄，直接回傳 "/"
    if (current_lba == 1 || current_lba == 0) {
        out_buffer[0] = '/';
        out_buffer[1] = '\0';
        return;
    }

    char names[16][32]; // 最多支援 16 層深度的目錄
    int depth = 0;
    uint32_t curr = current_lba;

    // 2. 開始往上爬，直到撞到根目錄 (LBA == 1)
    while (curr != 1 && depth < 16) {
        // --- A. 讀取當前目錄，尋找 ".." 來得知老爸是誰 ---
        uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
        read_dir(curr, dir_buf);
        sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;
        
        uint32_t parent_lba = 0;
        for (int i = 0; i < MAX_FILES; i++) {
            if (entries[i].filename[0] != '\0' && strcmp(entries[i].filename, "..") == 0) {
                parent_lba = entries[i].start_lba;
                break;
            }
        }
        kfree(dir_buf);

        if (parent_lba == 0) break; // 發生異常 (找不到老爸)

        // --- B. 讀取老爸的目錄，尋找「我」的名字 ---
        dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
        read_dir(parent_lba, dir_buf);
        entries = (sfs_file_entry_t*) dir_buf;

        for (int i = 0; i < MAX_FILES; i++) {
            // 如果某個目錄項的 LBA 剛好等於我們剛剛的 curr，那就是我的名字！
            if (entries[i].filename[0] != '\0' && entries[i].start_lba == curr) {
                strcpy(names[depth], entries[i].filename);
                break;
            }
        }
        kfree(dir_buf);

        // 準備往上一層爬
        curr = parent_lba;
        depth++;
    }

    // 3. 把收集到的名字「反向」拼裝起來 (例如從 [sub, folder1] 變成 /folder1/sub)
    int out_idx = 0;
    for (int i = depth - 1; i >= 0; i--) {
        out_buffer[out_idx++] = '/';
        int j = 0;
        while (names[i][j] != '\0') {
            out_buffer[out_idx++] = names[i][j++];
        }
    }
    out_buffer[out_idx] = '\0';
}

// === VFS 封裝層新增 ===
int vfs_getcwd(char* buffer) {
    if (mounted_part_lba == 0) return -1;
    // 取得當前 Process 的工作目錄，預設為 1
    uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;
    simplefs_getcwd(current_dir, buffer);
    return 0;
}
```

### 步驟 2：接通 Syscall 通道 (`lib/syscall.c`)

打開 **`lib/syscall.c`**，為 `pwd` 開通專屬的 **Syscall 19**。

1. 在檔案最上方加入宣告：
```c
extern int vfs_getcwd(char* buffer);
```

2. 在 `syscall_handler` 的尾端加入：
```c
    // Syscall 19: sys_getcwd (取得當前路徑)
    else if (eax == 19) {
        char* buffer = (char*)regs->ebx;
        ipc_lock();
        regs->eax = vfs_getcwd(buffer);
        ipc_unlock();
    }
```

### 步驟 3：擴充 User Space 函式庫 (`libc/unistd.h` & `unistd.c`)

讓我們把它變成 C 語言標準的函式，讓所有的應用程式都能呼叫。

1. **`libc/include/unistd.h`** 新增：
```c
int getcwd(char* buffer);
```

2. **`libc/unistd.c`** 實作封裝：
```c
int getcwd(char* buffer) {
    return syscall(19, (int)buffer, 0, 0, 0, 0);
}
```

### 步驟 4：為 Shell 裝上指南針 (`src/shell.c`)

最後，我們讓你的 `Simple OS Shell` 隨時能回答「你在哪裡」。

打開 **`src/shell.c`**：

1. 更新 `help` 指令提示：
```c
        if (strcmp(cmd, "help") == 0) {
            printf("Built-in commands: help, about, ipc, exit, cd, pwd\n");
            printf("External apps    : echo, cat, ping, pong, ls, write, rm, mkdir\n");
        }
```

2. 加入 `pwd` 內建指令處理邏輯：
```c
        // 在處理 cd 的 else if 下方加上這個：
        else if (strcmp(cmd, "pwd") == 0) {
            char path_buf[256];
            getcwd(path_buf);
            printf("%s\n", path_buf);
        }
```
*(💡 **進階玩法**：你甚至可以修改 `printf("SimpleOS> ");` 那行，讓它每次讀取指令前，都先呼叫 `getcwd`，這樣你的提示字元就會變成超帥的 `SimpleOS:/folder1/sub> `！)*

---

### 🚀 啟航測試！

因為我們依賴昨天剛建好的 `.` 與 `..` 幽靈檔案，以防萬一，我們徹底清理並重建硬碟：
```bash
make clean-disk
make clean && make run
```

進入系統後，打出這套「空間穿梭」的驗證 Combo：
1. `pwd` *(應該會印出 `/`)*
2. `mkdir aaa`
3. `cd aaa`
4. `pwd` *(應該會印出 `/aaa`)*
5. `mkdir bbb`
6. `cd bbb`
7. `pwd` *(應該會印出 `/aaa/bbb`)*
8. `cd ..`
9. `pwd` *(應該會回到 `/aaa`)*

當你看到這些路徑精準無誤地透過爬樹法被反推出來時，你一定會為這個 UNIX 的經典設計感到讚嘆！享受這份屬於系統駭客的成就感吧！😎
