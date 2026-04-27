/**
 * @file src/kernel/kernel/task.c
 * @brief 核心多工任務管理系統 (Kernel Multitasking System)
 *
 * 本檔案負責處理進程控制區塊 (PCB) 的管理、任務排程 (Scheduling)、
 * 垃圾回收 (GC) 以及系統呼叫如 fork, exec, wait 的核心實作。
 */

#include <stdint.h>
#include "task.h"
#include "pmm.h"
#include "kheap.h"
#include "utils.h"
#include "tty.h"
// #include "timer.h"
#include "gdt.h"
#include "paging.h"
#include "simplefs.h"
#include "vfs.h"
#include "elf.h"
#include "gui.h"

/** @brief 當前正在執行的任務指標 */
volatile task_t *current_task = 0;

/** @brief 排程器準備佇列的起點 (目前採環狀鏈結串列) */
volatile task_t *ready_queue = 0;

/** @brief 閒置任務 (Idle Task)，當系統無事可做時執行 */
task_t *idle_task = 0;

/** @brief 下一個可分配的 PID */
// `next_pid` 目前是 `uint32_t` 遞增，理論上可用很久，但若長時間運作且頻繁開關進程，需要考慮 PID 回收機制，以免 PID 數字變得過大。
uint32_t next_pid = 0;

extern uint32_t page_directory[];   // paging.c
extern volatile uint32_t tick;      // timer.c

// task_switch.S
extern void switch_task(uint32_t *current_esp, uint32_t *next_esp, uint32_t next_cr3);
extern void child_ret_stub();

/**
 * @brief 根據 PID 尋找特定的任務控制區塊
 * @param pid 目標 PID
 * @return 找到回傳 task_t 指標，否則回傳 NULL
 */
static task_t* get_task_by_pid(uint32_t pid) {
    if (!current_task) return 0;
    task_t *start = (task_t*)current_task;
    task_t *temp = start;
    do {
        if (temp->pid == pid) return temp;
        temp = temp->next;
    } while (temp != start);
    return 0;
}

/**
 * @brief 垃圾回收器 (Task Garbage Collector)
 *
 * 掃描任務鏈結串列，尋找標記為 TASK_DEAD 的任務，並釋放其佔用的
 * 核心堆疊 (Kernel Stack) 與 PCB 記憶體空間。
 */
static void task_gc() {
    // 【修復】如果當前是 idle_task，它不在主環內，不要以它為起點進行 GC
    if (!current_task || current_task == idle_task) return;

    task_t *prev = (task_t*)current_task;
    task_t *curr = prev->next;

    while (curr != current_task) {
        if (curr->state == TASK_DEAD) {
            // 【修復：懸空指標防護】如果剛好刪除到 ready_queue 所指的任務，必須交棒
            if (curr == ready_queue) {
                ready_queue = curr->next;
                // 如果任務全死光了，清空指標
                if (ready_queue == curr) ready_queue = 0;
            }

            prev->next = curr->next;
            if (curr->kernel_stack != 0) {
                kfree((void*)(curr->kernel_stack - PAGE_SIZE));
            }
            kfree((void*)curr);
            curr = prev->next;
        } else {
            prev = curr;
            curr = curr->next;
        }
    }
}

/**
 * @brief 閒置任務迴圈
 * 當系統中沒有任何可執行的任務時，CPU 將在此執行 hlt (休眠) 節省電力，
 * 直到下一次時脈中斷觸發排程。
 */
void idle_loop() {
    while(1) { __asm__ volatile("sti; hlt"); }
}

/**
 * @brief 初始化多工系統
 *
 * 將目前的執行流包裝成 PID 0 (kernel_main)，並建立閒置任務。
 */
