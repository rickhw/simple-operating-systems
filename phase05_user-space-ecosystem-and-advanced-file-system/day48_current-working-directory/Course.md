太神啦，Rick！修好了這個「跨界失憶」的指標傳遞問題後，你的目錄系統終於名副其實了！🎉

既然我們已經能夠成功建立子目錄，現在是時候進入 **Day 48：當前工作目錄 (CWD) 與 `cd` 指令！** 🚀

在我們目前的系統中，我們就像是被綁在樹根上的猴子，只能看到根目錄（Root Directory）的東西。如果我們想走進 `folder1` 裡面去建立檔案或看裡面的東西，系統必須擁有「當前工作目錄 (Current Working Directory, CWD)」的概念。



### 💡 核心哲學：為什麼 `cd` 必須是 Shell 的「內建指令」？
你有沒有想過，為什麼我們之前寫的 `ls`, `cat`, `echo` 都是獨立的 `.elf` 執行檔，唯獨 `cd` 不能這樣做？
因為在 UNIX 系統中，**CWD 是跟著「Process (行程)」走的**！
如果我們寫了一個 `cd.elf`，Shell 會 `fork()` 出一個子行程去執行它，子行程更改了自己的目錄後就 `exit()` 死掉了，**老爸 Shell 的目錄根本不會改變！** 這就是為什麼 `cd` 必須像 `exit` 一樣，深深寫死在 Shell (`app.c` / `shell.c`) 的靈魂裡！

請跟著我進行這 4 個步驟，讓我們解鎖在多層次宇宙中穿梭的能力：

---

### 步驟 1：為 Process 掛上目錄追蹤器 (`lib/task.h` & `lib/task.c`)

我們要讓每個任務都知道自己現在「站」在哪個磁區（LBA）上。

1. 打開 **`lib/task.h`**，在 `task_t` 結構中新增 `cwd_lba`：
```c
typedef struct task {
    uint32_t esp;
    uint32_t id;
    uint32_t kernel_stack;
    uint32_t page_directory; 
    uint32_t state;
    uint32_t wait_pid;
    uint32_t heap_end; 
    uint32_t cwd_lba; // 【新增】當前工作目錄的 LBA
    struct task *next;
} task_t;
```

2. 打開 **`lib/task.c`**，在 `init_multitasking`、`create_user_task` 和 `sys_fork` 這三個創造任務的函式裡，都加上初始值：
```c
    // ... 在設定 heap_end 後面加上這行 ...
    // 設定為 0，代表我們晚點會由 Syscall 自動把它導向根目錄
    new_task->cwd_lba = 0; 
    // (注意：init_multitasking 是 main_task 和 idle_task)
    // (sys_fork 則是 child->cwd_lba = current_task->cwd_lba; 繼承老爸的目錄！)
```

---

### 步驟 2：讓檔案系統支援「目錄切換」 (`lib/simplefs.c`)

目前的 API 都寫死了去讀 `part_lba + 1` (根目錄)。我們要把它改成讀取傳入的 `dir_lba`！

請打開 **`lib/simplefs.c`**：

1. **把這兩個輔助函式改名並修改：**
```c
// 拔掉原本的 read_root_dir / write_root_dir，換成這兩個通用的：
static void read_dir(uint32_t dir_lba, uint8_t* buffer) {
    for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
        ata_read_sector(dir_lba + i, buffer + (i * 512));
    }
}
static void write_dir(uint32_t dir_lba, uint8_t* buffer) {
    for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
        ata_write_sector(dir_lba + i, buffer + (i * 512));
    }
}
```

2. **新增一個用來找目錄 LBA 的函式：**
```c
// 【新增】尋找子目錄的專用函式
uint32_t simplefs_get_dir_lba(uint32_t current_dir_lba, char* dirname) {
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_dir(current_dir_lba, dir_buf);
    sfs_file_entry_t* entries = (sfs_file_entry_t*) dir_buf;

    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].filename[0] != '\0' && strcmp(entries[i].filename, dirname) == 0) {
            if (entries[i].type == FS_DIR) {
                uint32_t target_lba = entries[i].start_lba;
                kfree(dir_buf);
                return target_lba; // 找到了！回傳這個資料夾的 LBA
            }
        }
    }
    kfree(dir_buf);
    return 0; // 找不到，或它根本不是一個資料夾
}
```

