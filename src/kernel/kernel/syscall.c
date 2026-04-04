/**
 * @file src/kernel/kernel/syscall.c
 * @brief System call handling and dispatcher.
 */

#include "syscall.h"
#include "tty.h"
#include "utils.h"
#include "simplefs.h"
#include "keyboard.h"
#include "task.h"
#include "gui.h"
#include "vfs.h"
#include "pmm.h"
#include "paging.h"
#include "icmp.h"

#define MAX_IPC_MESSAGES 16
#define MAX_FD           32

extern void read_time(int* h, int* m);

typedef struct {
    char data[128]; // Each message is at most 128 bytes
} ipc_msg_t;

fs_node_t* fd_table[MAX_FD] = {0};

// Circular Buffer for IPC mailbox
ipc_msg_t mailbox_queue[MAX_IPC_MESSAGES];
int mb_head = 0;  // Read head
int mb_tail = 0;  // Write tail
int mb_count = 0; // Current number of messages in the mailbox

/** @brief Lock IPC mailbox (disable interrupts). */
void ipc_lock() {
    __asm__ volatile("cli");
}

/** @brief Unlock IPC mailbox (enable interrupts). */
void ipc_unlock() {
    __asm__ volatile("sti");
}

/** @brief Handle file system related system calls. */
static void handle_fs_syscalls(uint32_t eax, registers_t *regs) {
    // 取得當前行程的工作目錄 (如果為 0 則預設為根目錄 LBA 1)
    uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;

    switch (eax) {
        case SYS_OPEN: {
            char* filename = (char*)regs->ebx;
            fs_node_t* node = simplefs_find(current_dir, filename);
            if (node == 0) { regs->eax = -1; return; }

            // 尋找空閒的檔案描述符 (File Descriptor, FD)
            // FD 0,1,2 通常保留給 stdin, stdout, stderr
            for (int i = 3; i < MAX_FD; i++) {
                if (fd_table[i] == 0) {
                    fd_table[i] = node;
                    regs->eax = i;
                    return;
                }
            }
            regs->eax = -1; // FD 表已滿
            break;
        }
        case SYS_READ: {
            int fd = (int)regs->ebx;
            uint8_t* buffer = (uint8_t*)regs->ecx;
            uint32_t size = (uint32_t)regs->edx;

            // 檢查 FD 的合法性並執行 VFS 讀取
            if (fd >= 3 && fd < MAX_FD && fd_table[fd] != 0) {
                regs->eax = vfs_read(fd_table[fd], 0, size, buffer);
            } else regs->eax = -1;
            break;
        }
        case SYS_CREATE:
            regs->eax = vfs_create_file(current_dir, (char*)regs->ebx, (char*)regs->ecx);
            break;
        case SYS_READDIR:
            regs->eax = simplefs_readdir(current_dir, (int)regs->ebx, (char*)regs->ecx, (uint32_t*)regs->edx, (uint32_t*)regs->esi);
            break;
        case SYS_REMOVE:
            regs->eax = vfs_delete_file(current_dir, (char*)regs->ebx);
            break;
        case SYS_MKDIR:
            regs->eax = vfs_mkdir(current_dir, (char*)regs->ebx);
            break;
        case SYS_CHDIR: {
            char* dirname = (char*)regs->ebx;

            // 處理回到根目錄的特例
            if (strcmp(dirname, "/") == 0) {
                current_task->cwd_lba = 1;
                regs->eax = 0;
            } else {
                // 嘗試解析目標目錄的 LBA 並更新當前行程的 PCB
                uint32_t target_lba = simplefs_get_dir_lba(current_dir, dirname);
                if (target_lba != 0) {
                    current_task->cwd_lba = target_lba;
                    regs->eax = 0;
                } else {
                    regs->eax = -1; // 目錄不存在
                }
            }
            break;
        }
        case SYS_GETCWD:
            regs->eax = vfs_getcwd(current_dir, (char*)regs->ebx);
            break;
    }
}

/** @brief Handle task management related system calls. */
static void handle_task_syscalls(uint32_t eax, registers_t *regs) {
    switch (eax) {
        case SYS_YIELD:    schedule(); break;
        case SYS_EXIT:     exit_task(); break;
        case SYS_FORK:     regs->eax = sys_fork(regs); break;
        case SYS_EXEC:     regs->eax = sys_exec(regs); break;
        case SYS_WAIT:     regs->eax = sys_wait(regs->ebx); break;
        case SYS_GETPID:   regs->eax = current_task->pid; break;
        case SYS_GETPROCS: regs->eax = task_get_process_list((process_info_t*)regs->ebx, (int)regs->ecx); break;
        case SYS_KILL:     regs->eax = sys_kill((uint32_t)regs->ebx); break;
    }
}

/** @brief Handle Inter-Process Communication (IPC) related system calls. */
static void handle_ipc_syscalls(uint32_t eax, registers_t *regs) {
    if (eax == SYS_IPC_SEND) {
        // [生產者] 將訊息推入環狀緩衝區
        if (mb_count < MAX_IPC_MESSAGES) {
            strcpy(mailbox_queue[mb_tail].data, (char*)regs->ebx);
            mb_tail = (mb_tail + 1) % MAX_IPC_MESSAGES;
            mb_count++;
            regs->eax = 0;
        } else regs->eax = -1; // 緩衝區已滿
    } else if (eax == SYS_IPC_RECV) {
        // [消費者] 從環狀緩衝區提取訊息
        if (mb_count > 0) {
            strcpy((char*)regs->ebx, mailbox_queue[mb_head].data);
            mb_head = (mb_head + 1) % MAX_IPC_MESSAGES;
            mb_count--;
            regs->eax = 1; // 成功讀取
        } else regs->eax = 0; // 沒有新訊息
    }
}

