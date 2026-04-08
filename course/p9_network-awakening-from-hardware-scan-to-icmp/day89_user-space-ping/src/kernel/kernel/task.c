/**
 * @file src/kernel/kernel/task.c
 * @brief Task management and multitasking system.
 *
 * This file handles task creation, scheduling, and process lifecycle management.
 * It implements a circular linked list of tasks for round-robin scheduling.
 */

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
#include "gui.h"

volatile task_t *current_task = 0;
volatile task_t *ready_queue = 0;
uint32_t next_pid = 0;
task_t *idle_task = 0;

extern uint32_t page_directory[];
extern void switch_task(uint32_t *current_esp, uint32_t *next_esp, uint32_t next_cr3);  // switch_task.S
extern void child_ret_stub();   // switch_task.S

// --- Internal API ----

/**
 * @brief Idle loop for the idle task.
 */
void idle_loop() {
    while(1) { __asm__ volatile("sti; hlt"); }
}

// ---- Public API ----

/**
 * @brief Initialize multitasking system.
 */
void init_multitasking() {
    task_t *main_task = (task_t*) kmalloc(sizeof(task_t));
    main_task->pid = next_pid++;
    main_task->ppid = 0;
    strcpy(main_task->name, "[kernel_main]");

    main_task->esp = 0;
    main_task->kernel_stack = 0;
    main_task->state = TASK_RUNNING;
    main_task->wait_pid = 0;
    main_task->page_directory = (uint32_t) page_directory;
    main_task->cwd_lba = 0;
    main_task->total_ticks = 0;

    main_task->next = main_task;
    current_task = main_task;
    ready_queue = main_task;

    idle_task = (task_t*) kmalloc(sizeof(task_t));
    idle_task->pid = 9999;
    idle_task->ppid = 0;
    strcpy(idle_task->name, "[kernel_idle]");
    idle_task->state = TASK_RUNNING;
    idle_task->wait_pid = 0;
    idle_task->page_directory = (uint32_t) page_directory;
    idle_task->cwd_lba = 0;
    idle_task->total_ticks = 0;

    uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
    idle_task->kernel_stack = (uint32_t) kstack;

    *(--kstack) = (uint32_t) idle_loop;
    for(int i = 0; i < 8; i++) *(--kstack) = 0;
    *(--kstack) = 0x0202;

    idle_task->esp = (uint32_t) kstack;
}

/**
 * @brief Create the first user task.
 * @param entry_point The entry point of the task.
 * @param user_stack_top The top of the user stack.
 */
