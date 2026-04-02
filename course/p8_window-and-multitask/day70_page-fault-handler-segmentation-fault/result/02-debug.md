
順利編譯後，進入 desktop mode，在 terminal 輸入 `paint &` 出現 page_fault_handler 處理訊息
然後繼續輸入 `segfault` 後，卻出現了 **System Status** Window
接下來就動不了了 ... 點 [x] 也不會動, 左下角的 **Start** 也沒反應.
看起來整個是卡住了 ...

我整理可能有問題的 code：

---
interrupts.S
```asm
; [目的] 中斷服務例程 (ISR) 總匯與 IDT 載入
; [流程 - 以 System Call 0x128 為例]
; User App 執行 int 0x128 (eax=功能號)
;    |
;    V
; [ isr128 ]
;    |--> [ pusha ] (將 eax, ebx... 等暫存器快照壓入堆疊)
;    |--> [ push esp ] (將指向 registers_t 的指標傳給 C 函式)
;    |--> [ call syscall_handler ]
;    |      |--> C 語言修改 regs->eax 作為回傳值
;    |--> [ add esp, 4 ] (清理參數)
;    |--> [ popa ] (此時 eax 已變更為回傳值)
;    |--> [ iret ] (返回 User Mode)

global idt_flush

global isr0
extern isr0_handler

; [day70] Page Fault
global isr14
extern page_fault_handler

global isr32
extern timer_handler

global isr33
extern keyboard_handler

global isr44
extern mouse_handler

global isr128
extern syscall_handler

idt_flush:
    mov eax, [esp+4]
    lidt [eax]
    ret

isr0:
    cli
    push 0          ; [Day70] add 動推入假錯誤碼
    call isr0_handler
    add esp, 4      ; [day70] add 清除假錯誤碼
    sti
    iret

; 第 14 號中斷：Page Fault
isr14:
    ; 💡 注意：Page Fault 時，CPU「已經」自動 push error code 了！所以這裡不需要 push 0
    pusha           ; 壓入 eax, ecx, edx, ebx, esp, ebp, esi, edi
    push esp        ; 傳遞 registers_t 指標
    call page_fault_handler
    add esp, 4      ; 清除 registers_t 指標參數
    popa            ; 恢復通用暫存器
    add esp, 4      ; 【重要】丟棄 CPU 壓入的 Error Code！
    iret

isr32:
    push 0          ; 【Day 70 修復】手動推入假錯誤碼
    pusha
    call timer_handler
    popa
    add esp, 4      ; 【Day 70 修復】清除假錯誤碼
    iret

isr33:
    push 0          ; 【Day 70 修復】
    pusha
    call keyboard_handler
    popa
    add esp, 4      ; 【Day 70 修復】
    iret

isr44:
    push 0          ; 【Day 70 修復】
    pusha
    call mouse_handler
    popa
    add esp, 4      ; 【Day 70 修復】
    iret

; 第 128 號中斷 (System Call 窗口)
isr128:
    push 0          ; 【Day 70 修復】手動推入假錯誤碼，對齊 registers_t！
    pusha           ; 備份通用暫存器 (32 bytes)
    push esp        ; 把 registers_t 的指標傳給 C 語言
    call syscall_handler
    add esp, 4      ; 清除 esp 參數
    popa            ; 恢復所有暫存器
    add esp, 4      ; 【Day 70 修復】清除假錯誤碼
    iret
```
---
task.h
```c
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

    /* 【Day 70 新增】由 CPU 在某些例外發生時自動壓入的錯誤碼 */
    /* 如果是一般中斷，我們會在 ASM 裡手動補一個 0，確保結構對齊 */
    uint32_t err_code;

    /* 以下由 CPU 在發生中斷時「自動」壓入堆疊 (Interrupt Stack Frame) */
    uint32_t eip;    // Instruction Pointer: 程式被中斷時的下一條指令位址
    uint32_t cs;     // Code Segment: 包含權限等級 (CPL)，決定是 Ring 0 還是 3
    uint32_t eflags; // CPU Flags: 包含中斷開關 (IF) 與運算狀態標誌

    /* 以下兩項僅在「權限切換」(Ring 3 -> Ring 0) 時由 CPU 自動壓入 */
    uint32_t user_esp; // User Stack Pointer: 使用者原本的堆疊位址
    uint32_t user_ss;  // User Stack Segment: 使用者原本的堆疊段選擇子 (通常 0x23)
} registers_t;


typedef struct task {
    uint32_t esp;

    // ==========================================
    // 【Day 62 修改與新增】行程身分資訊 (PCB 擴充)
    // ==========================================
    uint32_t pid;               // 將原本的 id 改名為 pid (更符合 OS 慣例)
    uint32_t ppid;              // 【新增】父行程 ID (Parent PID)
    char name[32];              // 【新增】行程名稱 (例如 "shell.elf")

    // 【Day 65 新增】記錄這個行程總共跑了多少個 Ticks
    uint32_t total_ticks;

    uint32_t page_directory;    // 這個 Task 專屬的分頁目錄實體位址 (CR3)
    uint32_t kernel_stack;

    uint32_t state;
    uint32_t wait_pid;          // 保留！這是你 sys_wait 的核心機制

    uint32_t heap_start;        // 【新增】記錄 Heap 初始起點 (未來給 top 計算記憶體佔用量用)
    uint32_t heap_end;          // 記錄 User Heap 的當前頂點
    uint32_t cwd_lba;           // 當前工作目錄 (Current Working Directory, CWD) 的 LBA
    struct task *next;
} task_t;

// 【Day 63 新增】用於傳遞給 User Space 的行程資訊
typedef struct {
    uint32_t pid;
    uint32_t ppid;
    char name[32];
    uint32_t state;
    uint32_t memory_used; // 佔用記憶體大小 (Bytes)
    uint32_t total_ticks; // 【Day 65 新增】
} process_info_t;



extern volatile task_t *current_task;

void init_multitasking();

void create_user_task(uint32_t entry_point, uint32_t user_stack_top);
void exit_task();
void schedule();
int sys_fork(registers_t *regs);
int sys_exec(registers_t *regs);
int sys_wait(int pid);

int task_get_process_list(process_info_t* list, int max_count);


#endif
```

