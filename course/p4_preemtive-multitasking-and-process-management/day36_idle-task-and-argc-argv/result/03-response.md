哈哈，Rick！看來我們還是遇到了一點小阻礙。
這次的 Dump 顯示：
```text
3: v=0e e=0004 i=0 cpl=3 IP=001b:080481a9 pc=080481a9 SP=0023:083ffecc CR2=08400004
```
又是 **Page Fault (Exception 0x0E)**，而且 `CR2` 一樣是 `0x08400004`！
這代表，即使我們塞入了空參數，初始的 Shell 在啟動時，它的 C 語言啟動碼 (crt0) 或是編譯器自動產生的 prologue，仍然在試圖存取一個越界的位址。

### 🔍 案發現場解密：被遺忘的 C 語言呼叫慣例 (cdecl)

回想一下我們在 `create_user_task` 中是怎麼構造假參數的：
```c
    uint32_t *ustack = (uint32_t*) user_stack_top;
    *(--ustack) = 0; // argv[0] = NULL
    uint32_t argv_ptr = (uint32_t) ustack;
    
    *(--ustack) = argv_ptr; // 參數 2: char** argv
    *(--ustack) = 0;        // 參數 1: int argc (設為 0)
    *(--ustack) = 0;        // 假裝的返回位址
```

我們是直接從 `user_stack_top` (`0x08400000`) 往下減 4。
這在堆疊上看起來像是這樣：
* `0x083FFFFC`: 0 (argv[0]) -> `argv_ptr`
* `0x083FFFF8`: argv_ptr
* `0x083FFFF4`: 0 (argc)
* `0x083FFFF0`: 0 (返回位址)

看起來很完美，對吧？`ESP` 最後停留在 `0x083FFFF0`。
當 `_start` 執行時，它會執行 `push ebp; mov ebp, esp`。
此時 `EBP` 是 `0x083FFEEC`。
當 `_start` 要拿 `argc` 參數時，它會讀取 `[ebp + 8]`，也就是 `0x083FFFF4`。這在我們的合法記憶體範圍內。

**但是！**
請注意 Dump 裡面的 `ESP` 是多少：
`SP=0023:083ffecc`

這代表在引發 Page Fault 的那一刻，`ESP` 已經往下掉了好幾十個 Bytes。
而且 `_start` 在某個地方（可能是準備呼叫其他函式，或者是存取陣列時），竟然去存取了 `CR2=08400004`！這已經超出 `user_stack_top` 了！

這是為什麼呢？
原因在於 C 語言在執行函式 (尤其是 `main` 或 `_start`) 的最初，除了把參數放進堆疊，它還有一個**「對齊要求」**。為了效能，許多編譯器會假設堆疊頂部（在壓入返回位址之前）是 **16 bytes 對齊** 的。如果我們手動構造的堆疊沒有對齊，後續的程式碼可能會用奇怪的位移去存取變數，導致越界。

另一個可能的原因，是你的 `app.c` 在編譯時，預設帶有某些我們不需要的 C 語言標準庫啟動碼 (CRT) 假設，導致它在進入 `_start` 前，試圖去上層找某些環境變數。

### 🗡️ 終極修復 6.0：讓 User Stack 留有餘地 (Breathing Room)

為了解決這種不可預期的「邊界越位」問題，作業系統開發中有一個最簡單暴力的心法：**永遠不要把 User Stack 貼著記憶體分頁的絕對邊緣！**

我們只要在最初分配 User Stack 時，把起點**往下壓一點點**（例如留個 16 bytes 或 64 bytes 的空白緩衝區），這樣不管 C 語言怎麼亂跳、怎麼對齊，都不會踩到隔壁尚未分配的分頁（`0x08400000`）！

請打開 **`lib/task.c`**，並對 `create_user_task` 與 `sys_exec` 進行微調：

#### 1. `create_user_task` 的留白術
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
    // 【關鍵修復】留給 User Stack 一點呼吸空間，不要緊貼 0x08400000！
    // ==========================================
    uint32_t *ustack = (uint32_t*) (user_stack_top - 64); // 往下壓 64 bytes
    
    *(--ustack) = 0; 
    uint32_t argv_ptr = (uint32_t) ustack;
    *(--ustack) = argv_ptr; 
    *(--ustack) = 0;        
    *(--ustack) = 0;        

    *(--kstack) = 0x23;             
    *(--kstack) = (uint32_t)ustack; 
    *(--kstack) = 0x0202;           
    *(--kstack) = 0x1B;             
    *(--kstack) = entry_point;      

    *(--kstack) = 0; 
    *(--kstack) = 0; 
    *(--kstack) = 0; 
    *(--kstack) = 0; 

    *(--kstack) = (uint32_t) child_ret_stub;

    for(int i = 0; i < 8; i++) *(--kstack) = 0; 
    *(--kstack) = 0x0202;

    new_task->esp = (uint32_t) kstack;
    new_task->next = current_task->next;
    current_task->next = new_task;
}
```

#### 2. `sys_exec` 也要留白
如果 `create_user_task` 留白了，`sys_exec` 變身時也應該比照辦理，以防萬一。

```c
int sys_exec(registers_t *regs) {
    char* filename = (char*)regs->ebx;
    char** argv = (char**)regs->ecx;

    fs_node_t* file_node = simplefs_find(filename);
    if (file_node == 0) { return -1; }

    uint8_t* buffer = (uint8_t*) kmalloc(file_node->length);
    vfs_read(file_node, 0, file_node->length, buffer);
    uint32_t entry_point = elf_load((elf32_ehdr_t*)buffer);
    kfree(buffer);

    if (entry_point == 0) return -1;

    // 【關鍵修復】留給 User Stack 一點呼吸空間
    uint32_t clean_user_stack_top = 0x083FF000 - (current_task->id * 4096) + 4096;
    uint32_t stack_ptr = clean_user_stack_top - 64; 

    int argc = 0;
    if (argv != 0 && (uint32_t)argv > 0x08000000) {
        while (argv[argc] != 0) argc++;
    }

    uint32_t argv_ptrs[16] = {0};

    if (argc > 0) {
        for (int i = argc - 1; i >= 0; i--) {
            int len = strlen(argv[i]) + 1;
            stack_ptr -= len;
            memcpy((void*)stack_ptr, argv[i], len);
            argv_ptrs[i] = stack_ptr;
        }

        stack_ptr = stack_ptr & ~3;

        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = 0;
        for (int i = argc - 1; i >= 0; i--) {
            stack_ptr -= 4;
            *(uint32_t*)stack_ptr = argv_ptrs[i];
        }
        uint32_t argv_base = stack_ptr;

        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = argv_base;
        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = argc;
    } else {
        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = 0; 
        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = 0; 
    }

    stack_ptr -= 4;
    *(uint32_t*)stack_ptr = 0;

    regs->eip = entry_point;
    regs->user_esp = stack_ptr;

    return 0;
}
```

這個看似微不足道的「退一步」(-64 bytes)，往往能拯救無數因為編譯器對齊或是隱藏變數造成的越界存取。

請重新 `make clean && make run`，讓我們看看這次能否成功騙過 C 語言編譯器！
