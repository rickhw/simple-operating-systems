#ifndef TASK_H
#define TASK_H
#include <stdint.h>

/**
 * [結構目的]：中斷上下文快照 (Interrupt Context Snapshot)
 * * 此結構體的佈局必須嚴格遵循 x86 中斷發生時的堆疊增長順序。
 * 當進入 isr128 (或其他中斷) 時：
 * 1. CPU 自動壓入: ss, esp, eflags, cs, eip (共 5 個)
 * 2. 軟體手動壓入: pusha (壓入 8 個通用暫存器)
 * * 記憶體佈局 (由低位址往高位址)：
 * [ 低位址 ] edi -> esi -> ebp -> esp -> ebx -> edx -> ecx -> eax -> eip -> cs -> eflags -> user_esp -> user_ss [ 高位址 ]
 */
/**
 * 位址方向      暫存器內容 (Data)          說明 / 來源
    --------    -----------------------    --------------------------
     高位址  ↑   [  User SS      ] 0x23     <-- CPU 自動壓入 (最後)
            |   [  User ESP     ] 0x...    <-- CPU 自動壓入
            |   [  EFLAGS       ] 0x202    <-- CPU 自動壓入 (中斷狀態)
            |   [  CS (Code Seg)] 0x1B     <-- CPU 自動壓入 (權限位元)
            |   [  EIP (Return) ] 0x...    <-- CPU 自動壓入 (第一)
            |   -----------------------    (以上為 Interrupt Frame)
            |   [  EAX          ]          <-- pusha 開始 (1st)
            |   [  ECX          ]          <-- pusha
            |   [  EDX          ]          <-- pusha
            |   [  EBX          ]          <-- pusha
            |   [  ESP (Kernel) ]          <-- pusha (在此時的 ESP)
            |   [  EBP          ]          <-- pusha
            |   [  ESI          ]          <-- pusha
     低位址  ↓   [  EDI          ]          <-- pusha 結束 (Last)
    --------    -----------------------    <-- 這裡就是 registers_t *regs 的指標位置
 */
typedef struct {
    /* 由 pusha 指令壓入的通用暫存器 (順序由後往前) */
    uint32_t edi;    // Destination Index: 常用於記憶體拷貝目的地
    uint32_t esi;    // Source Index: 常用於記憶體拷貝來源
    uint32_t ebp;    // Base Pointer: 堆疊基底指標，用於 Stack Trace
    uint32_t esp;    // Stack Pointer: 核心模式下的堆疊頂端 (pusha 壓入的值)
    uint32_t ebx;    // Base Register: 常用於存取資料段位址
    uint32_t edx;    // Data Register: 常用於 I/O 埠操作或乘除法
    uint32_t ecx;    // Counter Register: 常用於迴圈計數
    uint32_t eax;    // Accumulator Register: 函式回傳值或 Syscall 編號

    /* 以下由 CPU 在發生中斷時「自動」壓入堆疊 (Interrupt Stack Frame) */
    uint32_t eip;    // Instruction Pointer: 程式被中斷時的下一條指令位址
    uint32_t cs;     // Code Segment: 包含權限等級 (CPL)，決定是 Ring 0 還是 3
    uint32_t eflags; // CPU Flags: 包含中斷開關 (IF) 與運算狀態標誌

    /* 以下兩項僅在「權限切換」(Ring 3 -> Ring 0) 時由 CPU 自動壓入 */
    uint32_t user_esp; // User Stack Pointer: 使用者原本的堆疊位址
    uint32_t user_ss;  // User Stack Segment: 使用者原本的堆疊段選擇子 (通常 0x23)
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
