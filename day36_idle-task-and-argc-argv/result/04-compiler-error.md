
在編譯的時候，出現 syscall.c 的錯誤，我附上這兩段 source code.

```bash
❯ make clean; make; make debug
rm -f app.o my_app.elf
==> User App 已清除。
rm -f hdd.img
rm -f asm/*.o lib/*.o *.bin *.iso
rm -rf isodir
docker build -t os-builder .
[+] Building 2.1s (7/7) FINISHED                                                                             docker:orbstack
 => [internal] load build definition from Dockerfile                                                                    0.0s
 => => transferring dockerfile: 288B                                                                                    0.0s
 => WARN: FromPlatformFlagConstDisallowed: FROM --platform flag should not use constant value "linux/amd64" (line 1)    0.0s
 => [internal] load metadata for docker.io/library/debian:bullseye                                                      1.9s
 => [internal] load .dockerignore                                                                                       0.0s
 => => transferring context: 2B                                                                                         0.0s
 => [1/3] FROM docker.io/library/debian:bullseye@sha256:943d97fa707482c24e1bc2bdd0b0adc45f75eb345c61dc4272c4157f9a2cc9  0.0s
 => CACHED [2/3] RUN apt-get update && apt-get install -y     nasm     build-essential     grub-pc-bin     grub-common  0.0s
 => CACHED [3/3] WORKDIR /workspace                                                                                     0.0s
 => exporting to image                                                                                                  0.0s
 => => exporting layers                                                                                                 0.0s
 => => writing image sha256:2e718a84f2e8302d405984c3c30d974040605b9ac63e0ac05e677ea5eefbc551                            0.0s
 => => naming to docker.io/library/os-builder                                                                           0.0s

 1 warning found (use docker --debug to expand):
 - FromPlatformFlagConstDisallowed: FROM --platform flag should not use constant value "linux/amd64" (line 1)
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder nasm -f elf32 asm/boot.S -o asm/boot.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder nasm -f elf32 asm/gdt_flush.S -o asm/gdt_flush.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder nasm -f elf32 asm/interrupts.S -o asm/interrupts.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder nasm -f elf32 asm/paging.S -o asm/paging.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder nasm -f elf32 asm/switch_task.S -o asm/switch_task.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder nasm -f elf32 asm/user_mode.S -o asm/user_mode.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/ata.c -o lib/ata.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/elf.c -o lib/elf.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/gdt.c -o lib/gdt.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/idt.c -o lib/idt.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/kernel.c -o lib/kernel.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/keyboard.c -o lib/keyboard.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/kheap.c -o lib/kheap.o
lib/kheap.c: In function 'init_kheap':
lib/kheap.c:16:30: warning: initialization of 'uint32_t' {aka 'unsigned int'} from 'void *' makes integer from pointer without a cast [-Wint-conversion]
   16 |         uint32_t phys_addr = pmm_alloc_page();
      |                              ^~~~~~~~~~~~~~
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/mbr.c -o lib/mbr.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/paging.c -o lib/paging.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/pmm.c -o lib/pmm.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/simplefs.c -o lib/simplefs.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/syscall.c -o lib/syscall.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/task.c -o lib/task.o
lib/task.c: In function 'schedule':
lib/task.c:103:33: warning: passing argument 2 of 'switch_task' discards 'volatile' qualifier from pointer target type [-Wdiscarded-qualifiers]
  103 |         switch_task(&prev->esp, &current_task->esp);
      |                                 ^~~~~~~~~~~~~~~~~~
lib/task.c:19:58: note: expected 'uint32_t *' {aka 'unsigned int *'} but argument is of type 'volatile uint32_t *' {aka 'volatile unsigned int *'}
   19 | extern void switch_task(uint32_t *current_esp, uint32_t *next_esp);
      |                                                ~~~~~~~~~~^~~~~~~~
lib/task.c: In function 'sys_fork':
lib/task.c:191:34: warning: initialization of 'uint32_t' {aka 'unsigned int'} from 'void *' makes integer from pointer without a cast [-Wint-conversion]
  191 |     uint32_t child_ustack_phys = pmm_alloc_page();
      |                                  ^~~~~~~~~~~~~~
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/timer.c -o lib/timer.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/tty.c -o lib/tty.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/utils.c -o lib/utils.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib -c lib/vfs.c -o lib/vfs.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder ld -m elf_i386 -n -T linker.ld -o myos.bin asm/boot.o asm/gdt_flush.o asm/interrupts.o asm/paging.o asm/switch_task.o asm/user_mode.o lib/ata.o lib/elf.o lib/gdt.o lib/idt.o lib/kernel.o lib/keyboard.o lib/kheap.o lib/mbr.o lib/paging.o lib/pmm.o lib/simplefs.o lib/syscall.o lib/task.o lib/timer.o lib/tty.o lib/utils.o lib/vfs.o
ld: lib/syscall.o: in function `syscall_handler':
syscall.c:(.text+0x88): undefined reference to `sys_wait'
make: *** [myos.bin] Error 1
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day36_idle-task-and-argc-argv/src:/workspace -w /workspace os-builder ld -m elf_i386 -n -T linker.ld -o myos.bin asm/boot.o asm/gdt_flush.o asm/interrupts.o asm/paging.o asm/switch_task.o asm/user_mode.o lib/ata.o lib/elf.o lib/gdt.o lib/idt.o lib/kernel.o lib/keyboard.o lib/kheap.o lib/mbr.o lib/paging.o lib/pmm.o lib/simplefs.o lib/syscall.o lib/task.o lib/timer.o lib/tty.o lib/utils.o lib/vfs.o
ld: lib/syscall.o: in function `syscall_handler':
syscall.c:(.text+0x88): undefined reference to `sys_wait'
make: *** [myos.bin] Error 1
```