---
idt.c
```c
// IDT: Interrupt Descriptor Table
// Intel 8259: https://zh.wikipedia.org/zh-tw/Intel_8259
// PIC: [Programmable Interrupt Controller](https://en.wikipedia.org/wiki/Programmable_interrupt_controller)
#include "idt.h"
#include "io.h"

extern void kprintf(const char* format, ...);

// 宣告一個長度為 256 的 IDT 陣列
idt_entry_t idt_entries[256];
idt_ptr_t   idt_ptr;

// 外部組合語言函式：載入 IDT 與中斷處理入口
extern void idt_flush(uint32_t);
extern void isr0();     // 第 0  號中斷：的 Assembly 進入點
extern void isr14();    // 第 14 號中斷：Page Fault
extern void isr32();    // 第 32 號中斷：Timer IRQ 0
extern void isr33();    // 第 33 號中斷：宣告組合語言的鍵盤跳板
extern void isr44();    // 第 44 號中斷：Mouse Handler
extern void isr128();   // 第 128 號中斷：system calls

// 設定單一 IDT 條目的輔助函式
static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].base_lo = base & 0xFFFF;
    idt_entries[num].base_hi = (base >> 16) & 0xFFFF;
    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    // flags 0x8E 代表：這是一個 32-bit 的中斷閘 (Interrupt Gate)，運行在 Ring 0，且此條目有效(Present)
    idt_entries[num].flags   = flags;
}


// --- 公開 API ---

void init_idt(void) {
    idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
    idt_ptr.base  = (uint32_t)&idt_entries;

    // 先把 256 個中斷全部清空 (避免指到未知的記憶體)
    // 這裡我們簡單用迴圈清零 (你也可以 include 昨天寫的 memset)
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    // 掛載第 0 號中斷：除以零
    // 0x08 是我們昨天在 GDT 設定的 Kernel Code Segment
    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);

    // 【Day 70 新增】掛載第 14 號中斷：Page Fault
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);


    // 重新映射 PIC
    pic_remap();

    // 掛載第 32 號中斷 (IRQ0: Timer)
    idt_set_gate(32, (uint32_t)isr32, 0x08, 0x8E);

    // 掛載第 33 號中斷 (IRQ1: Keyboard)
    idt_set_gate(33, (uint32_t)isr33, 0x08, 0x8E);

    // 掛載第 44 號中斷 (IRQ1: Mouse)
    idt_set_gate(44, (uint32_t)isr44, 0x08, 0x8E);

    // 掛載第 128 號中斷 (System Call)
    // 注意！旗標是 0xEE (允許 Ring 3 呼叫)
    idt_set_gate(128, (uint32_t)isr128, 0x08, 0xEE);

    // 呼叫組合語言，正式套用新的 IDT
    idt_flush((uint32_t)&idt_ptr);
}


// ---

// 這是當「除以零」發生時，實際會執行的 C 語言函式
void isr0_handler(void) {
    kprintf("\n[KERNEL PANIC] Exception 0: Divide by Zero!\n");
    kprintf("System Halted.\n");
    // 發生嚴重錯誤，直接把系統凍結
    __asm__ volatile ("cli; hlt");
}

// 初始化 8259 PIC，將 IRQ 0~15 重映射到 IDT 的 32~47
// Intel 8259: https://zh.wikipedia.org/zh-tw/Intel_8259
void pic_remap() {
    uint8_t a1 = inb(0x21);
    uint8_t a2 = inb(0xA1);
    (void)a1; (void)a2;

    // ICW1: 開始初始化序列
    outb(0x20, 0x11);
    outb(0xA0, 0x11);

    // ICW2: 設定 Master PIC 偏移量為 0x20 (32)
    outb(0x21, 0x20);
    // ICW2: 設定 Slave PIC 偏移量為 0x28 (40)
    outb(0xA1, 0x28);

    // ICW3: Master 告訴 Slave 接在 IRQ2
    outb(0x21, 0x04);
    // ICW3: Slave 確認身分
    outb(0xA1, 0x02);

    // ICW4: 設定為 8086 模式
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    // ==========================================================
    // 遮罩 (Masks)：0 代表開啟，1 代表屏蔽
    // 為了讓 IRQ 12 (滑鼠) 通過，我們必須：
    // 1. 開啟 Master PIC 的 IRQ 0(Timer), 1(Keyboard), 2(Slave連線) -> 1111 1000 = 0xF8
    // 2. 開啟 Slave PIC 的 IRQ 12 (第 4 個 bit) -> 1110 1111 = 0xEF
    // ==========================================================
    outb(0x21, 0xF8);
    outb(0xA1, 0xEF);
}
```

