/**
 * @file src/kernel/kernel/task.c
 * @brief Task management and multitasking system.
 */

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
#include "gui.h"

// ==========================================
// 記憶體分佈與權限常數定義
// ==========================================
#define PAGE_SIZE           4096
#define USER_HEAP_START     0x10000000
#define USER_HEAP_PAGES     256
#define USER_STACK_TOP      0x083FF000
#define USER_STACK_PADDING  64
#define USER_ARGS_LIMIT     0x08000000
#define MAX_EXEC_ARGS       16

#define USER_DS             0x23
#define USER_CS             0x1B
#define EFLAGS_DEFAULT      0x0202

#define KERNEL_IDLE_PID     9999

volatile task_t *current_task = 0;
volatile task_t *ready_queue = 0;
uint32_t next_pid = 0;
task_t *idle_task = 0;

extern uint32_t page_directory[];
extern void switch_task(uint32_t *current_esp, uint32_t *next_esp, uint32_t next_cr3);
extern void child_ret_stub();

/** @brief Idle loop for the idle task. */
void idle_loop() {
    // sti 開啟中斷，hlt 讓 CPU 進入休眠，直到下一次時鐘中斷到來
    while(1) { __asm__ volatile("sti; hlt"); }
}

void init_multitasking() {
    // ----------------------------------------------------
    // 階段 1：將當下正在執行的 Kernel 初始化流程轉換為 main_task (PID 0)
    // ----------------------------------------------------
    task_t *main_task = (task_t*) kmalloc(sizeof(task_t));
    main_task->pid = next_pid++;
    main_task->ppid = 0;
    strcpy(main_task->name, "[kernel_main]");

    main_task->esp = 0;         // 等到真的被切換出去時，才會寫入真實數值
    main_task->kernel_stack = 0;
    main_task->state = TASK_RUNNING;
    main_task->wait_pid = 0;
    main_task->page_directory = (uint32_t) page_directory; // 使用 Kernel 的初始分頁表
    main_task->cwd_lba = 0;
    main_task->total_ticks = 0;

    // 初始化環狀鏈結串列
    main_task->next = main_task;
    current_task = main_task;
    ready_queue = main_task;

    // ----------------------------------------------------
    // 階段 2：建立 idle_task (兜底任務)
    // 當系統中沒有任何 TASK_RUNNING 行程時，會執行此行程避免 CPU 掛起
    // ----------------------------------------------------
    idle_task = (task_t*) kmalloc(sizeof(task_t));
    idle_task->pid = KERNEL_IDLE_PID;
    idle_task->ppid = 0;
    strcpy(idle_task->name, "[kernel_idle]");
    idle_task->state = TASK_RUNNING;
    idle_task->wait_pid = 0;
    idle_task->page_directory = (uint32_t) page_directory;
    idle_task->cwd_lba = 0;
    idle_task->total_ticks = 0;

    // 為 idle_task 配置專屬的核心堆疊
    uint32_t *kstack_mem = (uint32_t*) kmalloc(PAGE_SIZE);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + PAGE_SIZE);
    idle_task->kernel_stack = (uint32_t) kstack;

    // 手動偽造中斷堆疊幀，假裝它剛被中斷，準備 IRET 回到 idle_loop
    *(--kstack) = (uint32_t) idle_loop; // EIP
    for(int i = 0; i < 8; i++) *(--kstack) = 0; // Pusha (通用暫存器)
    *(--kstack) = EFLAGS_DEFAULT;       // EFLAGS 開啟中斷

    idle_task->esp = (uint32_t) kstack;
}

