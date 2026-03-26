#include "task.h"
#include "pmm.h"
#include "kheap.h"
#include "utils.h"
#include "tty.h"
#include "gdt.h" // 為了使用 set_kernel_stack()

volatile task_t *current_task = 0;
volatile task_t *ready_queue = 0;
uint32_t next_task_id = 0;

extern void switch_task(uint32_t *current_esp, uint32_t *next_esp);
extern void task_return_stub(); // 剛剛在組合語言寫的跳板

void init_multitasking() {
    kprintf("[Task] Initializing Multitasking...\n");
    task_t *main_task = (task_t*) kmalloc(sizeof(task_t));
    main_task->id = next_task_id++;
    main_task->esp = 0;

    // [新增] 確保主核心任務的 kernel_stack 初始為 0
    main_task->kernel_stack = 0;
    main_task->state = TASK_RUNNING; // [Day33][新增] 初始化為存活

    main_task->next = main_task;
    current_task = main_task;
    ready_queue = main_task;
}

// [新增] 建立 Ring 3 任務的終極工廠
void create_user_task(uint32_t entry_point, uint32_t user_stack_top) {
    task_t *new_task = (task_t*) kmalloc(sizeof(task_t));
    new_task->id = next_task_id++;

    // 1. 配發此任務專屬的 Kernel Stack (4KB)
    uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
    new_task->kernel_stack = (uint32_t) kstack;

    // --- 開始由上往下 (高位址到低位址) 偽造堆疊 ---

    // 階段 A: 偽造 Ring 3 的硬體中斷幀 (給 task_return_stub 的 iret 使用)
    *(--kstack) = 0x23;             // SS (User Data Segment，RPL=3)
    *(--kstack) = user_stack_top;   // ESP (平民專屬堆疊)
    *(--kstack) = 0x0202;           // EFLAGS (IF=1，開啟中斷)
    *(--kstack) = 0x1B;             // CS (User Code Segment，RPL=3)
    *(--kstack) = entry_point;      // EIP (程式進入點)

    // 階段 B: 偽造 ISR 壓入的除錯資訊 (給 task_return_stub 的 add esp, 8 消耗)
    *(--kstack) = 0; // 假 Error Code
    *(--kstack) = 0; // 假 Int Number

    // 階段 C: 偽造 ISR 的 pusha (給 task_return_stub 的 popa 消耗)
    for(int i = 0; i < 8; i++) *(--kstack) = 0;

    // 階段 D: 偽造 switch_task 的返回狀態
    *(--kstack) = (uint32_t) task_return_stub; // 給 switch_task 的 ret 消耗

    // 【關鍵修復】補上給 switch_task 的 popa 消耗的 8 個暫存器空間！
    for(int i = 0; i < 8; i++) *(--kstack) = 0;

    *(--kstack) = 0x0202; // 給 switch_task 的 popf 消耗

    new_task->esp = (uint32_t) kstack;
    new_task->state = TASK_RUNNING; // [Day33][新增] 新生兒預設為存活

    // 插入排程佇列
    new_task->next = current_task->next;
    current_task->next = new_task;
}

// void create_kernel_thread(void (*entry_point)()) {
//     task_t *new_task = (task_t*) kmalloc(sizeof(task_t));
//     new_task->id = next_task_id++;

//     // 【修復 1】使用 kmalloc 分配虛擬記憶體，避免 Page Fault！
//     uint32_t *stack_mem = (uint32_t*) kmalloc(4096);
//     uint32_t *stack = (uint32_t*) ((uint32_t)stack_mem + 4096); // 指向堆疊頂部
//     new_task->stack = stack_mem;

//     // 【修復 2】嚴格對應 switch_task 的 pop 順序！
//     *(--stack) = (uint32_t) entry_point; // ret 彈出的 EIP
//     *(--stack) = 0; // EAX
//     *(--stack) = 0; // ECX
//     *(--stack) = 0; // EDX
//     *(--stack) = 0; // EBX
//     *(--stack) = 0; // ESP (pusha 會忽略這個)
//     *(--stack) = 0; // EBP
//     *(--stack) = 0; // ESI
//     *(--stack) = 0; // EDI
//     *(--stack) = 0x0202; // EFLAGS (pushf 最後壓入，所以 popf 最先彈出！)

//     new_task->esp = (uint32_t) stack;

//     new_task->next = current_task->next;
//     current_task->next = new_task;
// }

// [Day33][新增] 處決當前任務的函式
void exit_task() {
    current_task->state = TASK_DEAD;
    // 標記為死亡後，立刻呼叫排程器強行切換，一去不復返！
    schedule();
}

// [升級] 具有「清理屍體」能力的排程器
void schedule() {
    if (!current_task) return;

    task_t *prev = (task_t*)current_task;
    task_t *next = current_task->next;

    // 沿著環狀佇列尋找下一個「活著」的任務，並將死掉的剃除
    while (next->state == TASK_DEAD && next != current_task) {
        prev->next = next->next; // 將指標繞過死掉的任務，將其從佇列中解開
        // (未來我們可以在這裡呼叫 kfree() 回收它的記憶體與堆疊)
        next = prev->next;
    }

    // 如果繞了一圈發現大家都死了 (包含自己)
    if (next == current_task && current_task->state == TASK_DEAD) {
        kprintf("\n[Kernel] All user processes terminated. System Idle.\n");
        while(1) { __asm__ volatile("cli; hlt"); } // 系統進入永久安眠
    }

    current_task = next;

    // 如果切換後的任務不是原本的任務 (代表真的有發生切換)
    if (current_task != prev) {
        if (current_task->kernel_stack != 0) {
            set_kernel_stack(current_task->kernel_stack);
        }
        switch_task(&prev->esp, &current_task->esp);
    }
}
