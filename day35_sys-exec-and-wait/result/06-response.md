哈，Rick！深呼吸！這就是開發作業系統最刺激的地方，每一次當你以為已經搞定一切的時候，編譯器和暫存器總會聯手給你一個「驚喜」。

我仔細看了你的原始碼和 Dump，發現問題出在哪裡了！
這是一個**「舊時代的殘影」與「新架構」的致命衝突**。

在上一回合，我們已經決定把 `child_ret_stub` 改成純組合語言（使用 `iret` 降落），**不再使用 C 語言的參數傳遞（cdecl）**。
但是，在你的 `task.c` 裡面，`create_user_task` 和 `sys_fork` 卻**依然保留了壓入 C 語言參數（如 `target_esp`, `target_eip`）的舊寫法**！

這導致了嚴重的「堆疊錯位」：
1. `switch_task` 執行 `ret` 跳轉到 `child_ret_stub`。
2. 組合語言裡的 `child_ret_stub` 預期堆疊上躺著的是 `[EDI, ESI, EBX, EBP, EIP, CS, EFLAGS, ESP, SS]` 這套完美的 IRET 幀。
3. 結果你的 `task.c` 給它準備的卻是 `[target_eip, target_esp, ebp, ebx, esi, edi, 假返回位址]`。
4. `iret` 彈出了完全錯誤的位址當作指令指標 (EIP)，第一支初始 Shell 根本還沒印出任何字，就當場 Page Fault 死機了！

沒關係，我們這次直接把架構精簡到極致。**不用假返回位址，也不用 C 語言參數！**

請**徹底覆蓋**以下這兩個檔案，這是最完美的堆疊對齊版本：

### 1. `asm/switch_task.S` (徹底極簡化)
把 `add esp, 4` 這個多餘的動作拿掉，讓它直接對接暫存器與 IRET 幀。

```assembly
[bits 32]
global switch_task
global child_ret_stub

switch_task:
    pusha
    pushf
    mov eax, [esp + 40]
    mov [eax], esp
    mov eax, [esp + 44]
    mov esp, [eax]
    popf
    popa
    ret

; ==========================================
; 【終極降落傘】純組合語言實作，保證絕不弄髒暫存器！
; ==========================================
child_ret_stub:
    ; 剛從 switch_task 的 ret 跳過來。
    ; 此時 ESP 完美指向我們在 C 語言壓入的 edi！
    
    ; 1. 精準彈出我們手工對齊的 4 個暫存器
    pop edi
    pop esi
    pop ebx
    pop ebp       ; 這裡拿回了完美修正過的 EBP！

    ; 2. 換上平民服裝 (User Data Segment = 0x23)
    mov cx, 0x23
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx

    mov eax, 0    ; 【核心魔法】子行程拿到回傳值 0！

    ; 3. 完美降落！
    ; 此刻 ESP 剛好指著我們手工準備的 IRET 幀 (EIP, CS, EFLAGS, ESP, SS)
    iret
```

### 2. `lib/task.c` (真正的硬體堆疊構造)
把舊的參數壓入邏輯清空，完全配合上面的 `child_ret_stub` 順序！

