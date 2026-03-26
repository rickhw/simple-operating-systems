#ifndef TASK_H
#define TASK_H
#include <stdint.h>

// [Day33] 定義任務的生命狀態
#define TASK_RUNNING 0
#define TASK_DEAD    1

typedef struct task {
    uint32_t esp;           // 任務目前的堆疊指標 (Context Switch 用)
    uint32_t id;
    uint32_t kernel_stack;  // [新增] 此任務專屬的核心堆疊頂端位址 (供 TSS 使用)
    uint32_t state;         // [Day33][新增] 紀錄任務現在是活著還是死了
    struct task *next;
} task_t;

void init_multitasking();
void create_user_task(uint32_t entry_point, uint32_t user_stack_top); // [新增] 建立平民任務
// void create_kernel_thread(void (*entry_point)());
void schedule();
void exit_task();           // [Day33][新增] 核心用來處決任務的函式

#endif
