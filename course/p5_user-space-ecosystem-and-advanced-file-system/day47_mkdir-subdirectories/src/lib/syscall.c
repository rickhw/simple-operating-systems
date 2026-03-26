#include "syscall.h"
#include "tty.h"
#include "utils.h"
#include "simplefs.h"
#include "keyboard.h"
#include "task.h"

fs_node_t* fd_table[32] = {0};

extern int vfs_create_file(char* filename, char* content);
extern int vfs_readdir(int index, char* out_name, uint32_t* out_size, uint32_t* out_type);
extern int vfs_delete_file(char* filename);
extern int vfs_mkdir(char* dirname);

// ==========================================
// IPC 訊息佇列 (Message Queue)
// ==========================================
#define MAX_MESSAGES 16

typedef struct {
    char data[128]; // 每則訊息最大 128 bytes
} ipc_msg_t;

// 環狀佇列 (Circular Buffer)
ipc_msg_t mailbox_queue[MAX_MESSAGES];
int mb_head = 0;  // 讀取頭
int mb_tail = 0;  // 寫入尾
int mb_count = 0; // 目前信箱裡的信件數量

// Mutex for Single Processor (SMP)
// 【新增】核心同步鎖：利用開關中斷來保護 Critical Section
void ipc_lock() {
    __asm__ volatile("cli"); // 關閉中斷：誰都別想搶走我的 CPU！
}

void ipc_unlock() {
    __asm__ volatile("sti"); // 開啟中斷：我用完了，大家可以繼續排隊了。
}

void init_syscalls(void) {
    kprintf("System Calls initialized on Interrupt 0x80 (128).\n");
}

void syscall_handler(registers_t *regs) {
    // Accumulator Register: 函式回傳值或 Syscall 編號
    uint32_t eax = regs->eax;

    // kprintf("[syscall_handler] eax: [%d]\n", eax);

    if (eax == 2) {
        kprintf((char*)regs->ebx);
    }
    else if (eax == 3) {
        char* filename = (char*)regs->ebx;
        fs_node_t* node = simplefs_find(filename);
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
    // 【新增】IPC 系統呼叫
    // ==========================================
    // Syscall 11: sys_send (傳送訊息)
    else if (eax == 11) {
        char* msg = (char*)regs->ebx;

        ipc_lock(); // 🔒 上鎖！進入危險區域

        if (mb_count < MAX_MESSAGES) {
            strcpy(mailbox_queue[mb_tail].data, msg);
            mb_tail = (mb_tail + 1) % MAX_MESSAGES;
            mb_count++;
            regs->eax = 0;
        } else {
            regs->eax = -1; // Queue Full
        }

        ipc_unlock(); // 🔓 解鎖！離開危險區域
    }
    // Syscall 12: sys_recv (接收訊息)
    else if (eax == 12) {
        char* buffer = (char*)regs->ebx;

        // START OF CRITICAL SECTION
        ipc_lock(); // 🔒 上鎖！進入危險區域

        if (mb_count > 0) {
            strcpy(buffer, mailbox_queue[mb_head].data);
            mb_head = (mb_head + 1) % MAX_MESSAGES;
            mb_count--;
            regs->eax = 1;
        } else {
            regs->eax = 0;
        }

        ipc_unlock(); // 🔓 解鎖！離開危險區域
        // End OF CRITICAL SECTION
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

        ipc_lock(); // 上鎖，防止寫入硬碟時被排程器打斷！
        regs->eax = vfs_create_file(filename, content);
        ipc_unlock();
    }
    // Syscall 15: sys_readdir (讀取目錄內容)
    else if (eax == 15) {
        int index = (int)regs->ebx;
        char* out_name = (char*)regs->ecx;
        uint32_t* out_size = (uint32_t*)regs->edx;

        uint32_t* out_type = (uint32_t*)regs->esi; // 【新增】從 ESI 暫存器拿出 out_type 指標！

        ipc_lock();
        regs->eax = vfs_readdir(index, out_name, out_size, out_type);   // 加入 out_type
        ipc_unlock();
    }

    // Syscall 16: sys_remove (刪除檔案)
    else if (eax == 16) {
        char* filename = (char*)regs->ebx;

        ipc_lock(); // 上鎖！修改硬碟資料是非常危險的操作
        regs->eax = vfs_delete_file(filename);
        ipc_unlock();
    }

    // Syscall 17: sys_mkdir (建立目錄)
    else if (eax == 17) {
        char* dirname = (char*)regs->ebx;
        kprintf("[syscall_handler] sys_mkdir, filename: [%s]\n", dirname);

        ipc_lock();
        regs->eax = vfs_mkdir(dirname);
        ipc_unlock();
    }
}
