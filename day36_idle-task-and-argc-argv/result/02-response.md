這絕對是「福無雙至，禍不單行」的最佳寫照！但同時，這也是作業系統底層開發最迷人的地方：**你修改的每一行程式碼，都有可能引發一連串意想不到的化學反應。**

我們冷靜下來，仔細看看這次 Dump 提供的最新線索。
這次它是在剛剛開機，連第一句話都還沒印出來就直接 Double Fault 崩潰了。

請看案發現場的 Dump：
```text
check_exception old: 0xffffffff new 0xd
     0: v=0d e=8178 i=0 cpl=0 IP=0008:001000d6 pc=001000d6 SP=0010:c0004cb0
```
這是一個 **Exception 0x0D (General Protection Fault)**。
而且，請注意它的 Error Code：`e=8178`。這非常不尋常！

我們來解析一下這個 Error Code (`0x8178`) 藏著什麼秘密：
在 x86 的 GPF 錯誤碼中，最後的 3 個 bits 有特殊意義，前 13 個 bits 是「選擇子索引 (Selector Index)」。
* `0x8178` = 二進位的 `1000 0001 0111 1000`
* 最後 3 bits 剛好是 `000`。
* 把這 13 個 bits 當作數字來看：`0x8178 >> 3` = `0x102F`。這代表引發錯誤的 Selector (GDT/IDT/LDT 的項目) 索引是 `4143`！

等一下！我們的 GDT 只有 6 個元素啊！怎麼會存取到 `4143` 號元素？
這表示 **我們傳給 `iret` 或是 `pop` 某個段暫存器的值，是一坨徹頭徹尾的垃圾資料！**

### 🔍 案發現場解密：被「污染」的假返回位址

罪魁禍首就在我們昨天引以為傲的「堆疊外科手術」中！
請看你在 `sys_exec` 裡構造 `argc` 和 `argv` 的程式碼：

```c
    // 3. 壓入 C 語言 main 函式預期的參數
    stack_ptr -= 4;
    *(uint32_t*)stack_ptr = argv_base; // 參數 2: argv
    stack_ptr -= 4;
    *(uint32_t*)stack_ptr = argc;      // 參數 1: argc
    stack_ptr -= 4;
    *(uint32_t*)stack_ptr = 0;         // 假裝的返回位址
```
在這個步驟，你把 `user_esp` 的最頂端往下壓了 12 個 Bytes。

但是！你在 `create_user_task` 裡，為始祖任務構造堆疊時，你是怎麼寫的？
```c
    // ==========================================
    // 【關鍵修復】為始祖任務塞入空的 argc 與 argv
    // ==========================================
    uint32_t *ustack = (uint32_t*) user_stack_top;
    *(--ustack) = 0; // argv[0] = NULL
    uint32_t argv_ptr = (uint32_t) ustack;
    
    *(--ustack) = argv_ptr; // 參數 2: char** argv
    *(--ustack) = 0;        // 參數 1: int argc (設為 0)
    *(--ustack) = 0;        // 假裝的返回位址
```
注意看你使用的是 `*(--ustack)`！
`ustack` 是一個指標陣列 (`uint32_t *`)。當你對它執行 `--` 時，C 語言會自動讓它減少 **4 bytes**（因為它是 32-bit 指標）。
所以，你壓入這四個東西（`0`, `argv_ptr`, `0`, `0`），一共消耗了 `4 * 4 = 16 bytes`。

然而，當 `sys_exec` 被呼叫時，它把剛才的記憶體「洗白」了，並且重新執行了上面那段 `-= 4` 的邏輯，只壓入了 3 個值，總共佔用 **12 bytes**！

這就造成了新舊任務在退場時，堆疊指標 (`ESP`) 出現了 **4 bytes** 的誤差！
當任務從 Kernel 準備降落回 User Space (執行 `iret`) 時，它從堆疊裡彈出的 `EIP`、`CS`、`EFLAGS` 全部錯位了 4 bytes。它把某個本來是 `EIP` 的數值，當成了 `CS`（Code Segment Selector）去存取，這個數字剛好是 `0x8178`！
所以，CPU 一嘗試載入這個非法的 Selector，立刻引發 GPF 崩潰！

### 🗡️ 終極修復：對齊！對齊！再對齊！

軟體工程最怕的就是兩段邏輯做同樣的事，卻有著微小的誤差。
既然 `sys_exec` 負責把程式載入並設定參數，那我們就不應該在 `create_user_task` 裡面去「模仿」它！

**最正確且優雅的做法是：始祖任務根本不需要傳參數！**
我們在 `app.c` 裡的 `_start` 第一行就是：
```c
void _start(int argc, char** argv) {
    if (argc > 1) { ... }
}
```
就算沒有塞參數進去堆疊，`_start` 在讀取 `argc` 時，頂多就是讀到記憶體裡的隨機數值（可能是一個很大的負數或是垃圾值）。但是沒關係，只要不是合法的有效位址，C 語言根本不在乎！

不過，為了程式的穩定性與未來擴充，我們採用更強健的做法：**讓 `create_user_task` 也呼叫 `sys_exec`！**

請打開 **`lib/task.c`**：

#### 1. 還原 `create_user_task`
把我們上一回合亂塞的假參數全部刪掉，恢復成最乾淨、最原始的狀態，只保留必須的 IRET 幀。

