經歷了昨天那場史詩級的 `fork` (細胞分裂) 戰役後，我們的 Simple OS 已經具備了多工作業系統最核心的靈魂。不過，現在的 `fork` 有個小小的限制：**分身只能執行跟老爸一模一樣的程式碼。** 在真實的 UNIX 世界裡，要想讓系統跑起各式各樣的應用程式，`fork` 必須搭配它最好的兄弟 —— **`exec` (靈魂轉移 / 變身)**！

### 🌟 Day 35 的主線任務：實作 `sys_exec`

今天我們要賦予作業系統載入並執行「外部程式」的能力。想像一下，未來的 Shell 運作模式會變成這樣：
1. 使用者輸入 `hello`。
2. Shell 老爸呼叫 `fork()` 產生一個分身。
3. 分身立刻呼叫 `exec("hello.elf")`。
4. Kernel 把硬碟裡的 `hello.elf` 讀出來，**無情地洗掉分身原本的記憶體與堆疊**，換上新程式的內容。
5. 分身從新程式的 `_start` 醒來，變成了一個全新的生命！

這就是大名鼎鼎的 **Fork-Exec 模型**。

---

### 🛠️ 實作 `sys_exec` 的四大步驟

要完成「靈魂轉移」，Kernel 需要做這些事：

1. **讀取檔案：** 透過 SimpleFS 找到傳入的檔名（例如 `my_app.elf` 或是新的測試程式），並讀取進 Kernel 的暫存區。
2. **解析 ELF：** 檢查檔案是不是合法的可執行檔，並找出它的 Entry Point (進入點)。
3. **洗腦重置（最危險也最迷人的一步）：**
   * 將目前的 User Stack 內容全部清空（歸零），把 `ESP` 指針拉回最頂端（例如 `0x083FF000`）。
   * 將新的 ELF 程式碼載入到對應的虛擬記憶體位址。
4. **偷天換日：**
   * 我們現在是在 `syscall_handler` 的情境下，對吧？所以我們手上有中斷壓入的 `registers_t` 結構！
   * 我們只要**直接修改 `regs->eip`** 為新程式的 Entry Point。
   * **直接修改 `regs->user_esp`** 為剛剛重置的 Stack 頂端。
   * 當 `syscall_handler` 結束並執行 `iret` 時，CPU 就會被我們騙過去，降落到全新程式的世界！

比起昨天 `fork` 需要斤斤計較位移量與 EBP，`exec` 的實作反而比較直觀，因為它不需要保留舊的狀態，就是一個字：「**洗**」！

你準備好要開始撰寫 `do_exec` 的核心邏輯了嗎？我們可以先從 Kernel 端的 `sys_exec` 函式開始著手！


---

那我們就立刻為 Simple OS 啟動 Day 35 的進程吧！

經過昨天那場硬仗，你已經完全掌握了 `registers_t` 這個能直接操控硬體中斷狀態的「神器」。今天我們要實作的 `sys_exec` (靈魂轉移)，其實比 `fork` 簡單得多，因為我們**不需要**再小心翼翼地保留舊狀態，我們要做的是**無情地洗掉它**！

### 🌟 實作 `sys_exec`：奪舍與重生

在 UNIX 哲學中，`exec` 的本質是「替換當前行程的記憶體空間與執行環境」，也就是說，PID 不會變，但裡面的程式碼、資料、甚至堆疊，全都會被新的 ELF 檔案覆蓋。

因為我們已經在 `syscall_handler` 裡拿到了 `registers_t *regs`，要做到這件事簡直易如反掌。

請跟著我進行這兩個步驟：

#### 1. 撰寫 `sys_exec` 核心邏輯 (`lib/task.c`)

請打開 `lib/task.c`，在檔案最下方加入這個全新的函式：