---

syscall.c
```c
#include "syscall.h"
#include "tty.h"
#include "utils.h"
#include "simplefs.h"
#include "keyboard.h"
#include "task.h"

// 【修復】拔掉 extern，讓它真正在這個檔案裡被分配記憶體空間！
fs_node_t* fd_table[32] = {0};

void init_syscalls(void) {
    kprintf("System Calls initialized on Interrupt 0x80 (128).\n");
}

void syscall_handler(registers_t *regs) {
    uint32_t eax = regs->eax;

    if (eax == 2) {
        kprintf((char*)regs->ebx);
    }
    else if (eax == 3) {
        char* filename = (char*)regs->ebx;
        fs_node_t* node = simplefs_find(filename);
        if (node == 0) { regs->eax = -1; return; }
        for (int i = 3; i < 32; i++) {
            if (fd_table[i] == 0) {
                fd_table[i] = node;
                regs->eax = i;
                return;
            }
        }
        regs->eax = -1;
    }
    else if (eax == 4) {
        int fd = (int)regs->ebx;
        uint8_t* buffer = (uint8_t*)regs->ecx;
        uint32_t size = (uint32_t)regs->edx;
        if (fd >= 3 && fd < 32 && fd_table[fd] != 0) {
            regs->eax = vfs_read(fd_table[fd], 0, size, buffer);
        } else {
            regs->eax = -1;
        }
    }
    else if (eax == 5) {
        char c = keyboard_getchar();
        char str[2] = {c, '\0'};
        kprintf(str);
        regs->eax = (uint32_t)c;
    }
    else if (eax == 6) {
        schedule();
    }
    else if (eax == 7) {
        exit_task();
    }
    else if (eax == 8) {
        regs->eax = sys_fork(regs);
    }
    else if (eax == 9) {
        regs->eax = sys_exec(regs);
    }
    else if (eax == 10) {
        regs->eax = sys_wait(regs->ebx);
    }
}
```

---

task.c
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
// 【新增】Idle Task 專屬變數
task_t *idle_task = 0;

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

    // [Day36]【新增】建立隱形的 Idle Task
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
        // while(1) { __asm__ volatile("sti; hlt"); }
        next_run = idle_task; // [Day36]【修改】如果大家都死了或睡著了，就派 Idle Task 上場！
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

// void create_user_task(uint32_t entry_point, uint32_t user_stack_top) {
//     task_t *new_task = (task_t*) kmalloc(sizeof(task_t));
//     new_task->id = next_task_id++;
//     new_task->state = TASK_RUNNING;
//     new_task->wait_pid = 0;

//     uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
//     uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
//     new_task->kernel_stack = (uint32_t) kstack;

//     *(--kstack) = 0x23;
//     *(--kstack) = user_stack_top;
//     *(--kstack) = 0x0202;
//     *(--kstack) = 0x1B;
//     *(--kstack) = entry_point;

//     *(--kstack) = 0;
//     *(--kstack) = 0;
//     *(--kstack) = 0;
//     *(--kstack) = 0;

//     *(--kstack) = (uint32_t) child_ret_stub;

//     for(int i = 0; i < 8; i++) *(--kstack) = 0;
//     *(--kstack) = 0x0202;

//     new_task->esp = (uint32_t) kstack;
//     new_task->next = current_task->next;
//     current_task->next = new_task;
// }

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

// int sys_exec(registers_t *regs) {
//     char* filename = (char*)regs->ebx;
//     fs_node_t* file_node = simplefs_find(filename);
//     if (file_node == 0) {
//         kprintf("[Exec] Error: File '%s' not found!\n", filename);
//         return -1;
//     }

//     uint8_t* buffer = (uint8_t*) kmalloc(file_node->length);
//     vfs_read(file_node, 0, file_node->length, buffer);

//     uint32_t entry_point = elf_load((elf32_ehdr_t*)buffer);
//     kfree(buffer);

//     if (entry_point == 0) return -1;

//     uint32_t clean_user_stack_top = 0x083FF000 - (current_task->id * 4096) + 4096;

//     regs->eip = entry_point;
//     regs->user_esp = clean_user_stack_top;

//     return 0;
// }

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


// 【新增】Idle Task 的靈魂：永遠開啟中斷並休眠
void idle_loop() {
    while(1) {
        __asm__ volatile("sti; hlt");
    }
}
```
