#ifndef TASK_H
#define TASK_H
#include <stdint.h>

#define TASK_RUNNING 0
#define TASK_DEAD    1

typedef struct {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // 由 pusha 壓入
    uint32_t eip, cs, eflags, user_esp, user_ss;     // 由硬體中斷壓入
} registers_t;

typedef struct task {
    uint32_t esp;
    uint32_t id;
    uint32_t kernel_stack;
    uint32_t state;
    struct task *next;
} task_t;

// --- 公開 API ---

void init_multitasking();
void create_user_task(uint32_t entry_point, uint32_t user_stack_top);
void schedule();
int sys_fork(registers_t *regs);
void exit_task();

#endif