```c
#include <stdint.h>
#include "task.h"
#include "pmm.h"
#include "kheap.h"
#include "utils.h"
#include "tty.h"
#include "gdt.h"
#include "paging.h"
#include "simplefs.h"
#include "vfs.h"
#include "elf.h"

volatile task_t *current_task = 0;
volatile task_t *ready_queue = 0;
uint32_t next_task_id = 0;

extern void switch_task(uint32_t *current_esp, uint32_t *next_esp);
extern void child_ret_stub(); // 純組合語言跳板

void init_multitasking() {
    kprintf("[Task] Initializing Multitasking...\n");
    task_t *main_task = (task_t*) kmalloc(sizeof(task_t));
    main_task->id = next_task_id++;
    main_task->esp = 0;
    main_task->kernel_stack = 0;
    main_task->state = TASK_RUNNING;
    main_task->wait_pid = 0;

    main_task->next = main_task;
    current_task = main_task;
    ready_queue = main_task;
}

void exit_task() {
    task_t *temp = current_task->next;
    while (temp != current_task) {
        if (temp->state == TASK_WAITING && temp->wait_pid == current_task->id) {
            temp->state = TASK_RUNNING;
            temp->wait_pid = 0;
        }
        temp = temp->next;
    }
    current_task->state = TASK_DEAD;
    schedule();
}

void schedule() {
    if (!current_task) return;

    task_t *curr = (task_t*)current_task;
    task_t *next_node = curr->next;
    while (next_node != curr) {
        if (next_node->state == TASK_DEAD) {
            curr->next = next_node->next;
            next_node = curr->next;
        } else {
            curr = next_node;
            next_node = curr->next;
        }
    }

    task_t *next_run = current_task->next;
    while (next_run->state != TASK_RUNNING && next_run != current_task) {
        next_run = next_run->next;
    }

    if (next_run->state != TASK_RUNNING) {
        kprintf("\n[Kernel] No runnable tasks. CPU Idle.\n");
        while(1) { __asm__ volatile("sti; hlt"); }
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

void create_user_task(uint32_t entry_point, uint32_t user_stack_top) {
    task_t *new_task = (task_t*) kmalloc(sizeof(task_t));
    new_task->id = next_task_id++;
    new_task->state = TASK_RUNNING;
    new_task->wait_pid = 0;

    uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
    new_task->kernel_stack = (uint32_t) kstack;

    // --- 1. 構造硬體 IRET 幀 (由高位址到低位址) ---
    *(--kstack) = 0x23;             // User SS
    *(--kstack) = user_stack_top;   // User ESP
    *(--kstack) = 0x0202;           // EFLAGS (IF=1)
    *(--kstack) = 0x1B;             // User CS
    *(--kstack) = entry_point;      // EIP

    // --- 2. 構造給 child_ret_stub 恢復的 4 個暫存器 ---
    *(--kstack) = 0; // ebp
    *(--kstack) = 0; // ebx
    *(--kstack) = 0; // esi
    *(--kstack) = 0; // edi

    // --- 3. switch_task 的退場位址 ---
    *(--kstack) = (uint32_t) child_ret_stub;

    // --- 4. 應付 switch_task 裡的 popa 與 popf ---
    for(int i = 0; i < 8; i++) *(--kstack) = 0; 
    *(--kstack) = 0x0202;

    new_task->esp = (uint32_t) kstack;
    new_task->next = current_task->next;
    current_task->next = new_task;
}

int sys_fork(registers_t *regs) {
    task_t *child = (task_t*) kmalloc(sizeof(task_t));
    child->id = next_task_id++;
    child->state = TASK_RUNNING;
    child->wait_pid = 0;

    uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
    child->kernel_stack = (uint32_t) kstack;

    uint32_t child_ustack_base = 0x083FF000 - (child->id * 4096);
    uint32_t child_ustack_phys = pmm_alloc_page();
    map_page(child_ustack_base, child_ustack_phys, 7);

    uint32_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3));

    uint32_t parent_ustack_base = regs->user_esp & 0xFFFFF000;
    uint8_t *src = (uint8_t*) parent_ustack_base;
    uint8_t *dst = (uint8_t*) child_ustack_base;
    for(int i = 0; i < 4096; i++) { dst[i] = src[i]; }

    int32_t delta = child_ustack_base - parent_ustack_base;
    uint32_t child_user_esp = regs->user_esp + delta;

    // 【外科手術】修復 EBP 鏈結
    uint32_t child_ebp = regs->ebp;
    if (child_ebp >= parent_ustack_base && child_ebp < parent_ustack_base + 4096) {
        child_ebp += delta;
    }

    uint32_t curr_ebp = child_ebp;
    while (curr_ebp >= child_ustack_base && curr_ebp < child_ustack_base + 4096) {
        uint32_t *ebp_ptr = (uint32_t*) curr_ebp;
        uint32_t next_ebp = *ebp_ptr;
        if (next_ebp >= parent_ustack_base && next_ebp < parent_ustack_base + 4096) {
            *ebp_ptr = next_ebp + delta;
            curr_ebp = *ebp_ptr;
        } else {
            break;
        }
    }

    // --- 1. 構造硬體 IRET 幀 (由高位址到低位址) ---
    *(--kstack) = regs->user_ss;
    *(--kstack) = child_user_esp;
    *(--kstack) = regs->eflags;
    *(--kstack) = regs->cs;
    *(--kstack) = regs->eip;

    // --- 2. 構造給 child_ret_stub 恢復的 4 個暫存器 ---
    *(--kstack) = child_ebp; // 放入修正過的 ebp
    *(--kstack) = regs->ebx;
    *(--kstack) = regs->esi;
    *(--kstack) = regs->edi;

    // --- 3. switch_task 的退場位址 ---
    *(--kstack) = (uint32_t) child_ret_stub;

    // --- 4. 應付 switch_task 裡的 popa 與 popf ---
    for(int i = 0; i < 8; i++) *(--kstack) = 0;
    *(--kstack) = 0x0202;

    child->esp = (uint32_t) kstack;
    child->next = current_task->next;
    current_task->next = child;

    return child->id;
}

int sys_exec(registers_t *regs) {
    char* filename = (char*)regs->ebx;
    fs_node_t* file_node = simplefs_find(filename);
    if (file_node == 0) {
        kprintf("[Exec] Error: File '%s' not found!\n", filename);
        return -1;
    }

    uint8_t* buffer = (uint8_t*) kmalloc(file_node->length);
    vfs_read(file_node, 0, file_node->length, buffer);

    uint32_t entry_point = elf_load((elf32_ehdr_t*)buffer);
    kfree(buffer);

    if (entry_point == 0) return -1;

    uint32_t clean_user_stack_top = 0x083FF000 - (current_task->id * 4096) + 4096;

    regs->eip = entry_point;
    regs->user_esp = clean_user_stack_top;

    return 0;
}

int sys_wait(int pid) {
    task_t *temp = current_task->next;
    int found = 0;
    while (temp != current_task) {
        if (temp->id == (uint32_t)pid) { found = 1; break; }
        temp = temp->next;
    }
    if (!found) return 0;

    current_task->wait_pid = pid;
    current_task->state = TASK_WAITING;
    schedule();
    return 0;
}
```

這次你的初始任務 (`create_user_task`) 將能完美銜接 IRET 的需求順利降落，不再有 `-4` 的荒謬存取錯誤了。請再次 `make clean && make run`！