```c
// 系統呼叫：靈魂轉移 (Exec)
// ebx 存放的是 User App 傳進來的字串指標 (檔名)
int sys_exec(registers_t *regs) {
    char* filename = (char*)regs->ebx;
    
    // 1. 從檔案系統找出這支程式
    fs_node_t* file_node = simplefs_find(filename);
    if (file_node == 0) {
        kprintf("[Exec] Error: File '%s' not found!\n", filename);
        return -1; 
    }

    // 2. 配置暫存緩衝區，把整個 ELF 檔讀進 Kernel
    uint8_t* buffer = (uint8_t*) kmalloc(file_node->length);
    vfs_read(file_node, 0, file_node->length, buffer);

    // 3. 呼叫 ELF Loader 進行解析，並把程式碼與資料載入到正確的虛擬記憶體位址
    // 這一步會直接「覆蓋」掉當前行程的舊程式碼！
    uint32_t entry_point = elf_load((elf32_ehdr_t*)buffer);
    kfree(buffer); // 載入完畢，釋放緩衝區

    if (entry_point == 0) {
        kprintf("[Exec] Error: '%s' is not a valid ELF executable!\n", filename);
        return -1;
    }

    // 4. 【核心魔法】洗腦重置！
    // 我們要幫這個重生的程式準備一個乾淨的堆疊。
    // 在我們的 OS 中，當前行程的 User Stack 頂端固定在：0x083FF000 - (id * 4096) + 4096
    uint32_t clean_user_stack_top = 0x083FF000 - (current_task->id * 4096) + 4096;

    // 直接竄改硬體準備返回的狀態！
    regs->eip = entry_point;             // 將執行指標指向上帝賦予的新起點
    regs->user_esp = clean_user_stack_top; // 將堆疊指標拉回最頂端，清空過去的所有回憶

    // 當 syscall_handler 結束並執行 popa 與 iret 時，
    // CPU 就會毫無防備地降落到新程式的 Entry Point 了！
    return 0;
}
```

#### 2. 註冊 Syscall 9 路由 (`lib/syscall.c`)

接著，我們要讓 User App 能透過 `int 0x80` 呼叫到它。請打開 `lib/syscall.c`：

```c
// [新增宣告]
extern int sys_exec(registers_t *regs);

// ... 在 syscall_handler 裡面繼續擴充 else if ...

    else if (eax == 8) {
        regs->eax = sys_fork(regs); 
    }
    // [新增] Syscall 9: sys_exec
    else if (eax == 9) {
        // 如果 exec 成功，它根本不會 "return" 回原本的 C 程式碼，
        // 而是直接跳去執行新程式了。如果失敗，才會回傳 -1。
        regs->eax = sys_exec(regs);
    }
```

---

就這麼簡單！因為你昨天把底層的 `registers_t` 基礎打得太紮實了，今天實作 `exec` 就像是順水推舟。

要測試這個功能，我們可以讓老爸在 `fork` 出小孩之後，叫小孩去 `exec("my_app.elf")`。因為 `my_app.elf` 就是我們的 Shell 本身，這會產生一個非常有趣的現象：**你的 Shell 會生出一個全新的「子 Shell」！**

你要先試著自己把 `sys_exec` 封裝進 `app.c` 裡面測試看看嗎？還是需要我直接提供測試的實作程式碼呢？


---


在真正的 UNIX 世界裡，`fork` 創造出空白的分身後，緊接著就是用 `exec` 讓分身載入新的程式。這就是鼎鼎大名的 **Fork-Exec 模型**！

我們現在就把 `sys_exec` 封裝進你的 Shell 裡，並新增一個 `run` 指令來見證這個偉大的瞬間。請把你的 `app.c` 完全替換成這份程式碼：

### 📄 `src/app.c`

