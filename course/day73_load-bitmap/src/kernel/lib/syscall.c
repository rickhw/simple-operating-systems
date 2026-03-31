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

#define MAX_MESSAGES 16 // IPC 訊息佇列 (Message Queue)
#define SYS_SET_DISPLAY_MODE 99 // 切換 desktop or terminal mode.

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

// --- Syscall Handlers ---

static void handle_fs_syscalls(uint32_t eax, registers_t *regs) {
    uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;

    if (eax == 3) { // open
        char* filename = (char*)regs->ebx;
        fs_node_t* node = simplefs_find(current_dir, filename);
        if (node == 0) { regs->eax = -1; return; }
        for (int i = 3; i < 32; i++) {
            if (fd_table[i] == 0) { fd_table[i] = node; regs->eax = i; return; }
        }
        regs->eax = -1;
    } else if (eax == 4) { // read
        int fd = (int)regs->ebx;
        uint8_t* buffer = (uint8_t*)regs->ecx;
        uint32_t size = (uint32_t)regs->edx;
        if (fd >= 3 && fd < 32 && fd_table[fd] != 0) {
            regs->eax = vfs_read(fd_table[fd], 0, size, buffer);
        } else regs->eax = -1;
    } else if (eax == 14) { // create
        regs->eax = vfs_create_file(current_dir, (char*)regs->ebx, (char*)regs->ecx);
    } else if (eax == 15) { // readdir
        regs->eax = simplefs_readdir(current_dir, (int)regs->ebx, (char*)regs->ecx, (uint32_t*)regs->edx, (uint32_t*)regs->esi);
    } else if (eax == 16) { // remove
        regs->eax = vfs_delete_file(current_dir, (char*)regs->ebx);
    } else if (eax == 17) { // mkdir
        regs->eax = vfs_mkdir(current_dir, (char*)regs->ebx);
    } else if (eax == 18) { // chdir
        char* dirname = (char*)regs->ebx;
        if (strcmp(dirname, "/") == 0) { current_task->cwd_lba = 1; regs->eax = 0; }
        else {
            uint32_t target_lba = simplefs_get_dir_lba(current_dir, dirname);
            if (target_lba != 0) { current_task->cwd_lba = target_lba; regs->eax = 0; }
            else regs->eax = -1;
        }
    } else if (eax == 19) { // getcwd
        regs->eax = vfs_getcwd(current_dir, (char*)regs->ebx);
    }
}

static void handle_task_syscalls(uint32_t eax, registers_t *regs) {
    if (eax == 6) schedule();
    else if (eax == 7) exit_task();
    else if (eax == 8) regs->eax = sys_fork(regs);
    else if (eax == 9) regs->eax = sys_exec(regs);
    else if (eax == 10) regs->eax = sys_wait(regs->ebx);
    else if (eax == 20) regs->eax = current_task->pid;
    else if (eax == 21) regs->eax = task_get_process_list((process_info_t*)regs->ebx, (int)regs->ecx);
    else if (eax == 24) regs->eax = sys_kill((int)regs->ebx);
}

static void handle_ipc_syscalls(uint32_t eax, registers_t *regs) {
    if (eax == 11) { // send
        if (mb_count < MAX_MESSAGES) {
            strcpy(mailbox_queue[mb_tail].data, (char*)regs->ebx);
            mb_tail = (mb_tail + 1) % MAX_MESSAGES;
            mb_count++;
            regs->eax = 0;
        } else regs->eax = -1;
    } else if (eax == 12) { // recv
        if (mb_count > 0) {
            strcpy((char*)regs->ebx, mailbox_queue[mb_head].data);
            mb_head = (mb_head + 1) % MAX_MESSAGES;
            mb_count--;
            regs->eax = 1;
        } else regs->eax = 0;
    }
}

static void handle_gui_syscalls(uint32_t eax, registers_t *regs) {
    if (eax == 22) { // clear screen
        terminal_initialize();
        gui_redraw();
        regs->eax = 0;
    } else if (eax == 26) { // create window
        static int win_offset = 0;
        int x = 200 + win_offset;
        int y = 150 + win_offset;
        win_offset = (win_offset + 30) % 200;
        regs->eax = create_window(x, y, (int)regs->ecx, (int)regs->edx, (char*)regs->ebx, current_task->pid);
        gui_redraw();
    } else if (eax == 27) { // update window
        window_t* win = get_window((int)regs->ebx);
        if (win != 0 && (uint32_t)win->owner_pid == current_task->pid && win->canvas != 0) {
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
 * [Syscall Dispatcher]
 * 
 * ASCII Flow:
 * Ring 3 (int 0x80) -> isr128 (asm) -> syscall_handler(regs)
 *                                         |
 *        +--------------------------------+--------------------------------+
 *        |                |                |               |               |
 *  handle_fs()     handle_task()     handle_ipc()    handle_gui()    handle_mem()
 */
void syscall_handler(registers_t *regs) {
    uint32_t eax = regs->eax;

    if (eax == 0) {
        kprintf((char*)regs->ebx, regs->ecx, regs->edx, regs->esi);
        return;
    }
    if (eax == 2) {
        kprintf((char*)regs->ebx);
        return;
    }
    if (eax == 5) {
        char c = keyboard_getchar();
        char str[2] = {c, '\0'};
        kprintf(str);
        regs->eax = (uint32_t)c;
        return;
    }
    if (eax == 13) { // sbrk
        uint32_t old_end = current_task->heap_end;
        current_task->heap_end += (int)regs->ebx;
        regs->eax = old_end;
        return;
    }
    if (eax == 25) { // get_mem_info
        mem_info_t* info = (mem_info_t*)regs->ebx;
        ipc_lock();
        pmm_get_stats(&info->total_frames, &info->used_frames, &info->free_frames);
        info->active_universes = paging_get_active_universes();
        regs->eax = 0;
        ipc_unlock();
        return;
    }

    ipc_lock();
    handle_fs_syscalls(eax, regs);
    handle_task_syscalls(eax, regs);
    handle_ipc_syscalls(eax, regs);
    handle_gui_syscalls(eax, regs);
    ipc_unlock();
}
