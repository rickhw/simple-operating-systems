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
