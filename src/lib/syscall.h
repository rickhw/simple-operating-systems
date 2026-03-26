#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

// 初始化系統呼叫
void init_syscalls(void);

// 實際處理系統呼叫的函式 (會由組合語言跳板呼叫)
// void syscall_handler(uint32_t syscall_num, uint32_t arg1, uint32_t arg2);

void syscall_handler(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp,
                     uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax);
#endif
