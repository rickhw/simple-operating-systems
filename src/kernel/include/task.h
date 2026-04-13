#ifndef TASK_H
#define TASK_H
#include <stdint.h>

// ==========================================
// 記憶體分佈與權限常數定義 (Memory Layout & Privileges)
// ==========================================
#define PAGE_SIZE           4096
#define USER_HEAP_START     0x10000000
#define USER_HEAP_PAGES     256
#define USER_STACK_TOP      0x083FF000
#define USER_STACK_PADDING  64
#define USER_ARGS_LIMIT     0x08000000
#define MAX_EXEC_ARGS       16

// 段選擇子 (Segment Selectors)
#define USER_DS             0x23
#define USER_CS             0x1B
#define KERNEL_DS           0x10
#define KERNEL_CS           0x08
#define EFLAGS_DEFAULT      0x0202

#define KERNEL_IDLE_PID     9999

/** 行程狀態定義 */
#define TASK_RUNNING 0
#define TASK_DEAD    1
#define TASK_WAITING 2
#define TASK_ZOMBIE  3  ///< 行程已結束，但父行程尚未 wait() 回收它
#define TASK_SLEEPING 4 ///< 睡眠中

/**
 * @brief 中斷上下文快照 (Interrupt Context Snapshot)
 * * 此結構體的佈局必須嚴格遵循 x86 中斷發生時的堆疊增長順序。
 * 記憶體佈局 (由低位址往高位址)：
 * [低] edi -> esi -> ebp -> esp -> ebx -> edx -> ecx -> eax -> eip -> cs -> eflags -> user_esp -> user_ss [高]
 */
typedef struct {
    /* 由 pusha 指令壓入的通用暫存器 (順序由後往前) */
    uint32_t edi;    ///< Destination Index: 常用於記憶體拷貝目的地
    uint32_t esi;    ///< Source Index: 常用於記憶體拷貝來源
    uint32_t ebp;    ///< Base Pointer: 堆疊基底指標，用於 Stack Trace
    uint32_t esp;    ///< Stack Pointer: 核心模式下的堆疊頂端 (pusha 壓入的值)
    uint32_t ebx;    ///< Base Register: 常用於存取資料段位址
    uint32_t edx;    ///< Data Register: 常用於 I/O 埠操作或乘除法
    uint32_t ecx;    ///< Counter Register: 常用於迴圈計數
    uint32_t eax;    ///< Accumulator Register: 函式回傳值或 Syscall 編號

    /* 由 CPU 在某些例外發生時自動壓入的錯誤碼 (一般中斷手動補 0) */
    uint32_t err_code;

    /* 以下由 CPU 在發生中斷時「自動」壓入堆疊 (Interrupt Stack Frame) */
    uint32_t eip;    ///< Instruction Pointer: 程式被中斷時的下一條指令位址
    uint32_t cs;     ///< Code Segment: 包含權限等級 (CPL)，決定是 Ring 0 還是 3
    uint32_t eflags; ///< CPU Flags: 包含中斷開關 (IF) 與運算狀態標誌

    /* 以下兩項僅在「權限切換」(Ring 3 -> Ring 0) 時由 CPU 自動壓入 */
    uint32_t user_esp; ///< User Stack Pointer: 使用者原本的堆疊位址
    uint32_t user_ss;  ///< User Stack Segment: 使用者原本的堆疊段選擇子 (通常 0x23)
} registers_t;

/**
 * @brief 任務控制區塊 (Process Control Block, PCB)
 */
typedef struct task {
    uint32_t esp;               ///< 核心堆疊指標

    // === 行程身分資訊 ===
    uint32_t pid;               ///< 行程 ID
    uint32_t ppid;              ///< 父行程 ID
    char name[32];              ///< 行程名稱

    uint32_t total_ticks;       ///< 行程總執行時間 (Ticks)

    uint32_t page_directory;    ///< 專屬的分頁目錄實體位址 (CR3)
    uint32_t kernel_stack;      ///< 核心堆疊頂部

    uint32_t state;             ///< 行程當前狀態
    uint32_t wait_pid;          ///< 正在等待的子行程 PID

    uint32_t heap_start;        ///< Heap 初始起點
    uint32_t heap_end;          ///< User Heap 的當前頂點
    uint32_t cwd_lba;           ///< 當前工作目錄 (CWD) 的 LBA

    uint32_t wake_up_tick;      /// for sleep, 預計醒來的 tick 數

    struct task *next;          ///< 環狀鏈結串列指標
} task_t;

/**
 * @brief 用於傳遞給 User Space 的行程資訊
 */
typedef struct {
    uint32_t pid;
    uint32_t ppid;
    char name[32];
    uint32_t state;
    uint32_t memory_used; ///< 佔用記憶體大小 (Bytes)
    uint32_t total_ticks; ///< 總執行時間
} process_info_t;

extern volatile task_t *current_task;

/** @brief 初始化多工排程子系統 */
void init_multitasking();

/**
 * @brief 建立第一個 User Task
 * @param entry_point 程式進入點
 * @param user_stack_top User Stack 頂部
 */
void create_user_task(uint32_t entry_point, uint32_t user_stack_top);

/** @brief 結束當前行程 */
void exit_task();

/** @brief 排程器 (Round Robin) 切換下一個行程 */
void schedule();

/**
 * @brief 複製當前行程 (Fork)
 * @param regs 當前的暫存器狀態
 * @return 子行程回傳 0，父行程回傳子行程的 PID，失敗回傳 -1
 */
int sys_fork(registers_t *regs);

/**
 * @brief 替換當前行程的執行檔影像 (Exec)
 * @param regs 包含新執行檔名稱與參數的暫存器
 * @return 成功不回傳，失敗回傳 -1
 */
int sys_exec(registers_t *regs);

/**
 * @brief 等待子行程結束
 * @param pid 要等待的子行程 PID
 * @return 成功回傳 0，失敗回傳 -1
 */
int sys_wait(uint32_t pid);

/**
 * @brief 強制終止行程
 * @param pid 要終止的行程 PID
 * @return 成功回傳 0，找不到行程回傳 -1
 */
int sys_kill(uint32_t pid);

int sys_sleep(uint32_t ms);

/**
 * @brief 取得系統中所有活躍的行程清單
 * @param list 用於儲存結果的陣列
 * @param max_count 陣列最大容量
 * @return 實際取回的行程數量
 */
int task_get_process_list(process_info_t* list, int max_count);

#endif