void init_multitasking() {
    // ----------------------------------------------------
    // 1. 初始化 Kernel 主任務 (PID 0)
    // ----------------------------------------------------
    task_t *main_task = (task_t*) kmalloc(sizeof(task_t));
    main_task->pid = next_pid++;
    main_task->ppid = 0;
    strcpy(main_task->name, "[kernel_main]");

    main_task->esp = 0; // 初始值，將在第一次切換時由 ASM 寫入
    main_task->kernel_stack = 0;
    main_task->state = TASK_RUNNING;
    main_task->wait_pid = 0;
    main_task->page_directory = (uint32_t) page_directory;
    main_task->cwd_lba = 0;
    main_task->total_ticks = 0;

    // 建立環狀串列
    main_task->next = main_task;
    current_task = main_task;
    ready_queue = main_task;

    // ----------------------------------------------------
    // 2. 建立 Idle Task (PID 9999)
    // ----------------------------------------------------
    idle_task = (task_t*) kmalloc(sizeof(task_t));
    idle_task->pid = KERNEL_IDLE_PID;
    idle_task->ppid = 0;
    strcpy(idle_task->name, "[kernel_idle]");
    idle_task->state = TASK_RUNNING;
    idle_task->page_directory = (uint32_t) page_directory;

    // 為 Idle Task 配置獨立核心堆疊
    uint32_t *kstack_mem = (uint32_t*) kmalloc(PAGE_SIZE);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + PAGE_SIZE);
    idle_task->kernel_stack = (uint32_t) kstack;

    // 偽造中斷堆疊幀，模擬該任務曾被中斷
    *(--kstack) = (uint32_t) idle_loop; // EIP: 跳轉到閒置迴圈
    for(int i = 0; i < 8; i++) *(--kstack) = 0; // Pusha (暫存器清零)
    *(--kstack) = EFLAGS_DEFAULT;       // EFLAGS: 開啟中斷 (IF=1)

    idle_task->esp = (uint32_t) kstack;
}

/**
 * @brief 建立使用者模式任務
 * @param entry_point 程式起始位址
 * @param user_stack_top 使用者堆疊頂部位址
 *
 * 此函式負責建立一個 Ring 3 任務，並手動佈置堆疊以確保 IRET 後能切換到 User Mode。
 */
void create_user_task(uint32_t entry_point, uint32_t user_stack_top) {
    task_t *new_task = (task_t*) kmalloc(sizeof(task_t));
    new_task->pid = next_pid++;
    new_task->ppid = current_task->pid;
    strcpy(new_task->name, "init");

    new_task->state = TASK_RUNNING;
    new_task->page_directory = current_task->page_directory;
    new_task->cwd_lba = 0;

    // ----------------------------------------------------
    // 1. 初始化使用者堆積 (User Heap)
    // ----------------------------------------------------
    for (int i = 0; i < USER_HEAP_PAGES; i++) {
        uint32_t heap_phys = (uint32_t)pmm_alloc_page();
        map_page(USER_HEAP_START + (i * PAGE_SIZE), heap_phys, 7); // User, RW, Present
    }
    new_task->heap_start = USER_HEAP_START;
    new_task->heap_end = USER_HEAP_START;

    // ----------------------------------------------------
    // 2. 佈置使用者堆疊與核心堆疊
    // ----------------------------------------------------
    uint32_t *kstack_mem = (uint32_t*) kmalloc(PAGE_SIZE);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + PAGE_SIZE);
    new_task->kernel_stack = (uint32_t) kstack;

    // 模擬 C main(argc, argv) 參數佈局
    uint32_t *ustack = (uint32_t*) (user_stack_top - USER_STACK_PADDING);
    *(--ustack) = 0;         // NULL argv 結尾
    uint32_t argv_ptr = (uint32_t) ustack;
    *(--ustack) = argv_ptr;  // argv
    *(--ustack) = 0;         // argc
    *(--ustack) = 0;         // Dummy return address

    // ----------------------------------------------------
    // 3. 偽造 IRET 中斷幀 (切換 Ring 0 -> Ring 3)
    // ----------------------------------------------------
    *(--kstack) = USER_DS;          // User SS
    *(--kstack) = (uint32_t)ustack; // User ESP
    *(--kstack) = EFLAGS_DEFAULT;   // EFLAGS
    *(--kstack) = USER_CS;          // User CS
    *(--kstack) = entry_point;      // User EIP

    // 偽造 child_ret_stub 彈出的暫存器
    *(--kstack) = 0; // EBP
    *(--kstack) = 0; // EBX
    *(--kstack) = 0; // ESI
    *(--kstack) = 0; // EDI

    *(--kstack) = (uint32_t) child_ret_stub; // ← switch_task 的 ret 跳到這裡

    // 為 switch_task 的彈出指令留空位
    for(int i = 0; i < 8; i++) *(--kstack) = 0;
    *(--kstack) = EFLAGS_DEFAULT;

    new_task->esp = (uint32_t) kstack;

    // 加入排程鏈結串列
    new_task->next = current_task->next;
    current_task->next = new_task;
}