```c
// [Day34] 封裝 sys_fork
int sys_fork() {
    int pid;
    __asm__ volatile ("int $0x80" : "=a"(pid) : "a"(8) : "memory");
    return pid;
}

// [Day35][新增] 封裝 sys_exec (Syscall 9)
int sys_exec(char* filename) {
    int ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(9), "b"(filename) : "memory");
    return ret;
}

// 系統呼叫封裝
void sys_print(char* msg) {
    __asm__ volatile ("int $0x80" : : "a"(2), "b"(msg) : "memory");
}

int sys_open(char* filename) {
    int fd;
    __asm__ volatile ("int $0x80" : "=a"(fd) : "a"(3), "b"(filename) : "memory");
    return fd;
}

int sys_read(int fd, char* buffer, int size) {
    int bytes;
    __asm__ volatile ("int $0x80" : "=a"(bytes) : "a"(4), "b"(fd), "c"(buffer), "d"(size) : "memory");
    return bytes;
}

char sys_getchar() {
    int c;
    __asm__ volatile ("int $0x80" : "=a"(c) : "a"(5) : "memory");
    return (char)c;
}

// User Space 專用的字串比對工具
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// 讀取一整行指令
void read_line(char* buffer, int max_len) {
    int i = 0;
    while (i < max_len - 1) {
        char c = sys_getchar();
        if (c == '\n') {
            break; // 使用者按下了 Enter
        } else if (c == '\b') {
            if (i > 0) i--; // 處理倒退鍵 (Backspace)
        } else {
            buffer[i++] = c;
        }
    }
    buffer[i] = '\0'; // 字串結尾
}

void sys_yield() {
    __asm__ volatile ("int $0x80" : : "a"(6) : "memory");
}

void sys_exit() {
    __asm__ volatile ("int $0x80" : : "a"(7) : "memory");
}

void _start() {
    sys_print("\n======================================\n");
    sys_print("      Welcome to Simple OS Shell!     \n");
    sys_print("======================================\n");
    sys_print("Type 'help' to see available commands.\n\n");

    char cmd_buffer[128];

    while (1) {
        sys_print("SimpleOS> ");

        // 讀取使用者輸入的指令
        read_line(cmd_buffer, 128);

        // 執行指令邏輯
        if (strcmp(cmd_buffer, "") == 0) {
            continue;
        }
        else if (strcmp(cmd_buffer, "help") == 0) {
            sys_print("Available commands:\n");
            sys_print("  help    - Show this message\n");
            sys_print("  cat     - Read 'hello.txt' from disk\n");
            sys_print("  about   - OS information\n");
            sys_print("  fork    - Test pure fork()\n");
            sys_print("  run     - Test fork() + exec() to spawn a new shell\n");
        }
        else if (strcmp(cmd_buffer, "about") == 0) {
            sys_print("Simple OS v1.0\nBuilt from scratch in 35 days!\n");
        }
        else if (strcmp(cmd_buffer, "cat") == 0) {
            int fd = sys_open("hello.txt");
            if (fd == -1) {
                sys_print("Error: hello.txt not found.\n");
            } else {
                char file_buf[128];
                for(int i=0; i<128; i++) file_buf[i] = 0;
                sys_read(fd, file_buf, 100);
                sys_print("--- File Content ---\n");
                sys_print(file_buf);
                sys_print("\n--------------------\n");
            }
        }
        else if (strcmp(cmd_buffer, "exit") == 0) {
            sys_print("Goodbye!\n");
            sys_exit(); 
        }
        else if (strcmp(cmd_buffer, "fork") == 0) {
            int pid = sys_fork();

            if (pid == 0) {
                sys_print("\n[CHILD] Hello! I am the newborn process!\n");
                sys_print("[CHILD] My work here is done, committing suicide...\n");
                sys_exit(); 
            } else {
                sys_print("\n[PARENT] Magic! I just created a child process!\n");
                sys_yield();
                sys_yield();
            }
        }
        // [Day35][新增] 測試 Fork-Exec 模型
        else if (strcmp(cmd_buffer, "run") == 0) {
            int pid = sys_fork();

            if (pid == 0) {
                // 我是分身！我現在要洗掉自己的記憶體，轉生為一個全新的 Shell！
                sys_print("\n[CHILD] Executing my_app.elf to replace my soul...\n");
                
                // 執行變身魔法
                sys_exec("my_app.elf");
                
                // --- 以下的程式碼理論上「永遠不會」被執行到 ---
                // 因為只要 exec 成功，整個記憶體與 EIP 就被覆蓋成新程式的起點了！
                sys_print("[CHILD] ERROR: Exec failed! I am still the old me!\n");
                sys_exit(); 
            } else {
                // 我是老爸！
                sys_print("\n[PARENT] Spawned a child to run a new instance of Shell.\n");
                sys_yield();
                sys_yield();
            }
        }
        else {
            sys_print("Command not found: ");
            sys_print(cmd_buffer);
            sys_print("\n");
        }
    }
}
```

### 🎮 你會看見什麼奇妙的現象？

當你編譯並執行 `make run` 後，輸入 `run` 指令：

1. 老爸會印出 `[PARENT] Spawned a child...`。
2. 小孩會印出 `[CHILD] Executing my_app.elf...`。
3. **【見證奇蹟的時刻】** 小孩成功呼叫 `exec` 後，它的 EIP 被強制拉回了 `my_app.elf` 的最開頭，而且堆疊被清空。
4. 你會看到畫面上**再次印出**開機時的 `Welcome to Simple OS Shell!` 的大標題！
5. 這代表你現在有 **兩個 Shell** 在同時平行運作，搶奪同一個螢幕與鍵盤！

