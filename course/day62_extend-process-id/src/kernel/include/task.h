#ifndef TASK_H
#define TASK_H
#include <stdint.h>

// 行程狀態定義 (保留你原本的，並新增 ZOMBIE 狀態)
#define TASK_RUNNING 0
#define TASK_DEAD    1
#define TASK_WAITING 2
#define TASK_ZOMBIE  3  // 【新增】行程已結束，但父行程尚未 wait() 回收它

/**
 * [結構目的]：中斷上下文快照 (Interrupt Context Snapshot)
 * (保留你原本完美的註解與結構，這部分完全不動)
 */
typedef struct {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;

    uint32_t user_esp;
    uint32_t user_ss;
} registers_t;


typedef struct task {
    uint32_t esp;

    // ==========================================
    // 【Day 62 修改與新增】行程身分資訊 (PCB 擴充)
    // ==========================================
    uint32_t pid;               // 將原本的 id 改名為 pid (更符合 OS 慣例)
    uint32_t ppid;              // 【新增】父行程 ID (Parent PID)
    char name[32];              // 【新增】行程名稱 (例如 "shell.elf")

    uint32_t page_directory;    // 這個 Task 專屬的分頁目錄實體位址 (CR3)
    uint32_t kernel_stack;

    uint32_t state;
    uint32_t wait_pid;          // 保留！這是你 sys_wait 的核心機制

    uint32_t heap_start;        // 【新增】記錄 Heap 初始起點 (未來給 top 計算記憶體佔用量用)
    uint32_t heap_end;          // 記錄 User Heap 的當前頂點
    uint32_t cwd_lba;           // 當前工作目錄 (Current Working Directory, CWD) 的 LBA
    struct task *next;
} task_t;

extern volatile task_t *current_task;

void init_multitasking();

void create_user_task(uint32_t entry_point, uint32_t user_stack_top);
void exit_task();
void schedule();
int sys_fork(registers_t *regs);
int sys_exec(registers_t *regs);
int sys_wait(int pid);

#endif
