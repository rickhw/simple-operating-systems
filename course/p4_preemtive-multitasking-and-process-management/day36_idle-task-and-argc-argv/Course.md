早安，Rick！休息了一晚，準備好迎接 Day 36 的挑戰了嗎？🚀

昨天我們成功跨越了 OS 開發的「成年禮」—— Fork-Exec-Wait 模型。你的 Simple OS 已經從單細胞生物進化成了具備完整生命週期的多工作業系統。

不過，你也注意到了昨天最後留下的一個有趣現象：當所有 User App 都結束 (`exit`) 後，畫面上會瘋狂洗版印出 `[Kernel] No runnable tasks. CPU Idle.`。

這其實是我們進入下一個里程碑的完美契機。在真實世界的作業系統中，為了讓系統更強大、更穩定，我們會著手進行兩大進化。今天我為你準備了兩條科技樹路線，看看你想先點哪一個技能：

### 路線一：優雅的守望者 —— 實作「真正的 Idle Task (空閒任務)」
昨天我們在 `schedule()` 裡面寫死了一個無窮迴圈，當找不到活著的任務時就直接印字並 `hlt`。但這會導致中斷一觸發就瘋狂印字。
在真實的 Linux 或 Windows 中（你可以打開你的工作管理員看看），永遠會有一個佔用極少資源、**PID 為 0 的「System Idle Process」**。
* **任務內容：** 我們會在 Kernel 裡建立一個永遠不會死、權限最低的「背景守護靈」。只要大家都在睡覺或死光了，排程器就會自然而然地切換到它身上。它唯一的職責就是執行 `hlt` 讓 CPU 降溫省電。
* **好處：** 徹底解決洗版問題，讓排程器的邏輯變得極度乾淨（不用再寫特例檢查是不是沒任務了）。

### 路線二：讓程式獨立 —— 實作參數傳遞 (`argc` 與 `argv`)
現在我們的 Shell 把 `help`, `cat`, `about` 這些指令的邏輯**全部寫死**在 `app.c` 裡了。
但在真實的 UNIX 世界，`cat` 應該是一支獨立的 ELF 執行檔（例如 `/bin/cat`），Shell 只是呼叫 `exec("/bin/cat", ["cat", "hello.txt"])`。
* **任務內容：** 我們要動刀修改 `sys_exec` 的底層，在「洗腦重置」User Stack 的時候，把使用者輸入的參數字串（例如 `"hello.txt"`）**悄悄塞進新的 Stack 裡**，讓新程式的 `main(int argc, char* argv[])` 能夠順利讀取！
* **好處：** 這是打造真實應用程式生態系的關鍵！你的 OS 將可以執行各種帶參數的外部工具。

---

這兩個都是現代作業系統不可或缺的核心機制。
**「路線一」**偏向底層核心的打磨與優化，大約 15 分鐘就能完美搞定。
**「路線二」**則是應用程式介面 (API) 的大躍進，會再次動用到我們熟悉的「堆疊外科手術」，非常刺激。

Rick，你想先從哪一條路線開始今天的開發呢？還是你想一口氣把兩個都拿下？😎


---


太有魄力了，Rick！成年人不做選擇，我們今天就一口氣把這兩大核心機制給拿下！🔥

這兩條路線雖然負責的功能不同，但它們都在為你的作業系統打下「穩定性」與「擴充性」的基石。我們將先建立一個隱形的守護靈（Idle Task）來維持系統心跳，接著再進行最刺激的「堆疊外科手術」，讓外部程式能接收參數！

請繫好安全帶，我們準備動刀！

---

### 第一階段：優雅的守望者 (Idle Task)

我們要建立一個永遠不會死亡、也不會被加入一般執行佇列的 Ring 0 任務。當排程器找不到任何人可以執行時，就會自動把 CPU 交給它。

請打開 **`lib/task.c`**，進行以下三個部分的修改：

**1. 宣告 Idle Task 與它的無窮迴圈**
在檔案開頭的 global 變數區，加入這段程式碼：
```c
volatile task_t *current_task = 0;
volatile task_t *ready_queue = 0;
uint32_t next_task_id = 0;

// 【新增】Idle Task 專屬變數
task_t *idle_task = 0;

// 【新增】Idle Task 的靈魂：永遠開啟中斷並休眠
void idle_loop() {
    while(1) {
        __asm__ volatile("sti; hlt");
    }
}
```