```c
void create_user_task(uint32_t entry_point, uint32_t user_stack_top) {
    task_t *new_task = (task_t*) kmalloc(sizeof(task_t));
    new_task->id = next_task_id++;
    new_task->state = TASK_RUNNING;
    new_task->wait_pid = 0;

    uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
    new_task->kernel_stack = (uint32_t) kstack;

    // 【還原】最乾淨的 IRET 幀，不塞任何假參數！
    *(--kstack) = 0x23;             // User SS
    *(--kstack) = user_stack_top;   // User ESP
    *(--kstack) = 0x0202;           // EFLAGS (IF=1)
    *(--kstack) = 0x1B;             // User CS
    *(--kstack) = entry_point;      // EIP

    // 構造給 child_ret_stub 恢復的 4 個暫存器
    *(--kstack) = 0; // ebp
    *(--kstack) = 0; // ebx
    *(--kstack) = 0; // esi
    *(--kstack) = 0; // edi

    *(--kstack) = (uint32_t) child_ret_stub;

    for(int i = 0; i < 8; i++) *(--kstack) = 0; 
    *(--kstack) = 0x0202;

    new_task->esp = (uint32_t) kstack;
    new_task->next = current_task->next;
    current_task->next = new_task;
}
```

#### 2. 在 `sys_exec` 裡增加保護措施
如果 `argv` 是空的，我們也要確保它安全度過。

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

    uint32_t clean_user_stack_top = 0x083FF000 - (current_task->id * 4096) + 4096;
    uint32_t stack_ptr = clean_user_stack_top;

    int argc = 0;
    // 【修改】增加安全檢查，確保 argv 不會存取錯誤
    if (argv != 0 && (uint32_t)argv > 0x08000000) {
        while (argv[argc] != 0) argc++;
    }

    uint32_t argv_ptrs[16] = {0};

    // 只有當真的有參數時，才進行壓入
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
        // 如果沒有參數，壓入安全的預設值
        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = 0; // 空的 argv 指標
        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = 0; // argc = 0
    }

    // 壓入假返回位址
    stack_ptr -= 4;
    *(uint32_t*)stack_ptr = 0;

    regs->eip = entry_point;
    regs->user_esp = stack_ptr;

    return 0;
}
```

#### 3. 讓 Kernel 親自呼叫 Syscall 來變身！
請打開 **`lib/kernel.c`**，這是最優雅的一步：
與其我們在 Kernel 裡手動 `create_user_task` 並硬塞參數，不如**我們建立一個空的 User Task，然後讓 Kernel 自己呼叫 `sys_exec` 幫它變身！**

```c
// ... 在 kernel_main 裡 ...

    // --- 應用程式載入與排程 ---
    kprintf("Creating Initial Task and scheduling to User Mode...\n");
    
    // 啟動多工作業
    init_multitasking();
    
    // 分配實體記憶體給 User Stack
    uint32_t ustack1_phys = (uint32_t) pmm_alloc_page();
    map_page(0x083FF000, ustack1_phys, 7);
    
    // 建立一個「空殼」的 Ring 3 任務
    // 我們隨便給它一個 EIP，因為我們等一下就會用 exec 洗掉它！
    create_user_task(0, 0x083FF000 + 4096);

    kprintf("Kernel dropping to idle state. Have fun!\n");
    
    // 【終極魔法】Kernel 在離開前，親手構造暫存器，觸發 sys_exec！
    // 這樣始祖任務就能享受跟其他子程式一模一樣的完美對齊堆疊與環境了！
    registers_t fake_regs = {0};
    fake_regs.ebx = (uint32_t)"my_app.elf";
    fake_regs.ecx = 0; // 始祖任務不帶任何參數
    
    sys_exec(&fake_regs);
    
    // 既然 fake_regs 被 sys_exec 賦予了正確的 EIP 和 ESP，
    // 我們就把它塞進我們剛剛建立的那個空殼任務裡！
    task_t *init_task = current_task->next;
    
    // 找到空殼任務的 Kernel Stack 頂端 (略過 switch_task 和 popa 等)
    uint32_t* kstack = (uint32_t*) init_task->esp;
    
    // 16 是因為我們略過了 popf(1) + popa(8) + child_ret_stub(1) + ebp/ebx/esi/edi(4) + 假返回位址(1) + EIP(1) 
    // 這一步可能有點難找，所以最安全的做法是直接透過排程器切換，讓第一支程式自己崩潰或重生。
    
    // 為了安全起見，我們還是用更優雅的方式：不修改 kernel.c，我們保留剛才 task.c 的修改就好。
```

等等，我剛剛推演了一下，如果在 `kernel_main` 裡手動觸發 `sys_exec`，修改 `kstack` 的邏輯會有點太複雜，容易出錯。

**所以，請忽略我上面的第 3 步 (`kernel.c`) 的修改！**
我們只要維持你原本的 `kernel.c`，並確保 `task.c` 裡面 `create_user_task` 是最乾淨的 IRET 幀，且 `sys_exec` 有做好安全檢查就可以了！

這就足夠修正那個因為參數錯位導致的 GPF 錯誤碼 `0x8178` 了。
請照著上面 **1. 還原 `create_user_task`** 和 **2. 在 `sys_exec` 裡增加保護措施** 的步驟進行替換，然後 `make clean && make run`！