void exit_task() {
    // 1. 關閉該行程創建的所有圖形介面視窗
    gui_close_windows_by_pid(current_task->pid);

    task_t *temp = current_task->next;
    int parent_waiting = 0;

    // 2. 喚醒可能正在 wait() 這個行程的父行程
    while (temp != current_task) {
        if (temp->state == TASK_WAITING && temp->wait_pid == current_task->pid) {
            temp->state = TASK_RUNNING;
            temp->wait_pid = 0;
            parent_waiting = 1;
        }
        temp = temp->next;
    }

    // 3. 釋放該行程專屬的分頁目錄 (如果是 Kernel 就不能清掉)
    if (current_task->page_directory != (uint32_t)page_directory) {
        free_page_directory(current_task->page_directory);
    }

    // 4. 決定行程的最終狀態
    // 如果父行程有在等待 (wait)，則變成 ZOMBIE 讓父行程來收屍並拿取退出碼
    // 否則直接標記為 DEAD，排程器稍後會直接回收記憶體
    if (current_task->ppid == 0 || !parent_waiting) {
        current_task->state = TASK_DEAD;
    } else {
        current_task->state = TASK_ZOMBIE;
    }

    // 讓出 CPU
    schedule();
}

void schedule() {
    if (!current_task) return;

    // ----------------------------------------------------
    // 步驟 1: 垃圾回收 + 喚醒睡眠任務
    // ----------------------------------------------------
    // 保存進入前的 EFLAGS (包含中斷開關狀態)
    uint32_t eflags;
    __asm__ volatile("pushfl; popl %0" : "=r"(eflags));

    // 關閉中斷，避免 Race Condition
    __asm__ volatile("cli");

    // 執行 GC: 釋放已標記為 TASK_DEAD 的 PCB 與 Kernel Stack
    task_gc();

    // 掃描睡眠任務，時間到了就喚醒
    // 不管現在是不是 idle_task，永遠從「主佇列 (ready_queue)」掃描
    if (ready_queue) {
        task_t *iter = (task_t*)ready_queue;
        task_t *first = iter;
        do {
            if (iter->state == TASK_SLEEPING && tick >= iter->wake_up_tick) {
                iter->state = TASK_RUNNING;
                iter->wake_up_tick = 0;
            }
            iter = iter->next;
        } while (iter != first && iter != 0);
    }

    // ----------------------------------------------------
    // 步驟 2：Round-Robin 輪轉找下一個準備就緒 (RUNNING) 的行程
    // ----------------------------------------------------
    task_t *next_run = idle_task; // 預設降落傘：都沒人跑，就切給 idle

    if (ready_queue) {
        // 從當前行程的 next 開始找，公平輪轉
        task_t *start_node = (current_task == idle_task)
            ? (task_t*)ready_queue
            : current_task->next;

        // 排程器從當前行程的「下一個」開始掃，找到第一個 TASK_RUNNING 的就選它
        task_t *iter = start_node;
        do {
            if (iter->state == TASK_RUNNING) {
                next_run = iter;
                break;
            }
            iter = iter->next;
        } while (iter != start_node && iter != 0);
    }

    // ----------------------------------------------------
    // 步驟 3：執行 Context Switch
    // ----------------------------------------------------
    task_t *prev = (task_t*)current_task;
    current_task = next_run;

    if (current_task != prev) {
        if (current_task->kernel_stack != 0) {
            // 更新 TSS.esp0 (Task State Segment)
            // @ref: src/kernel/lib/gdt.h#set_kernel_stack
            set_kernel_stack(current_task->kernel_stack);
        }
        // @ref: src/kernel/arch/x86/switch_task.S
        switch_task(
            (uint32_t*)&prev->esp,          // 儲存舊行程的 ESP
            (uint32_t*)&current_task->esp,  // 載入新行程的 ESP
            current_task->page_directory    // 新行程的 CR3
        );
    }

    // 恢復進入前的狀態，而不是盲目 sti
    __asm__ volatile("pushl %0; popfl" : : "r"(eflags));
}

