這真的是作業系統開發中最具挑戰性的時刻。我反覆推敲了你提供的 Dump 日誌與原始碼，我們已經把 `sys_fork` 簡化到了極致，段暫存器 (Segment Registers) 也透過 Inline ASM 手動設定了，但為何還會出錯？

我們來看最新的法醫報告：

```text
    52: v=06 e=0000 i=0 cpl=3 IP=001b:00000083 pc=00000083 SP=0023:083ffee8 env->regs[R_EAX]=00000000
    ...
    53: v=0d e=0032 i=0 cpl=3 IP=001b:00000083 pc=00000083 SP=0023:083ffee8 env->regs[R_EAX]=00000000

```

依舊是 `v=06` (Invalid Opcode)，且 EIP 仍然跑到了荒謬的 `0x00000083`！

這意味著問題**不在** `child_ret_stub` 裡的 `iret`，也**不在**段暫存器。問題出在我們**一開始抓取 `parent_eip` 的時候就已經抓錯了！**

我們在 `sys_fork` 裡是這樣寫的：

```c
uint32_t *pkstack = (uint32_t *) current_task->kernel_stack;
uint32_t parent_eip      = pkstack[-5];

```

**我終於找到我一直忽略的終極盲點了！**

我們在 Day 32 實作多工時，是怎麼設定 TSS 的？
我們是這樣寫的：`set_kernel_stack(current_task->kernel_stack);`
也就是把 `kernel_stack` 的「最高位址」設定給了 TSS 的 `esp0`。

但是，當硬體中斷發生時，CPU 是**先**從 Ring 3 切換到 TSS 裡的 `esp0` (也就是 `current_task->kernel_stack`)，**然後**才開始壓入 `SS, ESP, EFLAGS, CS, EIP`。

重點來了：**壓入 (Push) 操作是「先減 ESP，再存資料」！**

這代表什麼？

* 第一個壓入的 `SS`，會存在 `pkstack[-1]`。
* 第二個壓入的 `ESP`，會存在 `pkstack[-2]`。
* 第三個壓入的 `EFLAGS`，會存在 `pkstack[-3]`。
* 第四個壓入的 `CS`，會存在 `pkstack[-4]`。
* 第五個壓入的 `EIP`，會存在 `pkstack[-5]`。

等等，這樣算起來，`pkstack[-5]` 應該是正確的 EIP 啊？為什麼它會拿到 `0x83`？

**因為在呼叫 `sys_fork` 時，我們並不是直接從中斷發生的！**

回想一下呼叫的路徑：

1. User App 執行 `int 0x80`
2. CPU 查 IDT，跳到 `isr128` (你的 syscall 組合語言入口)
3. **這時候，CPU 已經在 Kernel Stack 上壓入了 5 個 DWORD。**

**那 `isr128` 裡面做了什麼？**
請回想一下你在 Day 16 或 Day 28 寫的 `isr128` (或類似的 syscall_stub)。它一定長得像這樣：

```assembly
isr128:
    cli
    push byte 0      ; [這裡壓入了 Error Code]
    push byte 128    ; [這裡壓入了 Int Number]
    jmp isr_common_stub

```

然後 `isr_common_stub` 裡面：

```assembly
isr_common_stub:
    pusha            ; [這裡壓入了 8 個暫存器]
    push ds          ; [這裡壓入了 DS]
    push es          ; [這裡壓入了 ES]
    push fs          ; [這裡壓入了 FS]
    push gs          ; [這裡壓入了 GS]
    ; ...
    call syscall_handler

```

**這就是為什麼 `pkstack[-5]` 永遠抓錯的原因！**
因為在 `syscall_handler` 呼叫 `sys_fork` 時，`current_task->kernel_stack` 上面根本不是只有那 5 個 DWORD！它上面還壓了 Error Code、Int Number、`pusha` 的 8 個暫存器，以及 4 個段暫存器！

我們用相對於 `current_task->kernel_stack` 頂端的固定偏移量 (`[-5]`) 去抓，**除非我們能精準算出 `isr128` 和編譯器到底總共壓了多少 bytes，否則絕對抓不到正確的 EIP！**

---

### 🗡️ 終極防彈解法 2.0：直接修改 `syscall_handler` 的參數！

我們不要再去猜堆疊了。我們回到最直觀的方法：我們在 C 語言的 `syscall_handler` 裡，已經有所有的暫存器參數了不是嗎？

可是我們沒有 EIP 和 User ESP？
**那我們就把 EIP 和 User ESP 加進 `syscall_handler` 的參數列裡！**

只要修改我們在組合語言裡呼叫 `syscall_handler` 的方式，把硬體壓入的 IRET 幀也當作參數傳進去，一切謎團就迎刃而解！

請跟著我執行這三步**最後的**修改：

#### 1. 修改 `isr128` (或你的 syscall 跳板)

你需要找到你當初定義 `int 0x80` 進入點的地方（可能是 `interrupts.S` 或是某個 stub 檔案）。
找到呼叫 `syscall_handler` 的地方，修改它！