/** @brief Handle Graphical User Interface (GUI) related system calls. */
static void handle_gui_syscalls(uint32_t eax, registers_t *regs) {
    if (eax == SYS_CLEAR_SCREEN) {
        terminal_initialize();
        gui_redraw();
        regs->eax = 0;
    } else if (eax == SYS_CREATE_WINDOW) {
        // 簡單的視窗級聯 (Cascading) 效果，每次新建視窗位置偏移 30px
        static int win_offset = 0;
        int x = 200 + win_offset;
        int y = 150 + win_offset;
        win_offset = (win_offset + 30) % 200;

        regs->eax = create_window(x, y, (int)regs->ecx, (int)regs->edx, (char*)regs->ebx, current_task->pid);
        gui_redraw();
    } else if (eax == SYS_UPDATE_WINDOW) {
        window_t* win = get_window((int)regs->ebx);

        // 安全性檢查：確保視窗存在，且呼叫者是該視窗的擁有者
        if (win != 0 && win->owner_pid == current_task->pid && win->canvas != 0) {
            // 扣除視窗邊框與標題列的空間後，拷貝 User Space 的像素資料到 Window Canvas
            memcpy(win->canvas, (uint32_t*)regs->ecx, (win->width - 4) * (win->height - 24) * 4);
            gui_redraw();
        }
        regs->eax = 0;
    } else if (eax == SYS_SET_DISPLAY_MODE) {
        switch_display_mode((int)regs->ebx);
        regs->eax = 0;
    }
}

/**
 * @brief Main system call dispatcher.
 * @param regs The current register state.
 *
 * Flow: Ring 3 (int 0x80) -> isr128 (asm) -> syscall_handler(regs)
 */
void syscall_handler(registers_t *regs) {
    uint32_t eax = regs->eax;

    // ==========================================
    // [Fast Path] 無鎖系統呼叫 (Lockless Syscalls)
    // 對於不會改變全域敏感狀態的呼叫，我們不鎖定中斷，以提高效能
    // ==========================================
    switch(eax) {
        case SYS_PRINT_STR_LEN:
            kprintf((char*)regs->ebx, regs->ecx, regs->edx, regs->esi);
            return;
        case SYS_PRINT_STR:
            kprintf((char*)regs->ebx);
            return;
        case SYS_GETCHAR: {
            char c = keyboard_getchar();
            char str[2] = {c, '\0'};
            kprintf(str);
            regs->eax = (uint32_t)c;
            return;
        }
        case SYS_SBRK: {
            // User Space 請求擴展 Heap 空間
            uint32_t old_end = current_task->heap_end;
            current_task->heap_end += (int)regs->ebx;
            regs->eax = old_end;
            return;
        }
        case SYS_GET_MEM_INFO: {
            mem_info_t* info = (mem_info_t*)regs->ebx;
            ipc_lock();
            pmm_get_stats(&info->total_frames, &info->used_frames, &info->free_frames);
            info->active_universes = paging_get_active_universes();
            regs->eax = 0;
            ipc_unlock();
            return;
        }
        case SYS_GET_TIME: {
            int* h = (int*)regs->ebx;
            int* m = (int*)regs->ecx;
            ipc_lock();
            read_time(h, m);
            regs->eax = 0;
            ipc_unlock();
            return;
        }
        case SYS_GET_WIN_EVENT: {
            int win_id = (int)regs->ebx;
            int* out_x = (int*)regs->ecx;
            int* out_y = (int*)regs->edx;

            ipc_lock();
            window_t* win = get_window(win_id);
            // 安全性檢查：確保只能讀取屬於自己的視窗事件
            if (win != 0 && win->owner_pid == current_task->pid && win->has_new_click) {
                *out_x = win->last_click_x;
                *out_y = win->last_click_y;
                win->has_new_click = 0; // 事件已讀取，降下 Flag
                regs->eax = 1;
            } else {
                regs->eax = 0;
            }
            ipc_unlock();
            return;
        }
        case SYS_GET_WIN_KEY: {
            int win_id = (int)regs->ebx;
            char* out_key = (char*)regs->ecx;

            ipc_lock();
            window_t* win = get_window(win_id);
            if (win != 0 && win->owner_pid == current_task->pid && win->has_new_key) {
                *out_key = win->last_key;
                win->has_new_key = 0; // 事件已讀取，降下 Flag
                regs->eax = 1;
            } else {
                regs->eax = 0;
            }
            ipc_unlock();
            return;
        }
        case SYS_PING: {
            uint8_t* target_ip = (uint8_t*)regs->ebx;
            // 由於發送封包可能耗時，我們不鎖住整個 IPC，直接呼叫網路層
            ping_send_request(target_ip);
            regs->eax = 0;
            return;
        }
    }

    // ==========================================
    // [Slow Path] 需要鎖定的系統呼叫
    // 牽涉到 File System, Task 排程等會修改全域狀態的操作
    // ==========================================
    ipc_lock();
    handle_fs_syscalls(eax, regs);
    handle_task_syscalls(eax, regs);
    handle_ipc_syscalls(eax, regs);
    handle_gui_syscalls(eax, regs);
    ipc_unlock();
}