int sys_fork(registers_t *regs) {
    // ----------------------------------------------------
    // 階段 1：複製父行程的 PCB
    // ----------------------------------------------------
    task_t *child = (task_t*) kmalloc(sizeof(task_t));
    child->pid = next_pid++;
    child->ppid = current_task->pid;
    strcpy(child->name, (const char*)current_task->name);

    child->state = TASK_RUNNING;
    child->wait_pid = 0;

    // 注意：這裡目前是共享 Page Directory (未來若實作寫入時複製 COW 會在這裡修改)
    child->page_directory = current_task->page_directory;
    child->cwd_lba = current_task->cwd_lba;

    child->heap_start = current_task->heap_start;
    child->heap_end = current_task->heap_end;
    child->total_ticks = 0; // 子行程的執行時間重新計算

    // 為子行程準備獨立的 Kernel Stack
    uint32_t *kstack_mem = (uint32_t*) kmalloc(PAGE_SIZE);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + PAGE_SIZE);
    child->kernel_stack = (uint32_t) kstack;

    // ----------------------------------------------------
    // 階段 2：拷貝 User Stack
    // 為了避免多個行程在同一個位址空間中 Stack 碰撞，根據 PID 計算不同的 Base
    // ----------------------------------------------------
    uint32_t child_ustack_base = USER_STACK_TOP - (child->pid * PAGE_SIZE);
    uint32_t child_ustack_phys = (uint32_t)pmm_alloc_page();
    map_page(child_ustack_base, child_ustack_phys, 7);

    // 刷新 TLB (重新載入 CR3)
    uint32_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3));

    uint32_t parent_ustack_base = regs->user_esp & 0xFFFFF000;
    uint8_t *src = (uint8_t*) parent_ustack_base;
    uint8_t *dst = (uint8_t*) child_ustack_base;
    for(int i = 0; i < PAGE_SIZE; i++) { dst[i] = src[i]; }

    int32_t delta = child_ustack_base - parent_ustack_base;
    uint32_t child_user_esp = regs->user_esp + delta;

    // ----------------------------------------------------
    // [高難度操作] 修正 Stack Frame (EBP 鏈)
    // 因為 Stack 被搬移到新的記憶體位址，所以存放 "指向上一個 EBP 位址" 的數值必須全部加上位移量 (delta)
    // ----------------------------------------------------
    uint32_t child_ebp = regs->ebp;
    if (child_ebp >= parent_ustack_base && child_ebp < parent_ustack_base + PAGE_SIZE) {
        child_ebp += delta;
    }

    uint32_t curr_ebp = child_ebp;
    while (curr_ebp >= child_ustack_base && curr_ebp < child_ustack_base + PAGE_SIZE) {
        uint32_t *ebp_ptr = (uint32_t*) curr_ebp;
        uint32_t next_ebp = *ebp_ptr;
        // 如果這個 EBP 指向同一個 Stack page 內，就進行修正
        if (next_ebp >= parent_ustack_base && next_ebp < parent_ustack_base + PAGE_SIZE) {
            *ebp_ptr = next_ebp + delta;
            curr_ebp = *ebp_ptr;
        } else {
            break; // 到達底部或外部
        }
    }

    // ----------------------------------------------------
    // 階段 3：準備子行程被排程到的「第一次恢復狀態」
    // ----------------------------------------------------
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
    *(--kstack) = EFLAGS_DEFAULT;

    child->esp = (uint32_t) kstack;

    // 加入排程佇列
    child->next = current_task->next;
    current_task->next = child;

    // 父行程的 fork 回傳子行程的 PID
    return child->pid;
}

