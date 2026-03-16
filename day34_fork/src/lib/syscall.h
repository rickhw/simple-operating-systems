#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include "task.h"

// 初始化系統呼叫
void init_syscalls(void);

void syscall_handler(registers_t *regs);

#endif