---
paging.c
```c
#include <stdint.h>
#include <stddef.h>
#include "paging.h"
#include "utils.h"
#include "pmm.h"
#include "task.h" // 確保有引入 task.h 才能知道 current_task

uint32_t page_directory[1024] __attribute__((aligned(4096)));

uint32_t first_page_table[1024] __attribute__((aligned(4096)));
uint32_t second_page_table[1024] __attribute__((aligned(4096)));
uint32_t third_page_table[1024] __attribute__((aligned(4096)));
// user page
uint32_t user_page_table[1024] __attribute__((aligned(4096)));
uint32_t user_heap_page_table[1024] __attribute__((aligned(4096)));
// vram page
uint32_t vram_page_table[1024] __attribute__((aligned(4096)));

// ====================================================================
// 預先分配 16 個宇宙的空間！
// ====================================================================
uint32_t universe_pds[16][1024] __attribute__((aligned(4096)));
uint32_t universe_pts[16][1024] __attribute__((aligned(4096)));
uint32_t universe_heap_pts[16][1024] __attribute__((aligned(4096)));    // 平行宇宙的 Heap 表陣列

int next_universe_id = 0;

// 在全域變數區新增一個陣列，記錄哪個宇宙被使用了
int universe_used[16] = {0};

extern void load_page_directory(uint32_t*);
extern void enable_paging(void);

void init_paging(void) {
    for(int i = 0; i < 1024; i++) { page_directory[i] = 0x00000002; }
    for(int i = 0; i < 1024; i++) { first_page_table[i] = (i * 0x1000) | 7; }
    for(int i = 0; i < 1024; i++) { second_page_table[i] = 0; }
    for(int i = 0; i < 1024; i++) { third_page_table[i] = 0; }
    for(int i = 0; i < 1024; i++) { user_page_table[i] = 0; }
    for(int i = 0; i < 1024; i++) { user_heap_page_table[i] = 0; } // 清空

    page_directory[0] = ((uint32_t)first_page_table) | 7;
    page_directory[32] = ((uint32_t)user_page_table) | 7;

    // [掛載 0x10000000 區域 (pd_idx = 64)
    page_directory[64] = ((uint32_t)user_heap_page_table) | 7;

    page_directory[512] = ((uint32_t)second_page_table) | 3;
    page_directory[768] = ((uint32_t)third_page_table) | 3;

    load_page_directory(page_directory);
    enable_paging();
}

void map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x03FF;

    uint32_t* page_table;

    if (pd_idx == 0) {
        page_table = first_page_table;
    } else if (pd_idx == 32) {
        // Stack & Code 區 (0x08000000)
        uint32_t cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
        uint32_t* current_pd = (uint32_t*) cr3;
        uint32_t pt_entry = current_pd[32];
        if (pt_entry & 1) {
            page_table = (uint32_t*)(pt_entry & 0xFFFFF000);
        } else {
            kprintf("Error: user page table not present in current CR3!\n");
            return;
        }
    } else if (pd_idx == 64) {
        // =========================================================
        // 【Heap 區】 (0x10000000)
        // =========================================================
        uint32_t cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
        uint32_t* current_pd = (uint32_t*) cr3;
        uint32_t pt_entry = current_pd[64]; // 檢查第 64 個目錄項

        if (pt_entry & 1) {
            page_table = (uint32_t*)(pt_entry & 0xFFFFF000);
        } else {
            kprintf("Error: heap page table not present in current CR3!\n");
            return;
        }
    } else if (pd_idx == 512) {
        page_table = second_page_table;
    } else if (pd_idx == 768) {
        page_table = third_page_table;
    } else {
        kprintf("Error: Page table not allocated for pd_idx [%d]!\n", pd_idx);
        return;
    }

    page_table[pt_idx] = (phys & 0xFFFFF000) | flags;
    __asm__ volatile("invlpg (%0)" ::"r" (virt) : "memory");
}

uint32_t create_page_directory() {

    int id = -1;
    // 尋找空閒的宇宙
    for (int i = 0; i < 16; i++) {
        if (!universe_used[i]) { id = i; break; }
    }
    if (id == -1) {
        kprintf("Error: Max universes reached!\n");
        while(1) __asm__ volatile("hlt");
    }

    universe_used[id] = 1; // 標記為使用中

    uint32_t* new_pd = universe_pds[id];
    uint32_t* new_pt = universe_pts[id];
    uint32_t* new_heap_pt = universe_heap_pts[id]; // 拿出這個宇宙專屬的 Heap 表

    for(int i = 0; i < 1024; i++) {
        new_pd[i] = page_directory[i];
    }

    for(int i = 0; i < 1024; i++) {
        new_pt[i] = 0;
        new_heap_pt[i] = 0; // 初始化清空
    }

    new_pd[32] = ((uint32_t)new_pt) | 7;
    // 將這張全新的 Heap 表掛載到新宇宙的 0x10000000 區段
    new_pd[64] = ((uint32_t)new_heap_pt) | 7;

    return (uint32_t)new_pd;
}


// 提供給 sys_exit 呼叫的回收函式
// 提供給 sys_exit 呼叫的回收函式
void free_page_directory(uint32_t pd_phys) {
    for (int i = 0; i < 16; i++) {
        if ((uint32_t)universe_pds[i] == pd_phys) {

            // 1. 釋放 Code & Stack 區段的物理分頁
            for (int j = 0; j < 1024; j++) {
                if (universe_pts[i][j] & 1) { // 檢查 Present Bit 是否為 1
                    uint32_t phys_addr = universe_pts[i][j] & 0xFFFFF000;
                    pmm_free_page((void*)phys_addr); // 還給實體記憶體管理器！
                    universe_pts[i][j] = 0;          // 清空分頁表紀錄
                }
            }

            // 2. 釋放 Heap 區段的物理分頁
            for (int j = 0; j < 1024; j++) {
                if (universe_heap_pts[i][j] & 1) {
                    uint32_t phys_addr = universe_heap_pts[i][j] & 0xFFFFF000;
                    pmm_free_page((void*)phys_addr);
                    universe_heap_pts[i][j] = 0;
                }
            }

            universe_used[i] = 0; // 解除佔用，讓給下一個程式！
            return;
        }
    }
}

// 專門用來映射 Framebuffer (MMIO, Memory-Mapped I/O) 的安全函數
void map_vram(uint32_t virt, uint32_t phys) {
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x03FF;

    if ((page_directory[pd_idx] & 1) == 0) {
        uint32_t pt_phys = (uint32_t)vram_page_table;
        for (int i = 0; i < 1024; i++) vram_page_table[i] = 0;
        page_directory[pd_idx] = pt_phys | 7;
    }
    vram_page_table[pt_idx] = phys | 7;
}

// 【新增】取得活躍的 Paging 宇宙數量
uint32_t paging_get_active_universes(void) {
    uint32_t count = 0;
    for (int i = 0; i < 16; i++) {
        if (universe_used[i]) count++;
    }
    return count;
}




// 【Day 70 新增】Page Fault 專屬的攔截與處理中心
void page_fault_handler(registers_t *regs) {
    uint32_t faulting_address;
    __asm__ volatile("mov %%cr2, %0" : "=r" (faulting_address));

    // 檢查錯誤碼的 bit 0：0 代表頁面不存在，1 代表權限違規
    int present = !(regs->err_code & 0x1);
    int rw = regs->err_code & 0x2;           // 0 為讀取，1 為寫入
    int us = regs->err_code & 0x4;           // 0 為核心模式，1 為使用者模式

    if (us) {
        kprintf("\n[Kernel] Segmentation Fault at 0x%x!\n", faulting_address);
        kprintf("  -> Cause: %s in %s mode\n",
                present ? "Page not present" : "Protection violation",
                rw ? "Write" : "Read");

        // 殺掉兇手！
        extern int sys_kill(int pid);
        sys_kill(current_task->pid);

        // 切換任務，永不回頭
        schedule();
    } else {
        // Kernel 自己崩潰了
        kprintf("\nKERNEL PANIC: Page Fault at 0x%x\n", faulting_address);
        while(1) __asm__ volatile("cli; hlt");
    }
}
```
---
task.c
```c
#include <stdint.h>
#include "task.h"
#include "pmm.h"
#include "kheap.h"
#include "utils.h"
#include "tty.h"
#include "gdt.h"
#include "paging.h"
#include "simplefs.h"
#include "vfs.h"
#include "elf.h"

volatile task_t *current_task = 0;
volatile task_t *ready_queue = 0;
uint32_t next_pid = 0; // 【Day 62 修改】將 next_task_id 改名為 next_pid
task_t *idle_task = 0;

extern uint32_t page_directory[];
extern void switch_task(uint32_t *current_esp, uint32_t *next_esp, uint32_t next_cr3);  // switch_task.S
extern void child_ret_stub();   // switch_task.S

// --- Internal API ----

void idle_loop() {
    while(1) { __asm__ volatile("sti; hlt"); }
}

// ---- Public API ----

void init_multitasking() {
    task_t *main_task = (task_t*) kmalloc(sizeof(task_t));
    main_task->pid = next_pid++; // 【Day 62 修改】
    main_task->ppid = 0;         // 【Day 62 新增】
    strcpy(main_task->name, "[kernel_main]"); // 【Day 62 新增】

    main_task->esp = 0;
    main_task->kernel_stack = 0;
    main_task->state = TASK_RUNNING;
    main_task->wait_pid = 0;
    main_task->page_directory = (uint32_t) page_directory;
    main_task->cwd_lba = 0;
    main_task->total_ticks = 0;

    main_task->next = main_task;
    current_task = main_task;
    ready_queue = main_task;

    idle_task = (task_t*) kmalloc(sizeof(task_t));
    idle_task->pid = 9999;       // 【Day 62 修改】
    idle_task->ppid = 0;         // 【Day 62 新增】
    strcpy(idle_task->name, "[kernel_idle]"); // 【Day 62 新增】
    idle_task->state = TASK_RUNNING;
    idle_task->wait_pid = 0;
    idle_task->page_directory = (uint32_t) page_directory;
    idle_task->cwd_lba = 0;
    idle_task->total_ticks = 0;

    uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
    idle_task->kernel_stack = (uint32_t) kstack;

    *(--kstack) = (uint32_t) idle_loop;
    for(int i = 0; i < 8; i++) *(--kstack) = 0;
    *(--kstack) = 0x0202;

    idle_task->esp = (uint32_t) kstack;
}

void create_user_task(uint32_t entry_point, uint32_t user_stack_top) {
    task_t *new_task = (task_t*) kmalloc(sizeof(task_t));
    new_task->pid = next_pid++; // 【Day 62 修改】
    new_task->ppid = current_task->pid; // 【Day 62 新增】
    strcpy(new_task->name, "init");     // 【Day 62 新增】初代程式命名為 init

    new_task->state = TASK_RUNNING;
    new_task->wait_pid = 0;
    new_task->page_directory = current_task->page_directory;
    new_task->cwd_lba = 0;
    new_task->total_ticks = 0;

    // 為初代老爸 (Shell) 預先分配 10 個實體分頁給 User Heap
    // 預先分配 256 個實體分頁 (1MB) 給 User Heap
    for (int i = 0; i < 256; i++) {
        uint32_t heap_phys = pmm_alloc_page();
        map_page(0x10000000 + (i * 4096), heap_phys, 7);
    }
    new_task->heap_start = 0x10000000; // 【Day 62 新增】
    new_task->heap_end = 0x10000000;

    uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
    new_task->kernel_stack = (uint32_t) kstack;

    uint32_t *ustack = (uint32_t*) (user_stack_top - 64);
    *(--ustack) = 0;
    uint32_t argv_ptr = (uint32_t) ustack;
    *(--ustack) = argv_ptr;
    *(--ustack) = 0;
    *(--ustack) = 0;

    *(--kstack) = 0x23;
    *(--kstack) = (uint32_t)ustack;
    *(--kstack) = 0x0202;
    *(--kstack) = 0x1B;
    *(--kstack) = entry_point;

    *(--kstack) = 0;
    *(--kstack) = 0;
    *(--kstack) = 0;
    *(--kstack) = 0;

    *(--kstack) = (uint32_t) child_ret_stub;

    for(int i = 0; i < 8; i++) *(--kstack) = 0;
    *(--kstack) = 0x0202;

    new_task->esp = (uint32_t) kstack;
    new_task->next = current_task->next;
    current_task->next = new_task;
}

void exit_task() {
    task_t *temp = current_task->next;
    while (temp != current_task) {
        // 【Day 62 修改】改用 pid 判斷
        if (temp->state == TASK_WAITING && temp->wait_pid == current_task->pid) {
            temp->state = TASK_RUNNING;
            temp->wait_pid = 0;
        }
        temp = temp->next;
    }

    if (current_task->page_directory != (uint32_t)page_directory) {
        free_page_directory(current_task->page_directory);
    }

    // 【修改】如果沒有老爸 (ppid == 0)，就直接 DEAD，否則變 ZOMBIE
    if (current_task->ppid == 0) {
        current_task->state = TASK_DEAD;
    } else {
        current_task->state = TASK_ZOMBIE;
    }

    schedule();
}

void schedule() {
    if (!current_task) return;

    task_t *curr = (task_t*)current_task;
    task_t *next_node = curr->next;

    // 尋找並剔除 DEAD 的任務
    while (next_node != current_task) {
        if (next_node->state == TASK_DEAD) {
            // 1. 將它從 Circular Linked List 拔除
            curr->next = next_node->next;

            // ==========================================
            // 【Day 67 核心新增】釋放記憶體 (Garbage Collection)
            // ==========================================
            if (next_node->kernel_stack != 0) {
                // 還記得我們當初是 kmalloc(4096)，然後把指標 +4096 嗎？
                // 現在要減回去才能正確 free！
                kfree((void*)(next_node->kernel_stack - 4096));
            }
            // 釋放 PCB 結構體本身
            kfree((void*)next_node);

            // 往下看下一個節點
            next_node = curr->next;
        } else {
            // 只有不是 DEAD 的任務，才繼續往下走
            curr = next_node;
            next_node = curr->next;
        }
    }

    task_t *next_run = current_task->next;
    while (next_run->state != TASK_RUNNING && next_run != current_task) {
        next_run = next_run->next;
    }

    if (next_run->state != TASK_RUNNING) {
        next_run = idle_task;
    }

    task_t *prev = (task_t*)current_task;
    current_task = next_run;

    if (current_task != prev) {
        if (current_task->kernel_stack != 0) {
            set_kernel_stack(current_task->kernel_stack);
        }
        switch_task(&prev->esp, &current_task->esp, current_task->page_directory);
    }
}

int sys_fork(registers_t *regs) {
    task_t *child = (task_t*) kmalloc(sizeof(task_t));
    child->pid = next_pid++;                 // 【Day 62 修改】
    child->ppid = current_task->pid;         // 【Day 62 新增】
    strcpy(child->name, current_task->name); // 【Day 62 新增】繼承老爸的名字

    child->state = TASK_RUNNING;
    child->wait_pid = 0;
    child->page_directory = current_task->page_directory;
    child->cwd_lba = current_task->cwd_lba;

    child->heap_start = current_task->heap_start; // 【Day 62 新增】
    child->heap_end = current_task->heap_end;
    child->total_ticks = 0; // 子行程出生時，努力值當然要歸零重新計算！

    uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
    child->kernel_stack = (uint32_t) kstack;

    // 【Day 62 修改】改用 child->pid 確保位址不衝突
    uint32_t child_ustack_base = 0x083FF000 - (child->pid * 4096);
    uint32_t child_ustack_phys = pmm_alloc_page();
    map_page(child_ustack_base, child_ustack_phys, 7);

    uint32_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3));

    uint32_t parent_ustack_base = regs->user_esp & 0xFFFFF000;
    uint8_t *src = (uint8_t*) parent_ustack_base;
    uint8_t *dst = (uint8_t*) child_ustack_base;
    for(int i = 0; i < 4096; i++) { dst[i] = src[i]; }

    int32_t delta = child_ustack_base - parent_ustack_base;
    uint32_t child_user_esp = regs->user_esp + delta;

    uint32_t child_ebp = regs->ebp;
    if (child_ebp >= parent_ustack_base && child_ebp < parent_ustack_base + 4096) {
        child_ebp += delta;
    }

    uint32_t curr_ebp = child_ebp;
    while (curr_ebp >= child_ustack_base && curr_ebp < child_ustack_base + 4096) {
        uint32_t *ebp_ptr = (uint32_t*) curr_ebp;
        uint32_t next_ebp = *ebp_ptr;
        if (next_ebp >= parent_ustack_base && next_ebp < parent_ustack_base + 4096) {
            *ebp_ptr = next_ebp + delta;
            curr_ebp = *ebp_ptr;
        } else {
            break;
        }
    }

    *(--kstack) = regs->user_ss;
    *(--kstack) = child_user_esp;
    *(--kstack) = regs->eflags;
    *(--kstack) = regs->cs;
    *(--kstack) = regs->eip;

    *(--kstack) = child_ebp;
    *(--kstack) = regs->ebx;
    *(--kstack) = regs->esi;
    *(--kstack) = regs->edi;

    *(--kstack) = (uint32_t) child_ret_stub;

    for(int i = 0; i < 8; i++) *(--kstack) = 0;
    *(--kstack) = 0x0202;

    child->esp = (uint32_t) kstack;
    child->next = current_task->next;
    current_task->next = child;

    return child->pid; // 【Day 62 修改】回傳 child 的 PID
}

int sys_exec(registers_t *regs) {
    char* filename = (char*)regs->ebx;
    char** old_argv = (char**)regs->ecx;

    uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;
    fs_node_t* file_node = simplefs_find(current_dir, filename);

    if (file_node == 0 && current_dir != 1) {
        file_node = simplefs_find(1, filename);
    }
    if (file_node == 0) { return -1; }

    // ==========================================
    // 【Day 62 新增】靈魂轉移：更新行程名稱！
    // ==========================================
    strcpy(current_task->name, filename);

    uint8_t* buffer = (uint8_t*) kmalloc(file_node->length);
    vfs_read(file_node, 0, file_node->length, buffer);

    int argc = 0;
    char* safe_argv[16];

    if (old_argv != 0 && (uint32_t)old_argv > 0x08000000) {
        while (old_argv[argc] != 0 && argc < 15) {
            int len = strlen(old_argv[argc]) + 1;
            safe_argv[argc] = (char*) kmalloc(len);
            memcpy(safe_argv[argc], old_argv[argc], len);
            argc++;
        }
    }
    safe_argv[argc] = 0;

    uint32_t new_cr3 = create_page_directory();
    uint32_t old_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(old_cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(new_cr3));

    uint32_t entry_point = elf_load((elf32_ehdr_t*)buffer);
    kfree(buffer);

    if (entry_point == 0) {
        __asm__ volatile("mov %0, %%cr3" : : "r"(old_cr3));
        return -1;
    }

    current_task->page_directory = new_cr3;

    uint32_t clean_user_stack_top = 0x083FF000 + 4096;
    uint32_t ustack_phys = pmm_alloc_page();
    map_page(0x083FF000, ustack_phys, 7);

    // 預先分配 256 個實體分頁 (1MB) 給 User Heap
    for (int i = 0; i < 256; i++) {
        uint32_t heap_phys = pmm_alloc_page();
        map_page(0x10000000 + (i * 4096), heap_phys, 7);
    }
    current_task->heap_start = 0x10000000; // 【Day 62 新增】
    current_task->heap_end = 0x10000000;

    uint32_t stack_ptr = clean_user_stack_top - 64;

    uint32_t argv_ptrs[16] = {0};
    if (argc > 0) {
        for (int i = argc - 1; i >= 0; i--) {
            int len = strlen(safe_argv[i]) + 1;
            stack_ptr -= len;
            memcpy((void*)stack_ptr, safe_argv[i], len);
            argv_ptrs[i] = stack_ptr;
            kfree(safe_argv[i]);
        }
        stack_ptr = stack_ptr & ~3;
        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = 0;
        for (int i = argc - 1; i >= 0; i--) {
            stack_ptr -= 4;
            *(uint32_t*)stack_ptr = argv_ptrs[i];
        }
        uint32_t argv_base = stack_ptr;
        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = argv_base;
        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = argc;
    } else {
        stack_ptr -= 4; *(uint32_t*)stack_ptr = 0;
        stack_ptr -= 4; *(uint32_t*)stack_ptr = 0;
    }

    stack_ptr -= 4; *(uint32_t*)stack_ptr = 0;

    regs->eip = entry_point;
    regs->user_esp = stack_ptr;

    return 0;
}

int sys_wait(int pid) {
    task_t *temp = current_task->next;
    int found = 0;

    // 1. 先檢查兒子是不是「已經是殭屍」了？
    while (temp != current_task) {
        if (temp->pid == (uint32_t)pid) {
            found = 1;
            if (temp->state == TASK_ZOMBIE) {
                // 兒子早就死了！直接收屍並返回，不用睡覺！
                temp->state = TASK_DEAD;
                return 0;
            }
            break;
        }
        temp = temp->next;
    }

    if (!found) return -1; // 根本沒有這個兒子

    // 2. 兒子還活著，老爸只好乖乖睡覺等待
    current_task->wait_pid = pid;
    current_task->state = TASK_WAITING;
    schedule();

    // 3. 【Day 67 新增】老爸被喚醒了！代表兒子剛死，趕快去收屍！
    temp = current_task->next;
    while (temp != current_task) {
        if (temp->pid == (uint32_t)pid && temp->state == TASK_ZOMBIE) {
            temp->state = TASK_DEAD; // 正式宣告死亡！
            break;
        }
        temp = temp->next;
    }

    return 0;
}

// 【Day 66 新增】終結指定的行程
int sys_kill(int pid) {
    if (pid <= 1) return -1; // 防禦機制：絕對不准殺 Kernel Idle (0) 和 Kernel Main (1)！

    __asm__ volatile("cli");
    task_t *temp = (task_t*)current_task;
    int found = 0;

    // 遍歷所有行程尋找目標
    do {
        if (temp->pid == (uint32_t)pid && temp->state != TASK_DEAD) {
            // 【Day 67 修改】變成殭屍
            temp->state = TASK_ZOMBIE;

            // 【Day 67 新增】如果老爸正在等它死，我們要順便把老爸叫醒！
            task_t *parent = current_task;
            do {
                if (parent->pid == temp->ppid && parent->state == TASK_WAITING && parent->wait_pid == pid) {
                    parent->state = TASK_RUNNING;
                    parent->wait_pid = 0;
                }
                parent = parent->next;
            } while (parent != current_task);

            found = 1;
            break;
        }
        temp = temp->next;
    } while (temp != current_task);

    __asm__ volatile("sti");

    return found ? 0 : -1;
}

// 【Day 63 新增】收集所有行程資料
int task_get_process_list(process_info_t* list, int max_count) {
    if (!current_task) return 0;

    int count = 0;
    task_t* temp = (task_t*)current_task;

    // 遍歷 Circular Linked List
    do {
        // 不要回報已經徹底死亡的行程
        if (temp->state != TASK_DEAD) {
            list[count].pid = temp->pid;
            list[count].ppid = temp->ppid;
            // 【修改】使用 strncpy 防止越界，最大長度 32
            // strcpy(list[count].name, temp->name);
            strncpy(list[count].name, temp->name, 32);
            list[count].state = temp->state;

            // 在迴圈內加入這一行拷貝邏輯
            list[count].total_ticks = temp->total_ticks;

            // 計算 Heap 動態佔用的空間 (End - Start)
            if (temp->heap_end >= temp->heap_start) {
                // User Task: 動態 Heap + 預先分配的 1MB Heap + 4KB User Stack
                list[count].memory_used = (temp->heap_end - temp->heap_start) + (256 * 4096) + 4096;
            } else {
                // Kernel Task: 只有 Kernel Stack (4KB)
                list[count].memory_used = 4096;
            }

            count++;
        }
        temp = temp->next;
    } while (temp != current_task && count < max_count);

    return count; // 回傳總共找到了幾個行程
}
```
---
pmm.c
```c
// PMM: Physical Memory Management
#include "pmm.h"
#include "utils.h"
#include "tty.h"

// 假設我們最多支援 4GB RAM (總共 1,048,576 個 Frames)
// 1048576 bits / 32 bits (一個 uint32_t) = 32768 個陣列元素
#define BITMAP_SIZE 32768
uint32_t memory_bitmap[BITMAP_SIZE];

uint32_t max_frames = 0; // 系統實際擁有的可用 Frame 數量

// --- 內部輔助函式：操作特定的 Bit ---

// 將第 frame_idx 個 bit 設為 1 (代表已使用)
static void bitmap_set(uint32_t frame_idx) {
    uint32_t array_idx = frame_idx / 32;
    uint32_t bit_offset = frame_idx % 32;
    memory_bitmap[array_idx] |= (1 << bit_offset);
}

// 將第 frame_idx 個 bit 設為 0 (代表釋放)
static void bitmap_clear(uint32_t frame_idx) {
    uint32_t array_idx = frame_idx / 32;
    uint32_t bit_offset = frame_idx % 32;
    memory_bitmap[array_idx] &= ~(1 << bit_offset);
}

// 檢查第 frame_idx 個 bit 是否為 1
static bool bitmap_test(uint32_t frame_idx) {
    uint32_t array_idx = frame_idx / 32;
    uint32_t bit_offset = frame_idx % 32;
    return (memory_bitmap[array_idx] & (1 << bit_offset)) != 0;
}

// 尋找第一個為 0 的 bit (第一塊空地)
static int bitmap_find_first_free() {
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        // 如果這個整數不是 0xFFFFFFFF (代表裡面至少有一個 bit 是 0)
        if (memory_bitmap[i] != 0xFFFFFFFF) {
            for (uint32_t j = 0; j < 32; j++) {
                uint32_t frame_idx = i * 32 + j;
                if (frame_idx >= max_frames) return -1; // 已經超出實際記憶體容量

                if (!bitmap_test(frame_idx)) {
                    return frame_idx;
                }
            }
        }
    }
    return -1; // 記憶體全滿 (Out of Memory)
}

// --- 公開 API ---

void init_pmm(uint32_t mem_size_kb) {
    // 總 KB 數 / 4 = 總共有多少個 4KB 的 Frames
    max_frames = mem_size_kb / 4;

    // 一開始先把所有的記憶體都標記為「已使用」(填滿 1)
    // 這是為了安全，避免我們不小心分配到不存在的硬體空間
    memset(memory_bitmap, 0xFF, sizeof(memory_bitmap));

    // 然後，我們只把「真實存在」的記憶體標記為「可用」(設為 0)
    for (uint32_t i = 0; i < max_frames; i++) {
        bitmap_clear(i);
    }

    // [極度重要] 保留系統前 4MB (0 ~ 1024 個 Frames) 不被分配！
    // 因為這 4MB 已經被我們的 Kernel 程式碼、VGA 記憶體、IDT/GDT 給佔用了
    for (uint32_t i = 0; i < 1024; i++) {
        bitmap_set(i);
    }
}

void* pmm_alloc_page(void) {
    int free_frame = bitmap_find_first_free();
    if (free_frame == -1) {
        kprintf("PANIC: Out of Physical Memory!\n");
        return NULL; // OOM
    }

    // 標記為已使用
    bitmap_set(free_frame);

    // 計算出實際的物理位址 (Frame 索引 * 4096)
    uint32_t phys_addr = free_frame * PMM_FRAME_SIZE;
    return (void*)phys_addr;
}

void pmm_free_page(void* ptr) {
    uint32_t phys_addr = (uint32_t)ptr;
    uint32_t frame_idx = phys_addr / PMM_FRAME_SIZE;

    bitmap_clear(frame_idx);
}

// 【新增】取得 PMM 使用統計
void pmm_get_stats(uint32_t* total, uint32_t* used, uint32_t* free_frames) {
    *total = max_frames;
    *used = 0;
    for (uint32_t i = 0; i < max_frames; i++) {
        if (bitmap_test(i)) {
            (*used)++;
        }
    }
    *free_frames = *total - *used;
}
```
---
syscall.c
```c
#include "syscall.h"
#include "tty.h"
#include "utils.h"
#include "simplefs.h"
#include "keyboard.h"
#include "task.h"
#include "gui.h"

#define MAX_MESSAGES 16 // IPC 訊息佇列 (Message Queue)
#define SYS_SET_DISPLAY_MODE 99 // 切換 desktop or terminal mode.

extern fs_node_t* simplefs_find(uint32_t dir_lba, char* filename);
extern int vfs_create_file(uint32_t dir_lba, char* filename, char* content);
extern int vfs_readdir(uint32_t dir_lba, int index, char* out_name, uint32_t* out_size, uint32_t* out_type);
extern int vfs_delete_file(uint32_t dir_lba, char* filename);
extern int vfs_mkdir(uint32_t dir_lba, char* dirname);
extern int vfs_getcwd(uint32_t dir_lba, char* buffer);
extern uint32_t simplefs_get_dir_lba(uint32_t current_dir_lba, char* dirname);
extern uint32_t mounted_part_lba;
extern int sys_kill(int pid);
extern void pmm_get_stats(uint32_t* total, uint32_t* used, uint32_t* free_frames);
extern uint32_t paging_get_active_universes(void);

typedef struct {
    char data[128]; // 每則訊息最大 128 bytes
} ipc_msg_t;

fs_node_t* fd_table[32] = {0};

// 環狀佇列 (Circular Buffer)
ipc_msg_t mailbox_queue[MAX_MESSAGES];
int mb_head = 0;  // 讀取頭
int mb_tail = 0;  // 寫入尾
int mb_count = 0; // 目前信箱裡的信件數量


// --- Internal API ----
// Mutex for Single Processor (SMP)
// 核心同步鎖：利用開關中斷來保護 Critical Section
void ipc_lock() {
    __asm__ volatile("cli"); // 關閉中斷：誰都別想搶走我的 CPU！
}

void ipc_unlock() {
    __asm__ volatile("sti"); // 開啟中斷：我用完了，大家可以繼續排隊了。
}


// --- Public API ----

void init_syscalls(void) {
    kprintf("System Calls initialized on Interrupt 0x80 (128).\n");
}

void syscall_handler(registers_t *regs) {
    // Accumulator Register: 函式回傳值或 Syscall 編號
    // 配合 asm/interrupt.S: IRS128
    uint32_t eax = regs->eax;

    if (eax == 2) {
        kprintf((char*)regs->ebx);
    }
    else if (eax == 3) {
        char* filename = (char*)regs->ebx;

        // 取得目前的目錄 (如果沒設定，1 代表相對根目錄)
        uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;
        fs_node_t* node = simplefs_find(current_dir, filename);

        if (node == 0) { regs->eax = -1; return; }
        for (int i = 3; i < 32; i++) {
            if (fd_table[i] == 0) {
                fd_table[i] = node;
                regs->eax = i;
                return;
            }
        }
        regs->eax = -1;
    }
    else if (eax == 4) {
        int fd = (int)regs->ebx;
        uint8_t* buffer = (uint8_t*)regs->ecx;
        uint32_t size = (uint32_t)regs->edx;
        if (fd >= 3 && fd < 32 && fd_table[fd] != 0) {
            regs->eax = vfs_read(fd_table[fd], 0, size, buffer);
        } else {
            regs->eax = -1;
        }
    }
    else if (eax == 5) {
        char c = keyboard_getchar();
        char str[2] = {c, '\0'};
        kprintf(str);
        regs->eax = (uint32_t)c;
    }
    else if (eax == 6) {
        schedule();
    }
    else if (eax == 7) {
        exit_task();
    }
    else if (eax == 8) {
        regs->eax = sys_fork(regs);
    }
    else if (eax == 9) {
        regs->eax = sys_exec(regs);
    }
    else if (eax == 10) {
        regs->eax = sys_wait(regs->ebx);
    }

    // ==========================================
    // IPC 系統呼叫
    // ==========================================
    // Syscall 11: sys_send (傳送訊息)
    else if (eax == 11) {
        char* msg = (char*)regs->ebx;

        ipc_lock(); // LOCK: CRITICAL SECTION

        if (mb_count < MAX_MESSAGES) {
            strcpy(mailbox_queue[mb_tail].data, msg);
            mb_tail = (mb_tail + 1) % MAX_MESSAGES;
            mb_count++;
            regs->eax = 0;
        } else {
            regs->eax = -1; // Queue Full
        }

        ipc_unlock();   // UNLOCK
    }
    // Syscall 12: sys_recv (接收訊息)
    else if (eax == 12) {
        char* buffer = (char*)regs->ebx;

        ipc_lock();

        if (mb_count > 0) {
            strcpy(buffer, mailbox_queue[mb_head].data);
            mb_head = (mb_head + 1) % MAX_MESSAGES;
            mb_count--;
            regs->eax = 1;
        } else {
            regs->eax = 0;
        }

        ipc_unlock();
    }

    // Syscall 13: sbrk (動態記憶體擴充)
    else if (eax == 13) {
        int increment = (int)regs->ebx;
        uint32_t old_end = current_task->heap_end;

        // 把 Heap 的邊界往上推
        current_task->heap_end += increment;

        // 回傳舊的邊界，這就是新分配空間的起始位址！
        regs->eax = old_end;
    }

    // Syscall 14: sys_create (建立/寫入文字檔)
    else if (eax == 14) {
        char* filename = (char*)regs->ebx;
        char* content = (char*)regs->ecx;
        uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;

        ipc_lock();
        regs->eax = vfs_create_file(current_dir, filename, content);
        ipc_unlock();
    }

    // Syscall 15: sys_readdir (讀取目錄內容)
    else if (eax == 15) {
        int index = (int)regs->ebx;
        char* out_name = (char*)regs->ecx;
        uint32_t* out_size = (uint32_t*)regs->edx;
        uint32_t* out_type = (uint32_t*)regs->esi; //從 ESI 暫存器拿出 out_type 指標！

        // 取得目前的目錄
        uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;

        ipc_lock();
        regs->eax = simplefs_readdir(current_dir, index, out_name, out_size, out_type);
        ipc_unlock();
    }

    // Syscall 16: sys_remove (刪除檔案)
    else if (eax == 16) {
        char* filename = (char*)regs->ebx;
        uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;

        ipc_lock();
        regs->eax = vfs_delete_file(current_dir, filename);
        ipc_unlock();
    }

    // Syscall 17: sys_mkdir (建立目錄)
    else if (eax == 17) {
        char* dirname = (char*)regs->ebx;
        uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;

        ipc_lock();
        regs->eax = vfs_mkdir(current_dir, dirname);
        ipc_unlock();
    }

    // Syscall 18: sys_chdir (切換目錄)
    else if (eax == 18) {
        char* dirname = (char*)regs->ebx;
        // 如果還沒有 CWD，預設就是根目錄
        uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;

        ipc_lock();
        if (strcmp(dirname, "/") == 0) {
            current_task->cwd_lba = 1; // 回到根目錄
            regs->eax = 0;
        } else {
            uint32_t target_lba = simplefs_get_dir_lba(current_dir, dirname);
            if (target_lba != 0) {
                current_task->cwd_lba = target_lba; // 切換成功, 更新任務的 CWD！
                regs->eax = 0;
            } else {
                regs->eax = -1; // 找不到該目錄
            }
        }
        ipc_unlock();
    }
    // Syscall 19: sys_getcwd (取得當前路徑)
    else if (eax == 19) {
        char* buffer = (char*)regs->ebx;

        // 由 Syscall 層負責查出目前的目錄 LBA
        uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;

        ipc_lock();
        regs->eax = vfs_getcwd(current_dir, buffer);
        ipc_unlock();
    }

    // ==========================================
    // 【Day 63 新增】系統與行程資訊 Syscall
    // ==========================================
    // Syscall 20: sys_getpid
    else if (eax == 20) {
        ipc_lock();
        regs->eax = current_task->pid;
        ipc_unlock();
    }
    // Syscall 21: sys_get_process_list
    else if (eax == 21) {
        process_info_t* list = (process_info_t*)regs->ebx;
        int max_count = (int)regs->ecx;

        ipc_lock();
        regs->eax = task_get_process_list(list, max_count);
        ipc_unlock();
    }
    // ==========================================
    // 【Day 65 優化】Syscall 22: 清空終端機畫面
    // ==========================================
    else if (eax == 22) {
        ipc_lock();
        terminal_initialize(); // 清空 text_buffer 並把游標歸零
        extern void gui_redraw(void);
        gui_redraw();          // 強制觸發畫面更新
        regs->eax = 0;
        ipc_unlock();
    }
    // ==========================================
    // 【Day 66 新增】Syscall 24: sys_kill
    // ==========================================
    else if (eax == 24) {
        int target_pid = (int)regs->ebx;
        regs->eax = sys_kill(target_pid);
    }

    // Syscall 25: sys_get_mem_info
    else if (eax == 25) {
        mem_info_t* info = (mem_info_t*)regs->ebx;
        ipc_lock();
        pmm_get_stats(&info->total_frames, &info->used_frames, &info->free_frames);
        info->active_universes = paging_get_active_universes();
        regs->eax = 0;
        ipc_unlock();
    }

    // ==========================================
    // 【Day 68 新增】Syscall 26: sys_create_window
    // ==========================================
    else if (eax == 26) {
        char* title = (char*)regs->ebx;
        int width = (int)regs->ecx;
        int height = (int)regs->edx;

        // 簡單的視窗錯開邏輯，避免視窗全部疊在一起
        static int win_offset = 0;
        int x = 200 + win_offset;
        int y = 150 + win_offset;
        win_offset = (win_offset + 30) % 200;

        ipc_lock();
        // 綁定當前呼叫它的行程 PID！
        regs->eax = create_window(x, y, width, height, title, current_task->pid);
        extern void gui_redraw(void);
        gui_redraw(); // 立刻把視窗畫出來
        ipc_unlock();
    }
    // ==========================================
    // 【Day 69 新增】Syscall 27: sys_update_window
    // 將 User Space 的畫布資料推送到 Kernel 進行合成
    // ==========================================
    else if (eax == 27) {
        int win_id = (int)regs->ebx;
        uint32_t* user_buffer = (uint32_t*)regs->ecx;

        ipc_lock();
        extern window_t* get_window(int id);
        window_t* win = get_window(win_id);

        // 確保視窗存在，而且這個行程真的是視窗的主人！(安全性檢查)
        if (win != 0 && win->owner_pid == current_task->pid && win->canvas != 0) {
            int canvas_bytes = (win->width - 4) * (win->height - 24) * 4;
            // 把 User 的陣列拷貝到 Kernel 的視窗畫布裡
            memcpy(win->canvas, user_buffer, canvas_bytes);

            // 標記畫面為髒，請排程器在下個 Tick 重繪螢幕
            extern void gui_redraw(void);
            gui_redraw();
        }
        regs->eax = 0;
        ipc_unlock();
    }

    // Syscall: 99
    else if (eax == SYS_SET_DISPLAY_MODE) {
        // 假設 EBX 存放第一個參數 (is_gui)
        int is_gui = (int) regs->ebx;

        ipc_lock();
        switch_display_mode(is_gui);
        regs->eax = 0; // 回傳成功
        ipc_unlock();
    }
}
```
---
kernel.c
```c
#include "tty.h"
#include "utils.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "timer.h"
#include "pmm.h"
#include "kheap.h"
#include "ata.h"
#include "mbr.h"
#include "vfs.h"
#include "simplefs.h"
#include "elf.h"
#include "task.h"
#include "multiboot.h"
#include "gfx.h"
#include "mouse.h"
#include "gui.h"

void setup_filesystem(uint32_t part_lba, multiboot_info_t* mbd) {
    kprintf("[Kernel] Setting up SimpleFS environment...\n");
    simplefs_mount(part_lba);
    simplefs_format(part_lba, 10000);
    simplefs_create_file(1, "hello.txt", "This is the content of the very first file ever created on Simple OS!\n", 70);

    // 【修改】自動巡覽所有模組並寫入 HDD
    if (mbd->mods_count > 0) {
        multiboot_module_t* mod = (multiboot_module_t*)mbd->mods_addr;
        char* filenames[] = {
            "shell.elf",
            "echo.elf", "ping.elf", "pong.elf",
            // file/directory
            "touch.elf", "cat.elf", "ls.elf", "rm.elf", "mkdir.elf",
            // process
            "ps.elf", "top.elf", "kill.elf",
            // memory
            "free.elf", "segfault.elf",
            // window app
            "status.elf", "paint.elf"
        };
        uint32_t expected_mods = sizeof(filenames) / sizeof(filenames[0]);

        for(uint32_t i = 0; i < mbd->mods_count && i < expected_mods; i++) {
            uint32_t size = mod[i].mod_end - mod[i].mod_start;
            kprintf("[Kernel] Installing [%s] to HDD (Size: %d bytes)...\n", filenames[i], size);
            simplefs_create_file(1, filenames[i], (char*)mod[i].mod_start, size);
        }
    }

    // simplefs_list_files();

}

void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
    (void)magic; // 忽略未使用的警告

    terminal_initialize();
    kprintf("=== Simple OS Booting ===\n");

    // --- 系統底層基礎建設 ---
    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384);
    init_kheap();
    init_gfx(mbd);

    // ==========================================
    // 2. 解析 GRUB 傳遞的 Cmdline
    // ==========================================
    int is_gui = 1; // 預設為 GUI 模式

    // 檢查 mbd 的 bit 2，確認 cmdline 是否有效
    if (mbd->flags & (1 << 2)) {
        char* cmdline = (char*) mbd->cmdline;
        // 如果 GRUB 參數包含 mode=cli，就切換到 CLI 模式
        if (strstr(cmdline, "mode=cli") != 0) {
            is_gui = 0;
        }
    }

    // 3. 根據模式初始化系統介面
    enable_gui_mode(is_gui);
    terminal_set_mode(is_gui);

    if (is_gui) {
        init_gui();
        int term_win = create_window(50, 50, 368, 228, "Simple OS Terminal", 1); // 預設 PID(1) 建立
        terminal_bind_window(term_win);
        gui_render(400, 300);

    } else {
        // CLI 模式只綁定一個無效的視窗 ID，並強制重繪全螢幕
        terminal_bind_window(-1);
        gui_redraw();
    }

    init_mouse();

    __asm__ volatile ("sti");

    // 左上角的終端機文字會自然地印在藍綠色的桌面上
    kprintf("=== OS Subsystems Ready ===\n\n");
    kprintf("Welcome to Simple OS Desktop Environment!\n");

    // --- 儲存與檔案系統 ---
    uint32_t part_lba = parse_mbr();
    if (part_lba == 0) {
        kprintf("Disk Error: No partition found.\n");
        while(1) __asm__ volatile("hlt");
    }
    // 初始化 file system: mount, format, copy external applications
    setup_filesystem(part_lba, mbd);

    // ==========================================
    // 應用程式載入與排程 (Ring 0 -> Ring 3)
    // ==========================================
    kprintf("[Kernel] Fetching 'shell.elf' from Virtual File System...\n");
    fs_node_t* app_node = simplefs_find(1, "shell.elf");

    if (app_node != 0) {
        uint8_t* app_buffer = (uint8_t*) kmalloc(app_node->length);
        vfs_read(app_node, 0, app_node->length, app_buffer);
        uint32_t entry_point = elf_load((elf32_ehdr_t*)app_buffer);

        if (entry_point != 0) {
            kprintf("Creating Initial Process (Shell)...\n\n");

            // 啟動排程器 (Timer IRQ0 開始跳動)
            init_multitasking();

            // 為 Shell 分配專屬的 User Stack
            uint32_t ustack1_phys = (uint32_t) pmm_alloc_page();
            map_page(0x083FF000, ustack1_phys, 7);

            // 建立 Ring 3 主任務
            create_user_task(entry_point, 0x083FF000 + 4096);

            kprintf("[Kernel] Dropping to idle state. Enjoy your GUI!\n");

            // Kernel 功成身退，把自己宣告死亡！
            exit_task();
        }
    } else {
        kprintf("[Kernel] Error: Shell (shell.elf) not found on disk.\n");
    }

    while (1) { __asm__ volatile ("hlt"); }
}
```

