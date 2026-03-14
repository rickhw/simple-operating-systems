#ifndef TASK_H
#define TASK_H

#include <stdint.h>

// 任務控制區塊 (Task Control Block, TCB)
typedef struct task {
    uint32_t esp;       // [最重要] 記錄這個任務專屬的 Stack Pointer
    uint32_t stack_top; // 記錄這個任務配置到的記憶體頂端 (用來釋放記憶體)
    int id;             // 任務的 ID
    struct task* next;  // 指向下一個任務 (用於實作排程的 Linked List)
} task_t;

// 宣告切換任務的組合語言函式
extern void switch_task(uint32_t* old_esp_ptr, uint32_t new_esp);

#endif