**2. 在 `init_multitasking()` 中初始化 Idle Task**
我們需要為它偽造一個可以被 `switch_task` 彈出的堆疊：
```c
void init_multitasking() {
    kprintf("[Task] Initializing Multitasking...\n");
    task_t *main_task = (task_t*) kmalloc(sizeof(task_t));
    // ... 省略原本 main_task 的初始化 ...
    main_task->next = main_task;
    current_task = main_task;
    ready_queue = main_task;

    // 【新增】建立隱形的 Idle Task
    idle_task = (task_t*) kmalloc(sizeof(task_t));
    idle_task->id = 9999; // 給它一個特別的 ID
    idle_task->state = TASK_RUNNING;
    idle_task->wait_pid = 0;

    uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
    idle_task->kernel_stack = (uint32_t) kstack;

    // 偽造 switch_task 需要的返回堆疊 (退場位址 -> 8個通用暫存器 -> EFLAGS)
    *(--kstack) = (uint32_t) idle_loop; // RET_EIP
    for(int i = 0; i < 8; i++) *(--kstack) = 0; // POPA
    *(--kstack) = 0x0202; // POPF (IF=1)

    idle_task->esp = (uint32_t) kstack;
    // 注意：我們故意「不把」idle_task 加進 main_task->next 的環狀串列裡！
}
```

**3. 升級 `schedule()` 排程邏輯**
把原本會卡死系統的 `while(1)` 拔掉，換成切換至 Idle Task：
```c
void schedule() {
    // ... 前面的清理屍體邏輯不變 ...

    task_t *next_run = current_task->next;
    while (next_run->state != TASK_RUNNING && next_run != current_task) {
        next_run = next_run->next;
    }

    // 【修改】如果大家都死了或睡著了，就派 Idle Task 上場！
    if (next_run->state != TASK_RUNNING) {
        next_run = idle_task; 
    }

    task_t *prev = (task_t*)current_task;
    current_task = next_run;

    if (current_task != prev) {
        if (current_task->kernel_stack != 0) {
            set_kernel_stack(current_task->kernel_stack);
        }
        switch_task(&prev->esp, &current_task->esp);
    }
}
```
現在，你的系統再也不會瘋狂洗版了！當你退出所有 Shell 時，OS 會進入極度安靜且省電的待機模式。

---

### 第二階段：讓程式獨立 (實作 `argc` 與 `argv`)

接下來是最精華的堆疊外科手術。在 C 語言中，`main(int argc, char* argv[])` 的參數是依賴作業系統在啟動前，預先塞進 User Stack 裡的！

**1. 補齊必備字串工具 (`lib/utils.c`)**
我們需要測量字串長度，請在 **`lib/utils.c`** 底部加入 `strlen`，並記得在 **`lib/utils.h`** 宣告它：
```c
int strlen(const char* str) {
    int len = 0;
    while (str[len] != '\0') len++;
    return len;
}
```

**2. 大改造 `sys_exec` (`lib/task.c`)**
這是今天的重頭戲。我們要在清空的 User Stack 裡，反向塞入：字串實體 -> 指標陣列 -> main 的參數。

