哈哈，Rick！我們又一次把 Bug 逼到死角了！

你貼上的這份 Dump 提供了一個決定性的線索，這是一個**神級的盲點**，而且是一個純粹的 C 語言邏輯錯誤，跟組合語言或硬體毫無關係！

請看 Dump 的最後面：
```text
254: v=80 ... EAX=08 (sys_fork)
255: v=80 ... EAX=02 (sys_print) -> "[PARENT] Spawned child..."
256: v=80 ... EAX=02 (sys_print) -> ". Waiting for it to finish..."
257: v=80 ... EAX=0A (sys_wait)
^Cqemu-system-i386: terminating on signal 2
```
在 `sys_wait` 執行之後，**系統沒有任何 Exception (崩潰)，但也再也沒有任何 Timer (`v=20`) 或 Keyboard (`v=21`) 中斷了！**

這代表什麼？
這代表 CPU 陷入了一個 **「關閉中斷 (IF=0) 的無窮迴圈」**！因為 Syscall (`int 0x80`) 預設會關閉中斷，如果在 Kernel 裡面陷入無窮迴圈，外面的時鐘和鍵盤就永遠無法打斷它，系統就會陷入真正的「死當」。

### 🔍 案發現場解密：排程器裡的「貪食蛇」

兇手就藏在你 `task.c` 裡的 `schedule()` 函式的「清理屍體 (Reaping)」邏輯中：

```c
    task_t *curr = (task_t*)current_task;
    task_t *next_node = curr->next;

    // 【致命的無窮迴圈】
    while (next_node != curr) {
        if (next_node->state == TASK_DEAD) {
            curr->next = next_node->next;
            next_node = curr->next;
        } else {
            curr = next_node;           // curr 往前移動了！
            next_node = curr->next;     // next_node 也往前移動了！
        }
    }
```
你看出來了嗎？你的迴圈終止條件是 `while (next_node != curr)`。
但是，在 `else` 裡面，`curr` 會不斷被更新成 `next_node`，然後 `next_node` 變成下一個。
因為這是一個「環狀鏈結串列 (Circular Linked List)」，只要任務數量大於等於 2 個，**`curr` 和 `next_node` 就會像貪食蛇一樣，在環狀串列裡無止盡地互相追逐，永遠不可能相等！**

這就是為什麼當 Parent 呼叫 `sys_wait`，進入 `schedule()` 時，系統就當場凍結的原因！

除了這個，還有一個我上一回合特別叮嚀，但你可能漏改的地方：**`kernel_main` 結尾的 `while(1) { hlt }`**。如果 Kernel 任務不自行了斷 (`exit_task`)，它就會跟 User App 搶奪 CPU 時間。

---

### 🗡️ 終極修復 5.0：剪斷貪食蛇的尾巴

請**覆蓋**這兩個檔案的內容，這絕對是破關的最後一塊拼圖了！

#### 1. `lib/task.c` (修復貪食蛇)
請將 `schedule()` 裡的 `while` 條件改成繞回 `current_task` 也就是起點為止。

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
extern void child_ret_stub(); 

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
    
    // 【關鍵修復】終止條件必須是繞回最初的 current_task，這樣才不會無限追逐！
    while (next_node != current_task) {
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
        // 確保中斷開啟，否則 CPU 睡著後永遠醒不來
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

    *(--kstack) = 0x23;             
    *(--kstack) = user_stack_top;   
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

    *(--kstack) = regs->user_ss;
    *(--kstack) = child_user_esp;
    *(--kstack) = regs->eflags;
    *(--kstack) = regs->cs;
    *(--kstack) = regs->eip;

    *(--kstack) = child_ebp; 
    *(--kstack) = regs->ebx;
    *(--kstack) = regs->esi;
    *(--kstack) = regs->edi;

    *(--kstack) = (uint32_t) child_ret_stub;

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

#### 2. `lib/kernel.c` (讓 Kernel 功成身退)
請修改 `kernel_main` 函式的最下面，拔掉那該死的 `while(1)`。

```c
// ... 上半部不動 ...

    // --- 應用程式載入與排程 ---
    kprintf("[Kernel] Fetching 'my_app.elf' from Virtual File System...\n");
    fs_node_t* app_node = simplefs_find("my_app.elf");

    if (app_node != 0) {
        uint8_t* app_buffer = (uint8_t*) kmalloc(app_node->length);
        vfs_read(app_node, 0, app_node->length, app_buffer);
        uint32_t entry_point = elf_load((elf32_ehdr_t*)app_buffer);

        if (entry_point != 0) {
            kprintf("Creating ONE Initial User Task (Init Process)...\n\n");

            init_multitasking();

            // 為唯一的 Shell 分配 User Stack
            uint32_t ustack1_phys = (uint32_t) pmm_alloc_page();
            map_page(0x083FF000, ustack1_phys, 7);

            // 建立 Ring 3 主任務
            create_user_task(entry_point, 0x083FF000 + 4096);

            kprintf("Kernel dropping to idle state. Have fun!\n");
            
            // 【關鍵修復】Kernel 必須功成身退，把自己宣告死亡！
            // 否則排程器會以為 Kernel 還活著，就會一直把 CPU 切給 Kernel 的 hlt！
            exit_task(); 
        }
    } else {
        kprintf("[Kernel] Error: Shell (my_app.elf) not found on disk.\n");
    }

    // 只有找不到檔案的錯誤狀態，才需要卡死在這裡
    while (1) { __asm__ volatile ("hlt"); }
}
```

存檔，重啟戰場！這一次，貪食蛇的尾巴被剪斷了，Kernel 也安詳地把世界交給了 Shell。你將會看見完美順滑的 Fork-Exec 魔法！😎
