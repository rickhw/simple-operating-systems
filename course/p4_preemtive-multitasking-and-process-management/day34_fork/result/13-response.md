這個 Bug 的狡猾程度已經超越了一般的作業系統開發範疇。我們把所有堆疊對齊、段暫存器、呼叫慣例都算得清清楚楚，卻依然在最後一刻摔了一跤。

讓我們再看一次這個彷彿被詛咒的法醫報告：
`49: v=0e e=0006 i=0 cpl=3 IP=001b:0804800d pc=0804800d SP=0023:083fded4 CR2=fffffffc`

這一次，錯誤不是 GPF (Exception 0x0D) 也不是 Invalid Opcode (Exception 0x06)，而是 **Page Fault (Exception 0x0E)**！

* `cpl=3`: 分身成功降落回 User Space 了！
* `IP=0804800d`: 分身正在執行程式碼！
* `SP=083fded4`: 分身正在使用它的 User Stack！
* `CR2=fffffffc`: **分身試圖存取位址 `0xFFFFFFFC`！**

**等一下，`0xFFFFFFFC` 是什麼鬼位址？**
這可是位於 4GB 記憶體的頂端啊！我們的 User Stack 明明是分配在 `0x083FF000` 附近，怎麼會跑去存取 `0xFFFFFFFC`？

### 🔍 案發現場解密：藏在 `cdecl` 裡的惡魔

這一切的罪魁禍首，在於我們在 `app.c` 裡是如何封裝 `sys_fork` 的：

```c
int sys_fork() {
    int pid;
    __asm__ volatile ("int $0x80" : "=a"(pid) : "a"(8) : "memory");
    return pid;
}

```

這段程式碼看起來很無害，對吧？
但別忘了，這是一個 **C 語言函式**。在編譯成機器碼後，它的開頭和結尾會長這樣：

```assembly
sys_fork:
    push ebp        ; 保存老爸的 Base Pointer
    mov ebp, esp    ; 建立新的 Stack Frame
    ; ... int 0x80 ...
    mov esp, ebp    ; 恢復 Stack
    pop ebp         ; 彈出老爸的 Base Pointer
    ret

```

當老爸 (Parent) 執行 `int 0x80` 時，它的 `ebp` 指向哪裡？指向老爸自己的 User Stack 裡面！
我們在 `task.c` 的 `sys_fork` 裡，完美地把老爸的 User Stack **原封不動**拷貝給了小孩：

```c
    for(int i = 0; i < 4096; i++) { dst[i] = src[i]; }

```

所以，小孩的 User Stack 裡，存放的 `ebp` **依然是老爸的絕對位址！**

當小孩從 `child_ret_stub` 醒來，成功降落到 `sys_fork` 的下半段時，它會執行 `pop ebp`。這時候，小孩的 `ebp` 拿到了**老爸的位址**。
接著，小孩從 `sys_fork` `ret` 回到 `_start`，準備執行接下來的 `sys_print`。
在存取區域變數或準備參數時，C 語言會用到 `ebp`。因為這個 `ebp` 是錯的（可能指向一個對於小孩來說根本沒有 map 到的位址，甚至是負數 `0xFFFFFFFC`），所以立刻觸發了 Page Fault！

這就是為什麼 `fork` 被稱為作業系統最難的 Syscall。你不仅要複製記憶體，你還必須**修補所有指向記憶體內部的絕對指標！**
而 `ebp` (Base Pointer) 就是那個最要命的絕對指標！

---

### 🗡️ 終極修復 3.0：手動修補 `ebp` 指標！

我們不能指望 C 語言自己解決這個問題。我們必須在核心裡，幫剛出生的小孩做一次「外科手術」，把它的 `EBP` 暫存器（以及可能存在 Stack 裡面的舊 EBP 值），全部修正為相對於小孩 Stack 的新位址！

請跟著我執行這兩個**非常精準的外科研刀**：

#### 1. 抓出硬體的 `ebp` (`asm/interrupts.S`)

