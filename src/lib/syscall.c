#include "syscall.h"
#include "tty.h"
#include "utils.h"
#include "simplefs.h"
#include "keyboard.h"
#include "task.h"

// 【修復】拔掉 extern，讓它真正在這個檔案裡被分配記憶體空間！
fs_node_t* fd_table[32] = {0};

// ==========================================
// 【新增】IPC 中央信箱 (Mailbox)
// ==========================================
char ipc_mailbox[256] = {0};
int mailbox_has_msg = 0;

void init_syscalls(void) {
    kprintf("System Calls initialized on Interrupt 0x80 (128).\n");
}

void syscall_handler(registers_t *regs) {
    // Accumulator Register: 函式回傳值或 Syscall 編號
    uint32_t eax = regs->eax;

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

    // [Day39] Add -- start
    // ==========================================
    // 【新增】IPC 系統呼叫
    // ==========================================
    else if (eax == 11) { // Syscall 11: sys_send (傳送訊息)
        char* msg = (char*)regs->ebx;
        strcpy(ipc_mailbox, msg); // 將 User 宇宙的字串複製到 Kernel 的信箱裡
        mailbox_has_msg = 1;
        regs->eax = 0;
    }
    else if (eax == 12) { // Syscall 12: sys_recv (接收訊息)
        char* buffer = (char*)regs->ebx;
        if (mailbox_has_msg) {
            strcpy(buffer, ipc_mailbox); // 將 Kernel 信箱的字串複製給另一個 User 宇宙
            mailbox_has_msg = 0;
            regs->eax = 1; // 回傳 1 代表有收到訊息
        } else {
            regs->eax = 0; // 回傳 0 代表信箱是空的
        }
    }
    // [Day39] Add -- end
}
