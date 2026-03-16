#ifndef TASK_H
#define TASK_H
#include <stdint.h>

// 【核心魔法】這完美對應了 pusha 與硬體中斷壓入堆疊的順序！
typedef struct {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // 由 pusha 壓入
    uint32_t eip, cs, eflags, user_esp, user_ss;     // 由硬體中斷壓入
} registers_t;

#define TASK_RUNNING 0
#define TASK_DEAD    1

typedef struct task {
    uint32_t esp;
    uint32_t id;
    uint32_t kernel_stack;
    uint32_t state;
    struct task *next;
} task_t;

void init_multitasking();
void create_user_task(uint32_t entry_point, uint32_t user_stack_top);
void schedule();
void exit_task();
int sys_fork(registers_t *regs); // [修改] 接收精準的暫存器指標

#endif