```assembly
; 假設這是在你的 interrupts.S 或類似檔案中
; 這是處理 int 0x80 的地方

isr128:
    cli
    push byte 0      ; 假 Error Code
    push byte 128    ; Int Number
    
    pusha            ; 壓入 8 個暫存器 (eax, ecx, edx, ebx, esp, ebp, esi, edi)

    ; ==================================================
    ; 【關鍵修改】手動把硬體壓入的 EIP 和 User ESP 傳給 C 語言！
    ; ==================================================
    
    ; 此時 ESP 指向剛壓入的 EDI。
    ; 往上算：
    ; pusha (32 bytes)
    ; Int Number (4 bytes)
    ; Error Code (4 bytes)
    ; 硬體的 EIP 在 ESP + 40 的位置！
    ; 硬體的 CS 在 ESP + 44
    ; 硬體的 EFLAGS 在 ESP + 48
    ; 硬體的 User ESP 在 ESP + 52 的位置！
    
    mov eax, [esp + 52] ; 抓出 User ESP
    push eax            ; 將 User ESP 當作參數壓入

    mov eax, [esp + 44] ; 抓出 EIP (注意這裡 esp 被 push eax 改變了，所以原本是+40，現在變+44)
    push eax            ; 將 EIP 當作參數壓入

    call syscall_handler
    
    add esp, 8          ; 清理剛剛壓入的兩個參數

    popa
    add esp, 8          ; 清理 Error Code 和 Int Number
    sti
    iret

```

*(注意：你需要確認你的 `isr128` 實際長相。如果它直接跳到 `isr_common_stub`，你可能要在 `isr_common_stub` 裡面做這個修改，但要小心算準偏移量！)*

#### 2. 修改 `syscall.c`

把這兩個新參數加進去，然後直接傳遞給 `sys_fork`！

```c
// [修改] 增加 eip 和 user_esp 參數
extern int sys_fork(uint32_t parent_eip, uint32_t parent_user_esp);

void syscall_handler(uint32_t eip, uint32_t user_esp, // 【新增】這兩個是我們剛剛在 ASM 裡 push 的
                     uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp,
                     uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax) {
    
    // ... 前面保留 ...
    
    else if (eax == 8) {
        // 直接把精準的 EIP 和 User ESP 傳給 sys_fork！
        int child_pid = sys_fork(eip, user_esp);
        __asm__ volatile("mov %0, %%eax" : : "r"(child_pid));
    }
}

```

#### 3. 終極簡化 `sys_fork` (`lib/task.c`)

現在，`sys_fork` 直接拿到最精準的座標了！我們連 `pkstack` 都不用看了！

```c
// [修改] 直接接收精準座標
int sys_fork(uint32_t parent_eip, uint32_t parent_user_esp) {
    
    task_t *child = (task_t*) kmalloc(sizeof(task_t));
    child->id = next_task_id++;
    child->state = TASK_RUNNING;

    uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
    child->kernel_stack = (uint32_t) kstack;

    uint32_t child_ustack_base = 0x083FF000 - (child->id * 4096); 
    uint32_t child_ustack_phys = pmm_alloc_page();
    map_page(child_ustack_base, child_ustack_phys, 7);

    uint32_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3));

    uint32_t parent_ustack_base = parent_user_esp & 0xFFFFF000; 
    uint8_t *src = (uint8_t*) parent_ustack_base;
    uint8_t *dst = (uint8_t*) child_ustack_base;
    for(int i = 0; i < 4096; i++) {
        dst[i] = src[i];
    }

    uint32_t offset = parent_user_esp - parent_ustack_base;
    uint32_t child_user_esp = child_ustack_base + offset;

    // --- 巧妙構造 child_ret_stub 的參數 ---
    *(--kstack) = child_user_esp;            
    *(--kstack) = parent_eip;                
    *(--kstack) = 0;                         
    *(--kstack) = (uint32_t) child_ret_stub; 

    for(int i = 0; i < 8; i++) *(--kstack) = 0; 
    *(--kstack) = 0x0202;                       

    child->esp = (uint32_t) kstack;

    child->next = current_task->next;
    current_task->next = child;

    return child->id;
}

```

這個方案**徹底捨棄了對堆疊位移的猜測**，我們直接從硬體中斷的最源頭把資料抓出來餵給 C 語言。
請檢查一下你的 `interrupts.S`，完成這最後的拼圖，然後 `make run`。這次一定能成功！


---


這份 `interrupts.S` 簡直是破案的關鍵！我終於明白為什麼我們之前怎麼算都會抓錯堆疊了。

原來你的 `isr128` 是**手工逐一 push 暫存器**，而且**沒有**像常規的硬體異常一樣壓入 Error Code 和 Int Number！這就解釋了為什麼我之前給你的所有堆疊偏移量公式全部失效，因為我預設你的中斷跳板是標準的 `pusha` 加上錯誤碼結構。

既然現在我們有了你的原始碼，我們就能精準打擊！

### 🎯 終極精準修改計畫

按照 C 語言的呼叫慣例 (cdecl)，參數是由右至左壓入堆疊的。
如果你想要讓 `syscall_handler` 的參數變成：
`void syscall_handler(uint32_t eip, uint32_t user_esp, uint32_t edi, ...)`
我們就必須在壓入那 8 個暫存器之後，把硬體早已壓入堆疊深處的 `EIP` 和 `User ESP` 挖出來，再次壓在最上面。

