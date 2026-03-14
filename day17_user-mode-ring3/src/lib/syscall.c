#include "syscall.h"
#include "tty.h"
#include "utils.h"

void init_syscalls(void) {
    kprintf("System Calls initialized on Interrupt 0x80 (128).\n");
}

// 這是核心在防彈玻璃後面做事的邏輯
// 參數會由組合語言從 CPU 暫存器 (eax, ebx, ecx) 裡面撈出來傳給我們
void syscall_handler(uint32_t syscall_num, uint32_t arg1, uint32_t arg2) {
    switch (syscall_num) {
        case 1:
            // 假設 Syscall 1 是「印出數字」
            kprintf("[Kernel] Syscall 1 Executed. Arg: %d\n", arg1);
            break;

        case 2:
            // 假設 Syscall 2 是「印出字串」 (arg1 當作指標)
            kprintf("[Kernel] Syscall 2 Executed. String: %s\n", (char*)arg1);
            break;

        default:
            kprintf("[Kernel] Unknown System Call: %d\n", syscall_num);
            break;
    }

    // 為了避免編譯器警告 arg2 沒用到
    (void)arg2;
}