void create_user_task(uint32_t entry_point, uint32_t user_stack_top) {
    // ----------------------------------------------------
    // 階段 1：建立 PCB 與基礎資料
    // ----------------------------------------------------
    task_t *new_task = (task_t*) kmalloc(sizeof(task_t));
    new_task->pid = next_pid++;
    new_task->ppid = current_task->pid;
    strcpy(new_task->name, "init"); // 系統第一隻行程固定命名為 init

    new_task->state = TASK_RUNNING;
    new_task->wait_pid = 0;
    new_task->page_directory = current_task->page_directory;
    new_task->cwd_lba = 0;
    new_task->total_ticks = 0;

    // ----------------------------------------------------
    // 階段 2：初始化 User Heap (給 malloc 等函式使用)
    // 預分配 1MB (256 pages) 的物理記憶體並映射
    // ----------------------------------------------------
    for (int i = 0; i < USER_HEAP_PAGES; i++) {
        uint32_t heap_phys = (uint32_t)pmm_alloc_page();
        map_page(USER_HEAP_START + (i * PAGE_SIZE), heap_phys, 7); // 7 = User+RW+Present
    }
    new_task->heap_start = USER_HEAP_START;
    new_task->heap_end = USER_HEAP_START;

    // ----------------------------------------------------
    // 階段 3：準備 Kernel Stack 與 User Stack
    // ----------------------------------------------------
    uint32_t *kstack_mem = (uint32_t*) kmalloc(PAGE_SIZE);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + PAGE_SIZE);
    new_task->kernel_stack = (uint32_t) kstack;

    // 偽造 C 語言的 entry_point(int argc, char** argv) 參數佈局
    uint32_t *ustack = (uint32_t*) (user_stack_top - USER_STACK_PADDING);
    *(--ustack) = 0;                    // NULL 結尾
    uint32_t argv_ptr = (uint32_t) ustack;
    *(--ustack) = argv_ptr;             // char** argv
    *(--ustack) = 0;                    // int argc = 0
    *(--ustack) = 0;                    // Return address (User mode 不能直接 return)

    // ----------------------------------------------------
    // 階段 4：手造中斷返回幀 (IRET Frame) 實現 Ring0 -> Ring3 切換
    // ----------------------------------------------------
    *(--kstack) = USER_DS;              // User SS
    *(--kstack) = (uint32_t)ustack;     // User ESP
    *(--kstack) = EFLAGS_DEFAULT;       // EFLAGS (IF=1)
    *(--kstack) = USER_CS;              // User CS
    *(--kstack) = entry_point;          // User EIP

    // 偽造被中斷時的通用暫存器 (供 child_ret_stub 彈出)
    *(--kstack) = 0; // EBP
    *(--kstack) = 0; // EBX
    *(--kstack) = 0; // ESI
    *(--kstack) = 0; // EDI

    *(--kstack) = (uint32_t) child_ret_stub; // 讓 switch_task 執行 ret 時跳轉到這裡

    for(int i = 0; i < 8; i++) *(--kstack) = 0; // 再給 switch_task 的 popa 準備的空間
    *(--kstack) = EFLAGS_DEFAULT;

    new_task->esp = (uint32_t) kstack;

    // 插入環狀鏈結串列
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

    task_t *curr = (task_t*)current_task;
    task_t *next_node = curr->next;

    // ----------------------------------------------------
    // 階段 1：垃圾回收 (Garbage Collection)
    // 掃描鏈結串列，將所有標記為 TASK_DEAD 的行程資源徹底釋放
    // ----------------------------------------------------
    while (next_node != current_task) {
        if (next_node->state == TASK_DEAD) {
            // 從鏈結串列中剔除
            curr->next = next_node->next;

            // 釋放 Kernel Stack 記憶體 (回到當初 kmalloc 取得的起始位址)
            if (next_node->kernel_stack != 0) {
                kfree((void*)(next_node->kernel_stack - PAGE_SIZE));
            }
            // 釋放 PCB 結構本身
            kfree((void*)next_node);

            next_node = curr->next;
        } else {
            curr = next_node;
            next_node = curr->next;
        }
    }

    // ----------------------------------------------------
    // 階段 2：尋找下一個準備就緒 (RUNNING) 的行程
    // ----------------------------------------------------
    task_t *next_run = current_task->next;
    while (next_run->state != TASK_RUNNING && next_run != current_task) {
        next_run = next_run->next;
    }

    // 如果繞了一圈發現大家都還在等 (Waiting/Zombie)，那就派 idle_task 上場
    if (next_run->state != TASK_RUNNING) {
        next_run = idle_task;
    }

    // ----------------------------------------------------
    // 階段 3：執行 Context Switch
    // ----------------------------------------------------
    task_t *prev = (task_t*)current_task;
    current_task = next_run;

    if (current_task != prev) {
        // 更新 TSS 中的 ESP0，確保下次發生中斷時，能使用正確的 Kernel Stack
        if (current_task->kernel_stack != 0) {
            set_kernel_stack(current_task->kernel_stack);
        }
        // 切換堆疊與位址空間 (CR3)
        switch_task((uint32_t*)&prev->esp, (uint32_t*)&current_task->esp, current_task->page_directory);
    }
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
    if (pid <= 1) return -1;

    // 因為牽涉到跨行程狀態修改，鎖定中斷避免 Race Condition
    __asm__ volatile("cli");
    task_t *temp = (task_t*)current_task;
    int found = 0;

    // 走訪所有行程尋找目標
    do {
        if (temp->pid == pid && temp->state != TASK_DEAD) {
            // 1. 強制關閉它擁有的 GUI 視窗
            gui_close_windows_by_pid(pid);

            // 2. 檢查目標的父行程是否正在 wait() 它
            volatile task_t *parent = current_task;
            int parent_waiting = 0;
            do {
                if (parent->pid == temp->ppid && parent->state == TASK_WAITING && parent->wait_pid == pid) {
                    parent->state = TASK_RUNNING; // 喚醒父行程
                    parent->wait_pid = 0;
                    parent_waiting = 1;
                    break;
                }
                parent = parent->next;
            } while (parent != current_task);

            // 3. 根據父行程狀態決定它是變殭屍還是直接死透
            if (parent_waiting) {
                temp->state = TASK_ZOMBIE;
            } else {
                temp->state = TASK_DEAD;
            }

            found = 1;
            break;
        }
        temp = temp->next;
    } while (temp != current_task);

    __asm__ volatile("sti");

    return found ? 0 : -1;
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
