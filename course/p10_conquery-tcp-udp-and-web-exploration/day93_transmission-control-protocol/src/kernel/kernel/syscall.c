/**
 * @file src/kernel/kernel/syscall.c
 * @brief 系統呼叫分派器與實作 (System Call Dispatcher & Implementation)
 *
 * 本檔案實作了 Ring 3 與 Ring 0 之間的介面。當 User 程式執行 int 0x80 時，
 * 會跳轉到這裡的 syscall_handler。我們採用分派表 (Dispatch Table) 機制
 * 以提升擴充性與可讀性。
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
#define MAX_SYSCALLS     128

extern void read_time(int* h, int* m);

/** @brief IPC 訊息結構 */
typedef struct {
    char data[128]; // 每個訊息上限 128 位元組
} ipc_msg_t;

/** @brief 全域檔案描述符表 (目前為簡化版全域共享) */
static fs_node_t* fd_table[MAX_FD] = {0};

/** @brief IPC 信箱環狀緩衝區 */
static ipc_msg_t mailbox_queue[MAX_IPC_MESSAGES];
static int mb_head = 0;  // 讀取頭
static int mb_tail = 0;  // 寫入尾
static int mb_count = 0; // 當前訊息數

/** @brief 鎖定系統關鍵區域 (關閉中斷) */
static void syscall_lock() {
    __asm__ volatile("cli");
}

/** @brief 解除鎖定系統關鍵區域 (開啟中斷) */
static void syscall_unlock() {
    __asm__ volatile("sti");
}

// =============================================================================
// 系統呼叫處理常式 (Syscall Handlers)
// 每一個 sys_* 函式對應一個特定的系統服務
// =============================================================================

/** @brief 格式化輸出字串到終端機 (SYS_PRINT_STR_LEN) */
static int sys_print_str_len(registers_t *regs) {
    kprintf((char*)regs->ebx, regs->ecx, regs->edx, regs->esi);
    return 0;
}

/** @brief 輸出字串到終端機 (SYS_PRINT_STR) */
static int sys_print_str(registers_t *regs) {
    kprintf((char*)regs->ebx);
    return 0;
}

/** @brief 開啟檔案 (SYS_OPEN) */
static int sys_open(registers_t *regs) {
    uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;
    char* filename = (char*)regs->ebx;
    fs_node_t* node = simplefs_find(current_dir, filename);
    if (node == 0) return -1;

    // 分配 FD
    for (int i = 3; i < MAX_FD; i++) {
        if (fd_table[i] == 0) {
            fd_table[i] = node;
            return i;
        }
    }
    return -1;
}

/** @brief 讀取檔案內容 (SYS_READ) */
static int sys_read(registers_t *regs) {
    int fd = (int)regs->ebx;
    uint8_t* buffer = (uint8_t*)regs->ecx;
    uint32_t size = (uint32_t)regs->edx;

    if (fd >= 3 && fd < MAX_FD && fd_table[fd] != 0) {
        return vfs_read(fd_table[fd], 0, size, buffer);
    }
    return -1;
}

/** @brief 從鍵盤讀取一個字元 (SYS_GETCHAR) */
static int sys_getchar_handler(registers_t *regs) {
    (void)regs;
    char c = keyboard_getchar();
    char str[2] = {c, '\0'};
    kprintf(str); // 回顯到螢幕
    return (int)c;
}

/** @brief 自願放棄 CPU 執行權 (SYS_YIELD) */
static int sys_yield_handler(registers_t *regs) {
    (void)regs;
    schedule();
    return 0;
}

/** @brief 結束目前行程 (SYS_EXIT) */
static int sys_exit_handler(registers_t *regs) {
    (void)regs;
    exit_task();
    return 0;
}

/** @brief 建立子行程 (SYS_FORK) */
static int sys_fork_handler(registers_t *regs) {
    return sys_fork(regs);
}

/** @brief 載入並執行新程式 (SYS_EXEC) */
static int sys_exec_handler(registers_t *regs) {
    return sys_exec(regs);
}

/** @brief 等待子行程結束 (SYS_WAIT) */
static int sys_wait_handler(registers_t *regs) {
    return sys_wait(regs->ebx);
}

