哈哈，Rick！不要氣餒，我們遇到了所有 C/C++ 開發者都曾經歷過的「連結器 (Linker) 撞牆期」！

這一次的錯誤完全不是你的邏輯有問題，而是 **檔案的擺放位置** 和 **標頭檔 (Header) 的宣告** 產生了誤會。

讓我們來拆解這兩個錯誤：

### 💥 錯誤 1：`multiple definition of '_start'` 與 `undefined reference to 'main'`
這是一個超經典的 Makefile 陷阱！
請看你的錯誤訊息：`asm/crt0.o: multiple definition of '_start'`。

**兇手是誰？** 因為你把 `crt0.S` 放進了 **`asm/`** 資料夾裡！
你的 `Makefile` 有這神奇的一行：`ASM_SOURCES = $(wildcard asm/*.S)`。這行會自動把 `asm/` 裡面所有的 `.S` 檔都抓起來，編譯並塞進 **Kernel (`myos.bin`)** 裡！

但 `crt0.S` 是我們寫給 **User App (Shell)** 用的跳板！
結果 Kernel 把 `crt0.o` 吞進去後，發現裡面有一個 `_start`，但 Kernel 的 `boot.S` 早就已經有一個 `_start` 了，於是發生「重複定義」。接著 Kernel 又發現 `crt0.o` 裡面要呼叫 `main`，但 Kernel 根本沒有 `main` 函式，於是又噴出 `undefined reference to 'main'`！

**🛠️ 解決方法：**
非常簡單，**請把 `crt0.S` 從 `asm/` 資料夾移出來，直接放在專案的根目錄（也就是跟 `app.c` 和 `Makefile` 放一起）！**
你的 Makefile 裡面寫的 `crt0.o: crt0.S` 規則就是預期它在根目錄。只要搬移檔案，這個錯誤就會瞬間消失！

---

### 💥 錯誤 2：`undefined reference to 'sys_wait'`
這個錯誤代表 `syscall.c` 在呼叫 `sys_wait` 時，連結器在所有的 `.o` 檔裡面「找不到」這個函式。
雖然你說 `task.c` 裡面有這個函式，但有時候因為複製貼上時漏掉括號、或者是 `task.h` 沒有正確宣告，會導致編譯器以為它不存在。

為了確保萬無一失，我們進行一次「大掃除」，請確認並覆蓋以下兩個檔案：

#### 1. 確保 `lib/task.h` 有正確宣告
請打開 **`lib/task.h`**，確認裡面有 `sys_wait` 的宣告，並包含所有的屬性：

```c
#ifndef TASK_H
#define TASK_H
#include <stdint.h>

typedef struct {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eip, cs, eflags, user_esp, user_ss;
} registers_t;

#define TASK_RUNNING 0
#define TASK_DEAD    1
#define TASK_WAITING 2

typedef struct task {
    uint32_t esp;
    uint32_t id;
    uint32_t kernel_stack;
    uint32_t state;
    uint32_t wait_pid;
    struct task *next;
} task_t;

extern volatile task_t *current_task;

void init_multitasking();
void create_user_task(uint32_t entry_point, uint32_t user_stack_top);
void schedule();
void exit_task();
int sys_fork(registers_t *regs);
int sys_exec(registers_t *regs);

// 【請確認這行存在！】
int sys_wait(int pid); 

#endif
```

#### 2. 完整替換 `lib/task.c`
為了避免任何括號錯位或合併錯誤，請直接用這份 **最完整、最乾淨** 的版本覆蓋你的 `lib/task.c`：

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
task_t *idle_task = 0;

extern void switch_task(uint32_t *current_esp, uint32_t *next_esp);
extern void child_ret_stub();

void idle_loop() {
    while(1) { __asm__ volatile("sti; hlt"); }
}

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

    idle_task = (task_t*) kmalloc(sizeof(task_t));
    idle_task->id = 9999; 
    idle_task->state = TASK_RUNNING;
    idle_task->wait_pid = 0;

    uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
    idle_task->kernel_stack = (uint32_t) kstack;

    *(--kstack) = (uint32_t) idle_loop; 
    for(int i = 0; i < 8; i++) *(--kstack) = 0; 
    *(--kstack) = 0x0202; 

    idle_task->esp = (uint32_t) kstack;
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

void create_user_task(uint32_t entry_point, uint32_t user_stack_top) {
    task_t *new_task = (task_t*) kmalloc(sizeof(task_t));
    new_task->id = next_task_id++;
    new_task->state = TASK_RUNNING;
    new_task->wait_pid = 0;

    uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
    new_task->kernel_stack = (uint32_t) kstack;

    // 留給 User Stack 一點呼吸空間
    uint32_t *ustack = (uint32_t*) (user_stack_top - 64); 

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
    char** argv = (char**)regs->ecx;

    fs_node_t* file_node = simplefs_find(filename);
    if (file_node == 0) { return -1; }

    uint8_t* buffer = (uint8_t*) kmalloc(file_node->length);
    vfs_read(file_node, 0, file_node->length, buffer);
    uint32_t entry_point = elf_load((elf32_ehdr_t*)buffer);
    kfree(buffer);

    if (entry_point == 0) return -1;

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

---

完成這兩步（特別是把 `crt0.S` 從 `asm/` 移出來）後，你的編譯一定能順利通過。趕快 `make clean && make run`，見證我們完美移植 C 語言參數傳遞機制的歷史時刻吧！🚀
