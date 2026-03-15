#include "task.h"
#include "pmm.h"
#include "kheap.h"
#include "utils.h"
#include "tty.h"

volatile task_t *current_task = 0;
volatile task_t *ready_queue = 0;
uint32_t next_task_id = 0;

extern void switch_task(uint32_t *current_esp, uint32_t *next_esp);

void init_multitasking() {
    kprintf("[Task] Initializing Multitasking...\n");
    task_t *main_task = (task_t*) kmalloc(sizeof(task_t));
    main_task->id = next_task_id++;
    main_task->esp = 0;
    main_task->next = main_task;
    current_task = main_task;
    ready_queue = main_task;
}

void create_kernel_thread(void (*entry_point)()) {
    task_t *new_task = (task_t*) kmalloc(sizeof(task_t));
    new_task->id = next_task_id++;

    // 【修復 1】使用 kmalloc 分配虛擬記憶體，避免 Page Fault！
    uint32_t *stack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *stack = (uint32_t*) ((uint32_t)stack_mem + 4096); // 指向堆疊頂部
    new_task->stack = stack_mem;

    // 【修復 2】嚴格對應 switch_task 的 pop 順序！
    *(--stack) = (uint32_t) entry_point; // ret 彈出的 EIP
    *(--stack) = 0; // EAX
    *(--stack) = 0; // ECX
    *(--stack) = 0; // EDX
    *(--stack) = 0; // EBX
    *(--stack) = 0; // ESP (pusha 會忽略這個)
    *(--stack) = 0; // EBP
    *(--stack) = 0; // ESI
    *(--stack) = 0; // EDI
    *(--stack) = 0x0202; // EFLAGS (pushf 最後壓入，所以 popf 最先彈出！)

    new_task->esp = (uint32_t) stack;

    new_task->next = current_task->next;
    current_task->next = new_task;
}

void schedule() {
    if (!current_task || current_task->next == current_task) return;
    task_t *prev_task = (task_t*)current_task;
    current_task = current_task->next;
    switch_task(&prev_task->esp, &current_task->esp);
}
