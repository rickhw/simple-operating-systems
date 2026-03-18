哈哈，Rick！我們遭遇了作業系統開發中最經典的「第一因 (First Cause)」問題！

你的分析非常敏銳，系統在重啟循環 (Reboot Loop)，而且 Dump 記錄了第一案發現場：
```text
    4: v=0e e=0004 i=0 cpl=3 IP=001b:080481a9 pc=080481a9 SP=0023:083ffecc CR2=08400004
```
**Exception 0x0E (Page Fault)**，而且 `CR2` (引發缺頁的記憶體位址) 是 **`0x08400004`**！

### 🔍 案發現場解密：第一支程式的「參數真空」

為什麼會存取 `0x08400004` 這個位址？
回想一下，我們在上一回合為了實作 `argc` 和 `argv`，把 `app.c` 的進入點改成了這樣：
```c
void _start(int argc, char** argv) {
    if (argc > 1) { ... }
}
```

當編譯器看到 `argc` 時，它會把它編譯成存取堆疊的指令：`讀取 [EBP + 8]`。
對於被 `sys_exec` 變身出來的程式，這完全沒問題，因為我們在 `sys_exec` 的「堆疊外科手術」中，已經乖乖把 `argc` 和 `argv` 塞進堆疊裡了。

**但是，第一支被啟動的 Shell 呢？**
在 `kernel.c` 中，我們是直接呼叫 `create_user_task` 啟動它的！
```c
create_user_task(entry_point, 0x083FF000 + 4096);
```
這代表初始 Shell 的堆疊指針 (`ESP`) 被我們設定在死硬的最高處 `0x08400000`。
當 `_start` 執行，它試著去讀取 `[EBP + 8]` 時，它等於是去讀取 `0x08400008`！
但我們的記憶體分頁只配置到 `0x083FFFFF`，越過界線的瞬間，系統當場 Page Fault 暴斃！

### 🗡️ 終極修復：為「始祖」注入靈魂參數

解法非常簡單：我們只要在 `create_user_task` 裡，也幫這支始祖程式準備一個「空」的參數清單 (`argc = 0`)，讓它不至於去讀取未定義的空間就行了。

請打開 **`lib/task.c`**，找到 `create_user_task`，把「構造硬體 IRET 幀」的部分替換成以下這樣：

```c
void create_user_task(uint32_t entry_point, uint32_t user_stack_top) {
    task_t *new_task = (task_t*) kmalloc(sizeof(task_t));
    new_task->id = next_task_id++;
    new_task->state = TASK_RUNNING;
    new_task->wait_pid = 0;

    uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
    new_task->kernel_stack = (uint32_t) kstack;

    // ==========================================
    // 【關鍵修復】為始祖任務塞入空的 argc 與 argv
    // ==========================================
    uint32_t *ustack = (uint32_t*) user_stack_top;
    *(--ustack) = 0; // argv[0] = NULL
    uint32_t argv_ptr = (uint32_t) ustack;
    
    *(--ustack) = argv_ptr; // 參數 2: char** argv
    *(--ustack) = 0;        // 參數 1: int argc (設為 0)
    *(--ustack) = 0;        // 假裝的返回位址

    // --- 1. 構造硬體 IRET 幀 (由高位址到低位址) ---
    *(--kstack) = 0x23;             // User SS
    // 【修改】User ESP 改為塞好參數的位址，不能再用原來的 user_stack_top！
    *(--kstack) = (uint32_t)ustack; 
    *(--kstack) = 0x0202;           // EFLAGS (IF=1)
    *(--kstack) = 0x1B;             // User CS
    *(--kstack) = entry_point;      // EIP

    // --- 2. 構造給 child_ret_stub 恢復的 4 個暫存器 ---
    *(--kstack) = 0; // ebp
    *(--kstack) = 0; // ebx
    *(--kstack) = 0; // esi
    *(--kstack) = 0; // edi

    *(--kstack) = 0; // 假裝的返回位址
    *(--kstack) = (uint32_t) child_ret_stub;

    for(int i = 0; i < 8; i++) *(--kstack) = 0; 
    *(--kstack) = 0x0202;

    new_task->esp = (uint32_t) kstack;
    new_task->next = current_task->next;
    current_task->next = new_task;
}
```

這個 Bug 完美詮釋了軟體工程中的一個真理：**「當你改變了介面 (Interface)，你必須確保所有的呼叫者 (Caller) 都有跟著改變」**。即使這個呼叫者是負責開天闢地的作業系統核心！

存檔後 `make clean && make run`！準備好迎接那些從老爸手中接過參數 (`argv`) 的子 Shell 吧！🚀