---
segfault.c
```c
#include "stdio.h"
#include "unistd.h"

int main() {
    printf("Prepare to crash intentionally...\n");

    // 刻意製造一個空指標 (NULL Pointer)
    int *bad_ptr = (int*)0x00000000;

    // 嘗試寫入 0x00000000 (這塊記憶體沒有被 Paging 映射)
    // 這行會立刻觸發 CPU 硬體中斷 (Page Fault)！
    *bad_ptr = 42;

    // 如果 OS 防護失敗，或者沒有擋下來，這行就會印出來
    printf("Wait... I survived?! (This should never print)\n");

    return 0;
}
```
---
paint.c
```c
#include "stdio.h"
#include "unistd.h"

#define CANVAS_W 200
#define CANVAS_H 150

int main() {
    int win_id = create_gui_window("Dynamic Paint", CANVAS_W + 4, CANVAS_H + 24);
    if (win_id < 0) {
        printf("Failed to create window.\n");
        return -1;
    }

    // ==========================================
    // 【修改】向系統申請 Heap 記憶體 (sbrk)
    // 絕對不要把 117KB 的大傢伙塞在 4KB 的 Stack 裡！
    // ==========================================
    unsigned int* my_canvas = (unsigned int*)sbrk(CANVAS_W * CANVAS_H * 4);

    int offset = 0;

    while (1) {
        // 1. 在畫布上作畫 (產生會動的彩色漸層 / XOR 紋理)
        for (int y = 0; y < CANVAS_H; y++) {
            for (int x = 0; x < CANVAS_W; x++) {
                unsigned int r = (x + offset) % 255;
                unsigned int g = (y + offset) % 255;
                unsigned int b = (x ^ y) % 255;

                my_canvas[y * CANVAS_W + x] = (r << 16) | (g << 8) | b;
            }
        }

        // 2. 把畫布交給 Kernel 去更新螢幕！
        update_gui_window(win_id, my_canvas);

        // 3. 推進動畫影格，並稍微讓出 CPU 休息一下
        offset += 3;
        for(volatile int i = 0; i < 500000; i++);
    }

    return 0;
}
```