請打開 **`lib/task.c`**，覆蓋 `sys_exec` 函式：
```c
int sys_exec(registers_t *regs) {
    char* filename = (char*)regs->ebx;
    char** argv = (char**)regs->ecx; // 【新增】取得參數陣列的指標

    fs_node_t* file_node = simplefs_find(filename);
    if (file_node == 0) { return -1; }

    uint8_t* buffer = (uint8_t*) kmalloc(file_node->length);
    vfs_read(file_node, 0, file_node->length, buffer);
    uint32_t entry_point = elf_load((elf32_ehdr_t*)buffer);
    kfree(buffer);

    if (entry_point == 0) return -1;

    uint32_t clean_user_stack_top = 0x083FF000 - (current_task->id * 4096) + 4096;
    uint32_t stack_ptr = clean_user_stack_top;

    // ==========================================
    // 【堆疊外科手術】構造 argc 與 argv
    // ==========================================
    int argc = 0;
    if (argv != 0) {
        while (argv[argc] != 0) argc++;
    }

    uint32_t argv_ptrs[16] = {0}; // 假設最多支援 16 個參數

    // 1. 把字串的「實體內容」壓入堆疊 (從最後一個參數開始)
    for (int i = argc - 1; i >= 0; i--) {
        int len = strlen(argv[i]) + 1; // 包含結尾的 '\0'
        stack_ptr -= len;
        memcpy((void*)stack_ptr, argv[i], len);
        argv_ptrs[i] = stack_ptr; // 記錄這個字串在堆疊裡的新位址
    }

    // 堆疊位址必須對齊 4 bytes，否則 CPU 存取陣列會出錯
    stack_ptr = stack_ptr & ~3;

    // 2. 把字串的「指標陣列」壓入堆疊
    stack_ptr -= 4;
    *(uint32_t*)stack_ptr = 0; // argv 最後必須是以 NULL 結尾
    for (int i = argc - 1; i >= 0; i--) {
        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = argv_ptrs[i];
    }
    uint32_t argv_base = stack_ptr; // 這裡就是 char** argv 的位址

    // 3. 壓入 C 語言 main 函式預期的參數
    stack_ptr -= 4;
    *(uint32_t*)stack_ptr = argv_base; // 參數 2: argv
    stack_ptr -= 4;
    *(uint32_t*)stack_ptr = argc;      // 參數 1: argc
    stack_ptr -= 4;
    *(uint32_t*)stack_ptr = 0;         // 假裝的返回位址 (如果 main 執行 return，就會跳到這)

    // ==========================================

    regs->eip = entry_point;
    regs->user_esp = stack_ptr; // 【關鍵】把 ESP 更新為我們塞完參數的位址！

    return 0;
}
```

**3. 更新 Shell 邏輯 (`src/app.c`)**
我們修改 `sys_exec` 封裝，並加一個帶參數的新指令來測試！

```c
// [修改] 封裝 sys_exec，支援傳遞 argv (Syscall 9)
int sys_exec(char* filename, char** argv) {
    int ret;
    // ebx 傳檔名，ecx 傳字串陣列指標
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(9), "b"(filename), "c"(argv) : "memory");
    return ret;
}

// ... 找到 _start 裡的 while 迴圈 ...

        else if (strcmp(cmd_buffer, "run") == 0) {
            int pid = sys_fork();
            if (pid == 0) {
                // 準備傳遞給新程式的參數陣列 (最後必須是 0/NULL)
                char* my_args[] = {"my_app.elf", "Hello", "From", "Parent!", 0};
                
                sys_exec("my_app.elf", my_args);
                sys_exit(); 
            } else {
                sys_print("\n[PARENT] Spawned child. Waiting for it to finish...\n");
                sys_wait(pid);
                sys_print("[PARENT] Child has finished! I am back in control.\n");
            }
        }
```

**4. 讓 `app.c` 的 `_start` 能夠接住參數！**
為了看到效果，請修改 `app.c` 最底下的進入點：

```c
// 改變進入點的簽名，讓它能接收 OS 塞進來的參數！
void _start(int argc, char** argv) {
    sys_print("\n======================================\n");
    sys_print("      Welcome to Simple OS Shell!     \n");
    sys_print("======================================\n");
    
    // 把接收到的參數印出來證明靈魂轉移成功！
    if (argc > 1) {
        sys_print("Awesome! I received arguments:\n");
        for(int i = 0; i < argc; i++) {
            sys_print("  Arg ");
            char num[2] = {i + '0', '\0'};
            sys_print(num);
            sys_print(": ");
            sys_print(argv[i]);
            sys_print("\n");
        }
        sys_print("\n");
    }

    // ... 接下來原本的 while(1) Shell 迴圈保持不變 ...
```

---

完成這兩大進化後，執行 `make clean && make run`。
當你輸入 `run` 時，你的子 Shell 不僅會華麗重生，它還會完美地印出：
`Arg 0: my_app.elf`
`Arg 1: Hello`
`Arg 2: From`
`Arg 3: Parent!`

試試看這神奇的化學反應吧！如果要準備為下一個里程碑進行設計，你想為作業系統建立第一個真正的獨立應用程式 (例如一個可以玩的小遊戲或是小算盤)，還是想優化現在的檔案系統，讓它支援多層目錄呢？