void create_user_task(uint32_t entry_point, uint32_t user_stack_top) {
    task_t *new_task = (task_t*) kmalloc(sizeof(task_t));
    new_task->pid = next_pid++;
    new_task->ppid = current_task->pid;
    strcpy(new_task->name, "init"); // The first process is named init

    new_task->state = TASK_RUNNING;
    new_task->wait_pid = 0;
    new_task->page_directory = current_task->page_directory;
    new_task->cwd_lba = 0;
    new_task->total_ticks = 0;

    // Pre-allocate 256 physical pages (1MB) for User Heap
    for (int i = 0; i < 256; i++) {
        uint32_t heap_phys = (uint32_t)pmm_alloc_page();
        map_page(0x10000000 + (i * 4096), heap_phys, 7);
    }
    new_task->heap_start = 0x10000000;
    new_task->heap_end = 0x10000000;

    uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
    new_task->kernel_stack = (uint32_t) kstack;

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

/**
 * @brief Exit current task.
 */
void exit_task() {
    // When a process exits normally, close its windows!
    gui_close_windows_by_pid(current_task->pid);

    task_t *temp = current_task->next;
    int parent_waiting = 0;

    while (temp != current_task) {
        if (temp->state == TASK_WAITING && temp->wait_pid == current_task->pid) {
            temp->state = TASK_RUNNING;
            temp->wait_pid = 0;
            parent_waiting = 1;
        }
        temp = temp->next;
    }

    if (current_task->page_directory != (uint32_t)page_directory) {
        free_page_directory(current_task->page_directory);
    }

    // If no parent is waiting, it becomes DEAD, otherwise ZOMBIE
    if (current_task->ppid == 0 || !parent_waiting) {
        current_task->state = TASK_DEAD;
    } else {
        current_task->state = TASK_ZOMBIE;
    }

    schedule();
}

/**
 * @brief Task scheduler (Round Robin).
 */
void schedule() {
    if (!current_task) return;

    task_t *curr = (task_t*)current_task;
    task_t *next_node = curr->next;

    // Find and remove DEAD tasks
    while (next_node != current_task) {
        if (next_node->state == TASK_DEAD) {
            // Remove from Circular Linked List
            curr->next = next_node->next;

            // Garbage Collection: Release memory
            if (next_node->kernel_stack != 0) {
                // Return to the original pointer from kmalloc
                kfree((void*)(next_node->kernel_stack - 4096));
            }
            // Release PCB structure itself
            kfree((void*)next_node);

            // Move to next node
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
        switch_task((uint32_t*)&prev->esp, (uint32_t*)&current_task->esp, current_task->page_directory);
    }
}

/**
 * @brief Fork a new process.
 * @param regs The current register state.
 * @return The PID of the child process.
 */
int sys_fork(registers_t *regs) {
    task_t *child = (task_t*) kmalloc(sizeof(task_t));
    child->pid = next_pid++;
    child->ppid = current_task->pid;
    strcpy(child->name, (const char*)current_task->name); // Inherit parent's name

    child->state = TASK_RUNNING;
    child->wait_pid = 0;
    child->page_directory = current_task->page_directory;
    child->cwd_lba = current_task->cwd_lba;

    child->heap_start = current_task->heap_start;
    child->heap_end = current_task->heap_end;
    child->total_ticks = 0; // Reset ticks for the new process

    uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
    child->kernel_stack = (uint32_t) kstack;

    // Ensure non-conflicting user stack address using PID
    uint32_t child_ustack_base = 0x083FF000 - (child->pid * 4096);
    uint32_t child_ustack_phys = (uint32_t)pmm_alloc_page();
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

    return child->pid;
}

/**
 * @brief Execute a new program in the current process.
 * @param regs The current register state.
 * @return 0 on success, -1 on failure.
 */
int sys_exec(registers_t *regs) {
    char* filename = (char*)regs->ebx;
    char** old_argv = (char**)regs->ecx;

    uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;
    fs_node_t* file_node = simplefs_find(current_dir, filename);

    if (file_node == 0 && current_dir != 1) {
        file_node = simplefs_find(1, filename);
    }
    if (file_node == 0) { return -1; }

    // Update process name
    strcpy((char*)current_task->name, filename);

    uint8_t* buffer = (uint8_t*) kmalloc(file_node->length);
    vfs_read(file_node, 0, file_node->length, buffer);

    int argc = 0;
    char* safe_argv[16];

    if (old_argv != 0 && (uint32_t)old_argv > 0x08000000) {
        while (old_argv[argc] != 0 && argc < 15) {
            int len = strlen(old_argv[argc]) + 1;
            safe_argv[argc] = (char*) kmalloc(len);
            memcpy(safe_argv[argc], old_argv[argc], len);
            argc++;
        }
    }
    safe_argv[argc] = 0;

    uint32_t new_cr3 = create_page_directory();
    uint32_t old_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(old_cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(new_cr3));

    uint32_t entry_point = elf_load((elf32_ehdr_t*)buffer);
    kfree(buffer);

    if (entry_point == 0) {
        __asm__ volatile("mov %0, %%cr3" : : "r"(old_cr3));
        return -1;
    }

    current_task->page_directory = new_cr3;

    uint32_t clean_user_stack_top = 0x083FF000 + 4096;
    uint32_t ustack_phys = (uint32_t)pmm_alloc_page();
    map_page(0x083FF000, ustack_phys, 7);

    // Pre-allocate 256 physical pages (1MB) for User Heap
    for (int i = 0; i < 256; i++) {
        uint32_t heap_phys = (uint32_t)pmm_alloc_page();
        map_page(0x10000000 + (i * 4096), heap_phys, 7);
    }
    current_task->heap_start = 0x10000000;
    current_task->heap_end = 0x10000000;

    uint32_t stack_ptr = clean_user_stack_top - 64;

    uint32_t argv_ptrs[16] = {0};
    if (argc > 0) {
        for (int i = argc - 1; i >= 0; i--) {
            int len = strlen(safe_argv[i]) + 1;
            stack_ptr -= len;
            memcpy((void*)stack_ptr, safe_argv[i], len);
            argv_ptrs[i] = stack_ptr;
            kfree(safe_argv[i]);
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
        stack_ptr -= 4; *(uint32_t*)stack_ptr = 0;
        stack_ptr -= 4; *(uint32_t*)stack_ptr = 0;
    }

    stack_ptr -= 4; *(uint32_t*)stack_ptr = 0;

    regs->eip = entry_point;
    regs->user_esp = stack_ptr;

    return 0;
}

/**
 * @brief Wait for a child process to exit.
 * @param pid The PID of the child process.
 * @return 0 on success, -1 on failure.
 */
int sys_wait(uint32_t pid) {
    task_t *temp = current_task->next;
    int found = 0;

    // Check if the child is already a zombie
    while (temp != current_task) {
        if (temp->pid == pid) {
            found = 1;
            if (temp->state == TASK_ZOMBIE) {
                // Child already exited, clean up and return
                temp->state = TASK_DEAD;
                return 0;
            }
            break;
        }
        temp = temp->next;
    }

    if (!found) return -1; // Child process not found

    // Parent waits for child to exit
    current_task->wait_pid = pid;
    current_task->state = TASK_WAITING;
    schedule();

    // Parent woken up, check for zombie child to clean up
    temp = current_task->next;
    while (temp != current_task) {
        if (temp->pid == pid && temp->state == TASK_ZOMBIE) {
            temp->state = TASK_DEAD;
            break;
        }
        temp = temp->next;
    }

    return 0;
}

/**
 * @brief Kill a specified process.
 * @param pid The PID of the process to kill.
 * @return 0 on success, -1 on failure.
 */
int sys_kill(uint32_t pid) {
    if (pid <= 1) return -1; // Prevent killing Kernel Idle (0) and Kernel Main (1)

    __asm__ volatile("cli");
    task_t *temp = (task_t*)current_task;
    int found = 0;

    // Traverse all processes to find the target
    do {
        if (temp->pid == pid && temp->state != TASK_DEAD) {
            // Close all windows owned by this process
            gui_close_windows_by_pid(pid);

            // Check if parent is waiting for this child
            volatile task_t *parent = current_task;
            int parent_waiting = 0;
            do {
                if (parent->pid == temp->ppid && parent->state == TASK_WAITING && parent->wait_pid == pid) {
                    parent->state = TASK_RUNNING;
                    parent->wait_pid = 0;
                    parent_waiting = 1;
                    break;
                }
                parent = parent->next;
            } while (parent != current_task);

            // If parent is waiting, child becomes ZOMBIE, otherwise DEAD
            if (parent_waiting) {
                temp->state = TASK_ZOMBIE;
            } else {
                temp->state = TASK_DEAD;
            }

            found = 1;
            break;
        }
        temp = temp->next;
    } while (temp != current_task);

    __asm__ volatile("sti");

    return found ? 0 : -1;
}

/**
 * @brief Get a list of all processes.
 * @param list Buffer to store process information.
 * @param max_count Maximum number of processes to return.
 * @return Number of processes found.
 */
int task_get_process_list(process_info_t* list, int max_count) {
    if (!current_task) return 0;

    int count = 0;
    task_t* temp = (task_t*)current_task;

    // Traverse the circular linked list
    do {
        if (temp->state != TASK_DEAD) {
            list[count].pid = temp->pid;
            list[count].ppid = temp->ppid;
            strncpy(list[count].name, temp->name, 32);
            list[count].state = temp->state;
            list[count].total_ticks = temp->total_ticks;

            // Calculate memory usage
            if (temp->heap_end >= temp->heap_start) {
                // User Task: dynamic heap + pre-allocated 1MB heap + 4KB user stack
                list[count].memory_used = (temp->heap_end - temp->heap_start) + (256 * 4096) + 4096;
            } else {
                // Kernel Task: only kernel stack (4KB)
                list[count].memory_used = 4096;
            }

            count++;
        }
        temp = temp->next;
    } while (temp != current_task && count < max_count);

    return count;
}