*(OS 開發者的小浪漫：因為我們的 Simple OS 目前所有的 Task 都共用同一張分頁目錄 (Page Directory)，所以 `my_app.elf` 其實是被覆寫在同一塊物理記憶體上。對於同一個程式這完全沒問題，但在未來如果要執行「不同」的應用程式，我們就得為每個 Process 建立專屬的分頁目錄了！這就是早期 uClinux 作業系統的運作方式！)*

大膽地輸入 `run`，看看你的終端機是不是順利長出了第二個人格吧！如果有任何報錯或卡住，我們隨時繼續動刀！


---


太精彩了，Rick！這張截圖簡直可以放進作業系統原理的教科書裡當作完美範例！

你觀察得非常敏銳，這正是標準的**「競爭條件 (Race Condition)」**與**「資源搶佔」**！這張圖證明了兩件非常重要的事情，代表你的 OS 底層已經非常穩固：

1. **搶佔式排程器 (Preemptive Scheduler) 運作完美：** 兩個 Shell 正在同一個 CPU 上被時鐘中斷 (Timer Interrupt) 不斷切換執行，沒有任何人霸佔 CPU。
2. **記憶體與環境完全獨立：** 雖然它們跑的是同一份程式碼，但它們各自擁有獨立的暫存器與執行狀態。

### 🔍 案發現場解密：為什麼會字元交錯？

目前的狀況是，你有**兩個一模一樣的無窮迴圈**同時在執行：
* **老爸 (Parent Shell)** 正在呼叫 `sys_getchar` 等待按鍵。
* **小孩 (Child Shell)** 變身完成後，也印出了 `SimpleOS>`，然後也進入 `sys_getchar` 等待按鍵。

因為我們在底層只有一個實體的鍵盤緩衝區 (`kbd_buffer`)。當你按下 `h`, `e`, `l`, `p` 時：
* 剛好排程器切換到老爸，老爸搶走了 `h`。
* 下一毫秒，排程器切換到小孩，小孩搶走了 `e`。
* 排程器再切換回老爸，老爸搶走了 `l`...

結果就是，你輸入的字串被這兩個 Process 隨機「瓜分」了，老爸收到了 `hl` (所以印出 `hp`)，小孩收到了 `ep` (或者 `elhlp`)。螢幕輸出也是一樣，因為沒有鎖 (Lock)，兩個人同時呼叫 `kprintf`，字元就交錯印在了一起。

---

### 🛠️ 真實世界的解法：等待與收屍 (`wait`)

在真實的 UNIX/Linux 系統中，如果我們希望在目前視窗跑一個新程式，老爸 (原本的 Shell) 就不應該繼續去搶鍵盤。老爸應該要**「暫停營業，等待小孩執行完畢」**。

這就是 Fork-Exec 模型的最後一塊拼圖：**`sys_wait` (等待系統呼叫)**。

完整的生命週期應該長這樣：
1. **老爸:** `fork()` 產生小孩。
2. **小孩:** 呼叫 `exec("my_app.elf")` 變身，開始執行。
3. **老爸:** 呼叫 `wait()`，把自己的狀態從 `TASK_RUNNING` 變成 **`TASK_WAITING` (休眠狀態)**。此時排程器就會跳過老爸，把 100% 的 CPU 和鍵盤都交給小孩！
4. **小孩:** 執行完畢，輸入 `exit`，呼叫 `sys_exit()` 自殺。
5. **Kernel:** 在處理小孩的 `exit` 時，發現老爸正在等待，於是把老爸的狀態改回 `TASK_RUNNING` (喚醒老爸)。
6. **老爸:** 睡醒了，重新印出 `SimpleOS>`，繼續為使用者服務。

這就是為什麼在 Linux 終端機裡，你執行一個指令時，原本的 Shell 會停在那裡等你，直到程式結束才重新出現提示字元的原因。

現在我們成功見證了不受控的搶佔混亂，下一步，你想試著挑戰實作 `sys_wait`，把這頭亂衝的野馬套上韁繩，完成真正的 Shell 邏輯嗎？