/** @brief 發送 IPC 訊息 (SYS_IPC_SEND) */
static int sys_ipc_send(registers_t *regs) {
    if (mb_count < MAX_IPC_MESSAGES) {
        strcpy(mailbox_queue[mb_tail].data, (char*)regs->ebx);
        mb_tail = (mb_tail + 1) % MAX_IPC_MESSAGES;
        mb_count++;
        return 0;
    }
    return -1;
}

/** @brief 接收 IPC 訊息 (SYS_IPC_RECV) */
static int sys_ipc_recv(registers_t *regs) {
    if (mb_count > 0) {
        strcpy((char*)regs->ebx, mailbox_queue[mb_head].data);
        mb_head = (mb_head + 1) % MAX_IPC_MESSAGES;
        mb_count--;
        return 1;
    }
    return 0;
}

/** @brief 擴充使用者堆積空間 (SYS_SBRK) */
static int sys_sbrk_handler(registers_t *regs) {
    uint32_t old_end = current_task->heap_end;
    current_task->heap_end += (int)regs->ebx;
    return old_end;
}

/** @brief 建立新檔案 (SYS_CREATE) */
static int sys_create_handler(registers_t *regs) {
    uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;
    return vfs_create_file(current_dir, (char*)regs->ebx, (char*)regs->ecx);
}

/** @brief 讀取目錄內容 (SYS_READDIR) */
static int sys_readdir_handler(registers_t *regs) {
    uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;
    return simplefs_readdir(current_dir, (int)regs->ebx, (char*)regs->ecx, (uint32_t*)regs->edx, (uint32_t*)regs->esi);
}

/** @brief 刪除檔案 (SYS_REMOVE) */
static int sys_remove_handler(registers_t *regs) {
    uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;
    return vfs_delete_file(current_dir, (char*)regs->ebx);
}

/** @brief 建立新目錄 (SYS_MKDIR) */
static int sys_mkdir_handler(registers_t *regs) {
    uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;
    return vfs_mkdir(current_dir, (char*)regs->ebx);
}

/** @brief 切換目前工作目錄 (SYS_CHDIR) */
static int sys_chdir_handler(registers_t *regs) {
    uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;
    char* dirname = (char*)regs->ebx;

    if (strcmp(dirname, "/") == 0) {
        current_task->cwd_lba = 1;
        return 0;
    } else {
        uint32_t target_lba = simplefs_get_dir_lba(current_dir, dirname);
        if (target_lba != 0) {
            current_task->cwd_lba = target_lba;
            return 0;
        }
    }
    return -1;
}

/** @brief 取得目前工作目錄路徑 (SYS_GETCWD) */
static int sys_getcwd_handler(registers_t *regs) {
    uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;
    return vfs_getcwd(current_dir, (char*)regs->ebx);
}

/** @brief 取得當前 PID (SYS_GETPID) */
static int sys_getpid_handler(registers_t *regs) {
    (void)regs;
    return current_task->pid;
}

/** @brief 取得系統行程列表 (SYS_GETPROCS) */
static int sys_getprocs_handler(registers_t *regs) {
    return task_get_process_list((process_info_t*)regs->ebx, (int)regs->ecx);
}

/** @brief 清除畫面 (SYS_CLEAR_SCREEN) */
static int sys_clear_screen_handler(registers_t *regs) {
    (void)regs;
    terminal_initialize();
    gui_redraw();
    return 0;
}

/** @brief 殺死特定行程 (SYS_KILL) */
static int sys_kill_handler(registers_t *regs) {
    return sys_kill((uint32_t)regs->ebx);
}

/** @brief 取得記憶體使用狀態 (SYS_GET_MEM_INFO) */
static int sys_get_mem_info_handler(registers_t *regs) {
    mem_info_t* info = (mem_info_t*)regs->ebx;
    pmm_get_stats(&info->total_frames, &info->used_frames, &info->free_frames);
    info->active_universes = paging_get_active_universes();
    return 0;
}