int sys_exec(registers_t *regs) {
    char* filename = (char*)regs->ebx;
    char** old_argv = (char**)regs->ecx;

    // ----------------------------------------------------
    // 階段 1：尋找並載入執行檔 (ELF)
    // ----------------------------------------------------
    uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;
    fs_node_t* file_node = simplefs_find(current_dir, filename);

    // 支援 PATH 的簡陋版：如果在當前目錄找不到，去根目錄 (/) 找
    if (file_node == 0 && current_dir != 1) {
        file_node = simplefs_find(1, filename);
    }
    if (file_node == 0) { return -1; }

    strcpy((char*)current_task->name, filename);

    uint8_t* buffer = (uint8_t*) kmalloc(file_node->length);
    vfs_read(file_node, 0, file_node->length, buffer);

    // ----------------------------------------------------
    // 階段 2：保存命令列參數 (Arguments)
    // 因為我們即將砍掉舊的位址空間，必須先把參數字串拷貝到核心安全區
    // ----------------------------------------------------
    int argc = 0;
    char* safe_argv[MAX_EXEC_ARGS];

    if (old_argv != 0 && (uint32_t)old_argv > USER_ARGS_LIMIT) {
        while (old_argv[argc] != 0 && argc < (MAX_EXEC_ARGS - 1)) {
            int len = strlen(old_argv[argc]) + 1;
            safe_argv[argc] = (char*) kmalloc(len);
            memcpy(safe_argv[argc], old_argv[argc], len);
            argc++;
        }
    }
    safe_argv[argc] = 0;

    // ----------------------------------------------------
    // 階段 3：建立並切換至新的分頁目錄 (Page Directory)
    // ----------------------------------------------------
    uint32_t new_cr3 = create_page_directory();
    uint32_t old_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(old_cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(new_cr3)); // 切過去以便載入 ELF

    uint32_t entry_point = elf_load((elf32_ehdr_t*)buffer);
    kfree(buffer);

    if (entry_point == 0) {
        // 如果載入失敗，切回舊的空間並報錯
        __asm__ volatile("mov %0, %%cr3" : : "r"(old_cr3));
        return -1;
    }

    current_task->page_directory = new_cr3;

    // ----------------------------------------------------
    // 階段 4：為新程式佈置全新的 User Stack 與 User Heap
    // ----------------------------------------------------
    uint32_t clean_user_stack_top = USER_STACK_TOP + PAGE_SIZE;
    uint32_t ustack_phys = (uint32_t)pmm_alloc_page();
    map_page(USER_STACK_TOP, ustack_phys, 7);

    for (int i = 0; i < USER_HEAP_PAGES; i++) {
        uint32_t heap_phys = (uint32_t)pmm_alloc_page();
        map_page(USER_HEAP_START + (i * PAGE_SIZE), heap_phys, 7);
    }
    current_task->heap_start = USER_HEAP_START;
    current_task->heap_end = USER_HEAP_START;

    uint32_t stack_ptr = clean_user_stack_top - USER_STACK_PADDING;

    // ----------------------------------------------------
    // 階段 5：將剛剛保存的命令列參數 (Args) 推入新程式的 Stack 頂部
    // 佈局結構：[字串資料] -> [argv 指標陣列] -> [argv 的雙重指標] -> [argc]
    // ----------------------------------------------------
    uint32_t argv_ptrs[MAX_EXEC_ARGS] = {0};
    if (argc > 0) {
        // a. 先把字串本身推入堆疊
        for (int i = argc - 1; i >= 0; i--) {
            int len = strlen(safe_argv[i]) + 1;
            stack_ptr -= len;
            memcpy((void*)stack_ptr, safe_argv[i], len);
            argv_ptrs[i] = stack_ptr; // 記錄字串在 Stack 中的實際位址
            kfree(safe_argv[i]);      // 釋放剛才暫存的記憶體
        }

        stack_ptr = stack_ptr & ~3;   // 確保 4-byte 對齊
        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = 0;    // argv[argc] = NULL

        // b. 推入指向那些字串的指標 (argv 陣列)
        for (int i = argc - 1; i >= 0; i--) {
            stack_ptr -= 4;
            *(uint32_t*)stack_ptr = argv_ptrs[i];
        }

        uint32_t argv_base = stack_ptr;

        // c. 推入 C 語言 main(int argc, char** argv) 的參數
        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = argv_base; // arg2: char** argv
        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = argc;      // arg1: int argc
    } else {
        // 沒有參數的情況
        stack_ptr -= 4; *(uint32_t*)stack_ptr = 0;
        stack_ptr -= 4; *(uint32_t*)stack_ptr = 0;
    }

    // Return address (User mode 結束程式用的，通常是 libc 的 exit)
    stack_ptr -= 4; *(uint32_t*)stack_ptr = 0;

    // ----------------------------------------------------
    // 階段 6：修改系統呼叫返回時的暫存器
    // 這樣 IRET 回去時，CPU 就會直接開始執行新程式
    // ----------------------------------------------------
    regs->eip = entry_point;
    regs->user_esp = stack_ptr;

    return 0;
}