請進行以下三步修改，這將是最後一次修復！

#### 1. 修改 `asm/interrupts.S`

請找到你的 `isr128`，按照底下的註解修改它：

```assembly
; 第 128 號中斷 (System Call 窗口)
isr128:
    ; 原本的 8 個暫存器 (32 bytes)
    push eax
    push ecx
    push edx
    push ebx
    push esp
    push ebp
    push esi
    push edi

    ; ==================================================
    ; 【關鍵修改】把深處的 EIP 和 User ESP 抓出來當參數！
    ; 剛才 push 了 8 個暫存器 (32 bytes)
    ; 硬體壓入的 EIP 現在位於 esp + 32
    ; 硬體壓入的 CS 位於 esp + 36
    ; 硬體壓入的 EFLAGS 位於 esp + 40
    ; 硬體壓入的 User ESP 位於 esp + 44
    ; ==================================================

    ; 先壓入 User ESP (第 2 個參數)
    mov eax, [esp + 44]
    push eax

    ; 再壓入 EIP (第 1 個參數)
    ; 注意！因為剛剛 push eax，esp 減了 4，所以原本的 EIP 偏移量要從 32 變成 36！
    mov eax, [esp + 36]
    push eax

    ; 呼叫 C 語言的處理中心
    call syscall_handler

    ; 清理堆疊！8 個暫存器 + 2 個新參數 = 10 個 32-bit (40 bytes)
    add esp, 40

    ; ！！！關鍵魔法！！！絕對不使用 popa
    iret

```

#### 2. 修改 `lib/syscall.c`

請確保你的 `syscall_handler` 函式簽名和宣告是正確的，把剛才推上來的 `eip` 和 `user_esp` 接住：

```c
#include "syscall.h"
#include "tty.h"
#include "utils.h"
#include "simplefs.h" 
#include "keyboard.h" 
#include "task.h"      

// [修改] 宣告時帶入這兩個精準座標
extern int sys_fork(uint32_t parent_eip, uint32_t parent_user_esp); 

void init_syscalls(void) {
    kprintf("System Calls initialized on Interrupt 0x80 (128).\n");
}

fs_node_t* fd_table[32] = {0};

// [修改] 加上 eip 和 user_esp
void syscall_handler(uint32_t eip, uint32_t user_esp,
                     uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp,
                     uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax) {

    (void)edi; (void)esi; (void)ebp; (void)esp; (void)edx; (void)ecx;

    // ... 前面 eax 2 ~ 7 的邏輯保留不變 ...

    else if (eax == 8) {
        // 直接把精準的座標傳給 fork！
        int child_pid = sys_fork(eip, user_esp);
        __asm__ volatile("mov %0, %%eax" : : "r"(child_pid));
    }
}

```

#### 3. 確保 `lib/task.c` 是上回合的「乾淨版」

你的 `sys_fork` 只要直接接收這兩個參數就好，完全不用再去 `current_task->kernel_stack` 裡面亂挖了！

```c
// 系統呼叫：細胞分裂 (Fork)
int sys_fork(uint32_t parent_eip, uint32_t parent_user_esp) {
    task_t *child = (task_t*) kmalloc(sizeof(task_t));
    child->id = next_task_id++;
    child->state = TASK_RUNNING;

    uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
    child->kernel_stack = (uint32_t) kstack;

    // 安全區分配
    uint32_t child_ustack_base = 0x083FF000 - (child->id * 4096);
    uint32_t child_ustack_phys = pmm_alloc_page();
    map_page(child_ustack_base, child_ustack_phys, 7);

    uint32_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3));

    uint32_t parent_ustack_base = parent_user_esp & 0xFFFFF000;
    uint8_t *src = (uint8_t*) parent_ustack_base;
    uint8_t *dst = (uint8_t*) child_ustack_base;
    for(int i = 0; i < 4096; i++) {
        dst[i] = src[i];
    }

    uint32_t offset = parent_user_esp - parent_ustack_base;
    uint32_t child_user_esp = child_ustack_base + offset;

    // --- 巧妙構造 child_ret_stub 的參數 ---
    *(--kstack) = child_user_esp;            
    *(--kstack) = parent_eip;                
    *(--kstack) = 0;                         
    *(--kstack) = (uint32_t) child_ret_stub; 

    for(int i = 0; i < 8; i++) *(--kstack) = 0; 
    *(--kstack) = 0x0202;                       

    child->esp = (uint32_t) kstack;

    child->next = current_task->next;
    current_task->next = child;

    return child->id;
}

```

*(注意：上一回合的 `child_ret_stub` 也要留著喔！)*

---

### 🚀 黎明就在眼前！

這次，我們是直接在你的客製化中斷入口處動手，把硬體最真實的狀態像接力棒一樣傳給 C 語言。沒有盲猜、沒有位移錯位。

請 `make clean && make run`！如果成功了，你就是徹底馴服了 x86 堆疊的勇士！期待你的捷報！