/** @brief 建立圖形視窗 (SYS_CREATE_WINDOW) */
static int sys_create_window_handler(registers_t *regs) {
    static int win_offset = 0;
    int x = 200 + win_offset;
    int y = 150 + win_offset;
    win_offset = (win_offset + 30) % 200;

    int win_id = create_window(x, y, (int)regs->ecx, (int)regs->edx, (char*)regs->ebx, current_task->pid);
    gui_redraw();
    return win_id;
}

/** @brief 更新視窗內容 (SYS_UPDATE_WINDOW) */
static int sys_update_window_handler(registers_t *regs) {
    window_t* win = get_window((int)regs->ebx);
    if (win != 0 && win->owner_pid == current_task->pid && win->canvas != 0) {
        memcpy(win->canvas, (uint32_t*)regs->ecx, (win->width - 4) * (win->height - 24) * 4);
        gui_redraw();
    }
    return 0;
}

/** @brief 取得系統時間 (SYS_GET_TIME) */
static int sys_get_time_handler(registers_t *regs) {
    read_time((int*)regs->ebx, (int*)regs->ecx);
    return 0;
}

/** @brief 取得視窗點擊事件 (SYS_GET_WIN_EVENT) */
static int sys_get_win_event_handler(registers_t *regs) {
    int win_id = (int)regs->ebx;
    int* out_x = (int*)regs->ecx;
    int* out_y = (int*)regs->edx;

    window_t* win = get_window(win_id);
    if (win != 0 && win->owner_pid == current_task->pid && win->has_new_click) {
        *out_x = win->last_click_x;
        *out_y = win->last_click_y;
        win->has_new_click = 0;
        return 1;
    }
    return 0;
}

/** @brief 取得視窗按鍵事件 (SYS_GET_WIN_KEY) */
static int sys_get_win_key_handler(registers_t *regs) {
    int win_id = (int)regs->ebx;
    char* out_key = (char*)regs->ecx;

    window_t* win = get_window(win_id);
    if (win != 0 && win->owner_pid == current_task->pid && win->has_new_key) {
        *out_key = win->last_key;
        win->has_new_key = 0;
        return 1;
    }
    return 0;
}

/** @brief 發送 ICMP Ping 請求 (SYS_PING) */
static int sys_ping_handler(registers_t *regs) {
    ping_send_request((uint8_t*)regs->ebx);
    return 0;
}

// ==========================================
// 32 號 Syscall：UDP 發送 (ebx=IP陣列, ecx=Port, edx=字串)
// ==========================================
static int sys_send_udp_handler(registers_t *regs) {
    uint8_t* ip = (uint8_t*)regs->ebx;
    uint16_t port = (uint16_t)regs->ecx;
    char* msg = (char*)regs->edx;
    int len = strlen(msg);

    extern void udp_send_packet(uint8_t* dest_ip, uint16_t src_port, uint16_t dest_port, uint8_t* data, uint32_t len);

    // 我們將來源 Port 寫死為 12345，目標 Port 由 User 決定
    udp_send_packet(ip, 12345, port, (uint8_t*)msg, len);

    regs->eax = 0;
    return;
}

// SYS_NET_UDP_BIND
static int sys_net_udp_bind_handler(registers_t *regs) {
    extern void udp_bind_port(uint16_t port);
    udp_bind_port((uint16_t)regs->ebx);
    regs->eax = 0;
    return;
}

// SYS_NET_UDP_RECV
static int sys_net_udp_recv_handler(registers_t *regs) {
    extern int udp_recv_data(char* buffer);
    regs->eax = udp_recv_data((char*)regs->ebx); // 回傳 1 或 0
    return;
}

static int sys_net_tcp_connect_handler(registers_t *regs) {
    extern void tcp_send_syn(uint8_t* dest_ip, uint16_t src_port, uint16_t dest_port);
    // 使用固定的本機 Port 54321 去撞目標的 Port
    tcp_send_syn((uint8_t*)regs->ebx, 54321, (uint16_t)regs->ecx);
    regs->eax = 0;
    return;
}



/** @brief 切換顯示模式 (SYS_SET_DISPLAY_MODE) */
static int sys_set_display_mode_handler(registers_t *regs) {
    switch_display_mode((int)regs->ebx);
    return 0;
}