int sys_wait(uint32_t pid) {
    task_t *temp = current_task->next;
    int found = 0;

    // 1. 確認該子行程存在，且是否已經是 ZOMBIE (已經死掉了)
    while (temp != current_task) {
        if (temp->pid == pid) {
            found = 1;
            if (temp->state == TASK_ZOMBIE) {
                // 子行程已經結束，直接幫它收屍，無須阻塞 (Block) 父行程
                temp->state = TASK_DEAD;
                return 0;
            }
            break;
        }
        temp = temp->next;
    }

    if (!found) return -1; // 找不到該子行程

    // 2. 子行程還在執行，父行程進入 WAITING 狀態並讓出 CPU
    current_task->wait_pid = pid;
    current_task->state = TASK_WAITING;
    schedule();

    // 3. 當父行程被喚醒 (排程回到這裡)，代表子行程已經呼叫 exit() 變成 ZOMBIE 了
    // 再次掃描鏈結串列，將它設定為 DEAD 讓系統回收
    temp = current_task->next;
    while (temp != current_task) {
        if (temp->pid == pid && temp->state == TASK_ZOMBIE) {
            temp->state = TASK_DEAD;
            break;
        }
        temp = temp->next;
    }

    return 0;
}

int sys_kill(uint32_t pid) {
    // 防止自殺或誤殺系統核心 (Kernel Main PID 1 / Idle PID 9999)
    if (pid <= 1 || pid == KERNEL_IDLE_PID) return -1;

    // 因為牽涉到跨行程狀態修改，鎖定中斷避免 Race Condition
    __asm__ volatile("cli");

    task_t *target = get_task_by_pid(pid);
    if (!target || target->state == TASK_DEAD) {
        __asm__ volatile("sti");
        return -1;
    }

    // 1. 強制關閉它擁有的 GUI 視窗
    gui_close_windows_by_pid(pid);

    // 2. 檢查目標的父行程是否正在 wait() 它
    task_t *parent = get_task_by_pid(target->ppid);
    int parent_waiting = 0;
    if (parent && parent->state == TASK_WAITING && parent->wait_pid == pid) {
        parent->state = TASK_RUNNING; // 喚醒父行程
        parent->wait_pid = 0;
        parent_waiting = 1;
    }

    // 3. 根據父行程狀態決定它是變殭屍還是直接死透
    if (parent_waiting) {
        target->state = TASK_ZOMBIE;
    } else {
        target->state = TASK_DEAD;
    }

    __asm__ volatile("sti");
    return 0;
}

int task_get_process_list(process_info_t* list, int max_count) {
    if (!current_task) return 0;

    int count = 0;
    task_t* temp = (task_t*)current_task;

    // 走訪環狀鏈結串列，收集每個尚未死亡的 PCB 資訊
    do {
        if (temp->state != TASK_DEAD) {
            list[count].pid = temp->pid;
            list[count].ppid = temp->ppid;
            strncpy(list[count].name, temp->name, 32);
            list[count].state = temp->state;
            list[count].total_ticks = temp->total_ticks;

            // 估算記憶體使用量：(Heap 結尾 - Heap 開頭) + 預先分配的 1MB Heap + 4KB User Stack
            if (temp->heap_end >= temp->heap_start) {
                list[count].memory_used = (temp->heap_end - temp->heap_start) + (USER_HEAP_PAGES * PAGE_SIZE) + PAGE_SIZE;
            } else {
                // 如果是 Kernel Task，就只有 4KB Kernel Stack
                list[count].memory_used = PAGE_SIZE;
            }

            count++;
        }
        temp = temp->next;
    } while (temp != current_task && count < max_count);

    return count;
}


int sys_sleep(uint32_t ms) {
    // 假設頻率設定為 100Hz，則 1 tick = 10ms
    extern uint32_t timer_frequency;  // 從 timer.c 暴露
    uint32_t ticks_to_sleep = (ms * timer_frequency) / 1000;

    if (ticks_to_sleep == 0) ticks_to_sleep = 1; // 至少睡一個滴答

    current_task->wake_up_tick = tick + ticks_to_sleep;
    current_task->state = TASK_SLEEPING;

    // 讓出 CPU，schedule 會處理 cli/sti
    schedule();
    return 0;
}