3. **修改 `simplefs_readdir`：**
將裡面的 `read_root_dir(part_lba, ...)` 換成：
```c
int simplefs_readdir(uint32_t dir_lba, int index, char* out_name, uint32_t* out_size, uint32_t* out_type) {
    uint8_t* dir_buf = (uint8_t*) kmalloc(ROOT_DIR_BYTES);
    read_dir(dir_lba, dir_buf); // 【修改】讀取指定的目錄
    // ... 剩下的邏輯完全不變 ...
}
```

---

### 步驟 3：接通 `sys_chdir` 與 VFS 封裝 (`lib/syscall.c`)

現在，我們要讓作業系統的 Syscall 知道「我現在站在哪個目錄」。

請打開 **`lib/syscall.c`**：

1. **新增 `sys_chdir` (第 18 號系統呼叫)：**
```c
    // 記得在頂端宣告 extern uint32_t simplefs_get_dir_lba(...);
    // 以及 extern uint32_t mounted_part_lba;

    // Syscall 18: sys_chdir (切換目錄)
    else if (eax == 18) {
        char* dirname = (char*)regs->ebx;
        // 如果還沒有 CWD，預設就是根目錄
        uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : mounted_part_lba + 1;

        ipc_lock();
        if (strcmp(dirname, "/") == 0) {
            current_task->cwd_lba = mounted_part_lba + 1; // 回到根目錄
            regs->eax = 0;
        } else {
            uint32_t target_lba = simplefs_get_dir_lba(current_dir, dirname);
            if (target_lba != 0) {
                current_task->cwd_lba = target_lba; // 【切換成功】更新任務的 CWD！
                regs->eax = 0;
            } else {
                regs->eax = -1; // 找不到該目錄
            }
        }
        ipc_unlock();
    }
```

2. **更新 `sys_readdir` (讓 `ls` 讀取當前目錄)：**
找到你原本的 `eax == 15`，把 `vfs_readdir` 的呼叫改成：
```c
    else if (eax == 15) {
        int index = (int)regs->ebx;
        char* out_name = (char*)regs->ecx;
        uint32_t* out_size = (uint32_t*)regs->edx;
        uint32_t* out_type = (uint32_t*)regs->esi;

        // 取得目前的目錄
        uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : mounted_part_lba + 1;

        ipc_lock();
        // 【修改】直接呼叫底層，並傳入 current_dir！
        regs->eax = simplefs_readdir(current_dir, index, out_name, out_size, out_type);
        ipc_unlock();
    }
```
*(💡 備註：為了今晚先專心搞定 `cd` 和 `ls` 的導航體驗，我們暫時先不動 `mkdir` 和 `write`，讓它們依然寫在根目錄。只要導航成功，明天我們再把它們全域化！)*

---

### 步驟 4：賦予 Shell 導航能力 (`libc/unistd.c` & `shell.c`)

1. 在 **`libc/include/unistd.h`** 加入宣告：
```c
int chdir(const char* dirname);
```
2. 在 **`libc/unistd.c`** 實作：
```c
int chdir(const char* dirname) {
    return syscall(18, (int)dirname, 0, 0, 0, 0);
}
```

3. **最後一步，打開 `src/shell.c`**，在解析指令的地方加入這個內建神兵：
```c
        // 內建指令 (Built-ins)
        if (strcmp(cmd, "help") == 0) {
            // ... 略 ...
        }
        // 【新增】內建指令：cd
        else if (strcmp(cmd, "cd") == 0) {
            if (parsed_argc < 2) {
                printf("Usage: cd <directory>\n");
            } else {
                if (chdir(parsed_argv[1]) != 0) {
                    printf("cd: %s: No such directory\n", parsed_argv[1]);
                }
            }
        }
```

---

存檔！不需要 `clean-disk`，直接打下：
```bash
make clean && make run
```

進入系統後，準備見證奇蹟：
1. `mkdir folder1` (創造一個資料夾)
2. `ls` (確認 `folder1` 以 `[DIR]` 姿態出現)
3. **`cd folder1`** (踏入新世界！)
4. **`ls`** (這時候你應該會看到一個 **完全空白的清單**，因為你現在身處於 `folder1` 的深處！)
5. **`cd /`** (回到根目錄)
6. **`ls`** (原本的執行檔們又回來了！)

如果這套 Combo 成功打出來，代表你的 OS 已經真正擁有了「多層次樹狀結構」與「空間狀態追蹤」的概念了！試試看吧！🚀
