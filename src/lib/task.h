#ifndef TASK_H
#define TASK_H
#include <stdint.h>

// 任務控制區塊 (TCB)
typedef struct task {
    uint32_t esp;       // 儲存這個任務的堆疊指標
    uint32_t id;        // 任務 ID
    uint32_t *stack;    // 分配給這個任務的實體堆疊記憶體 (方便日後回收)
    struct task *next;  // 指向下一個任務 (形成環狀連結串列)
} task_t;

void init_multitasking();
void create_kernel_thread(void (*entry_point)());
void schedule();

#endif
