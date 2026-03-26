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
    uint32_t page_directory; // [Day38]【新增】這個 Task 專屬的分頁目錄實體位址 (CR3)
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
int sys_wait(int pid);

#endif