// =============================================================================
// 系統呼叫分派表 (Syscall Dispatch Table)
// 將編號映射到處理函式
// =============================================================================

static syscall_t syscall_table[MAX_SYSCALLS] = {
    [SYS_PRINT_STR_LEN]   = sys_print_str_len,
    [SYS_PRINT_STR]       = sys_print_str,
    [SYS_OPEN]            = sys_open,
    [SYS_READ]            = sys_read,
    [SYS_GETCHAR]         = sys_getchar_handler,
    [SYS_YIELD]           = sys_yield_handler,
    [SYS_EXIT]            = sys_exit_handler,
    [SYS_FORK]            = sys_fork_handler,
    [SYS_EXEC]            = sys_exec_handler,
    [SYS_WAIT]            = sys_wait_handler,
    [SYS_IPC_SEND]        = sys_ipc_send,
    [SYS_IPC_RECV]        = sys_ipc_recv,
    [SYS_SBRK]            = sys_sbrk_handler,
    [SYS_CREATE]          = sys_create_handler,
    [SYS_READDIR]         = sys_readdir_handler,
    [SYS_REMOVE]          = sys_remove_handler,
    [SYS_MKDIR]           = sys_mkdir_handler,
    [SYS_CHDIR]           = sys_chdir_handler,
    [SYS_GETCWD]          = sys_getcwd_handler,
    [SYS_GETPID]          = sys_getpid_handler,
    [SYS_GETPROCS]        = sys_getprocs_handler,
    [SYS_CLEAR_SCREEN]    = sys_clear_screen_handler,
    [SYS_KILL]            = sys_kill_handler,
    [SYS_GET_MEM_INFO]    = sys_get_mem_info_handler,
    [SYS_CREATE_WINDOW]   = sys_create_window_handler,
    [SYS_UPDATE_WINDOW]   = sys_update_window_handler,
    [SYS_GET_TIME]        = sys_get_time_handler,
    [SYS_GET_WIN_EVENT]   = sys_get_win_event_handler,
    [SYS_GET_WIN_KEY]     = sys_get_win_key_handler,
    [SYS_PING]            = sys_ping_handler,
    [SYS_SEND_UDP]        = sys_send_udp_handler,
    [SYS_NET_UDP_BIND]    = sys_net_udp_bind_handler,
    [SYS_NET_UDP_RECV]    = sys_net_udp_recv_handler,
    [SYS_NET_TCP_CONNECT] = sys_net_tcp_connect_handler,
    [SYS_SET_DISPLAY_MODE] = sys_set_display_mode_handler,
};

/**
 * @brief 系統呼叫主要入口點 (Main Entry Point)
 *
 * 當 CPU 觸發中斷時，組合語言會呼叫此函式。
 * 它會檢查編號合法性、決定是否需要鎖定、分派任務並存回結果。
 */
void syscall_handler(registers_t *regs) {
    uint32_t syscall_num = regs->eax;

    // 檢查編號範圍與是否有對應的處理常式
    if (syscall_num >= MAX_SYSCALLS || syscall_table[syscall_num] == 0) {
        regs->eax = -1;
        return;
    }

    // [效能優化] 判斷是否需要全域鎖定。
    // 部分唯讀或不涉及關鍵資料修改的呼叫可以無鎖執行。
    int need_lock = 1;
    switch(syscall_num) {
        case SYS_PRINT_STR_LEN:
        case SYS_PRINT_STR:
        case SYS_GETCHAR:
        case SYS_SBRK:
        case SYS_PING:
            need_lock = 0;
            break;
    }

    if (need_lock) syscall_lock();

    // 執行處理函式並將回傳值寫入 eax，這將成為 User 看到的 syscall 回傳值
    syscall_t handler = syscall_table[syscall_num];
    regs->eax = handler(regs);

    if (need_lock) syscall_unlock();
}

/** @brief 初始化系統呼叫子系統 (目前採靜態初始化) */
void init_syscalls(void) {
    // 未來若有動態註冊需求可在此擴充
}