為了精準修補 `ebp`，我們需要把硬體壓入的 `ebp` 也傳給 C 語言。
(雖然 `pusha` 裡有 `ebp`，但我們把它當作參數傳進去會更清晰)。

請修改 **`asm/interrupts.S`**：

```assembly
isr128:
    push eax
    push ecx
    push edx
    push ebx
    push esp
    push ebp  ; [我們需要這個 ebp！]
    push esi
    push edi

    ; [修改] 傳入三個參數：EBP, User ESP, EIP
    mov eax, [esp + 44] ; User ESP
    push eax
    mov eax, [esp + 36] ; EIP
    push eax
    
    ; 【新增】把 pusha 壓入的 ebp (在 esp+28) 抓出來當參數！
    ; 注意現在 esp 又往下掉了 8 bytes，所以原本的 +20 變成 +28
    mov eax, [esp + 28] 
    push eax

    call syscall_handler

    ; 清理 3 個參數 = 12 bytes。 32 + 12 = 44 bytes
    add esp, 44 

    iret

```

#### 2. 外科手術級的 `sys_fork` (`lib/task.c`)

請打開 **`lib/task.c`**。我們要在這裡計算老爸與小孩 Stack 之間的「位移量 (Delta)」，然後把小孩的 `EBP` 加上這個位移量！

```c
// [修改] 接收三個精準座標
int sys_fork(uint32_t parent_ebp, uint32_t parent_eip, uint32_t parent_user_esp) {
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

    // 計算偏移量 (Delta)
    int32_t delta = child_ustack_base - parent_ustack_base;

    uint32_t child_user_esp = parent_user_esp + delta;

    // ==========================================
    // 【終極修補】修正 EBP 指標
    // ==========================================
    uint32_t child_ebp = parent_ebp;
    
    // 如果老爸的 ebp 是落在它的 User Stack 範圍內，我們就幫它修正！
    if (parent_ebp >= parent_ustack_base && parent_ebp < parent_ustack_base + 4096) {
        child_ebp = parent_ebp + delta;
    }

    // --- 巧妙構造 child_ret_stub 的參數 ---
    *(--kstack) = child_user_esp;
    *(--kstack) = parent_eip;
    *(--kstack) = 0;
    *(--kstack) = (uint32_t) child_ret_stub;

    // --- 偽造 switch_task 的 8 個暫存器 (popa) ---
    // [修改] 這裡必須把修正過後的 ebp 塞給 switch_task 的 popa！
    // 順序是: EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX
    *(--kstack) = child_ebp; // 【這裡填入修正後的 EBP！】
    *(--kstack) = 0; // EDI
    *(--kstack) = 0; // ESI
    *(--kstack) = 0; // EDX
    *(--kstack) = 0; // ECX
    *(--kstack) = 0; // EBX
    *(--kstack) = 0; // EAX (初始為 0 也可以，反正 C 語言會拿 child_ret_stub 裡的 0)
    *(--kstack) = 0; // 給 ESP 補位

    *(--kstack) = 0x0202; // popf

    child->esp = (uint32_t) kstack;
    child->next = current_task->next;
    current_task->next = child;

    return child->id;
}

```

#### 3. 更新 Syscall 路由 (`lib/syscall.c`)

把第三個參數接好。

```c
// [修改]
extern int sys_fork(uint32_t parent_ebp, uint32_t parent_eip, uint32_t parent_user_esp);

void syscall_handler(uint32_t ebp_param, uint32_t eip, uint32_t user_esp, // <--- 新增 ebp
                     uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp,
                     uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax) {
    // ...
    else if (eax == 8) {
        int child_pid = sys_fork(ebp_param, eip, user_esp);
        __asm__ volatile("mov %0, %%eax" : : "r"(child_pid));
    }
}

```

這次的 Bug，讓我們見識到了 C 語言編譯器在背後做的小動作。修正了 EBP，小孩的分身就再也不會迷失在老爸的記憶體空間裡了。

來吧，再一次 `make clean && make run`！如果這次成功了，你就是真正掌握生死輪迴的大師了！
