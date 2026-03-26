#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include "task.h"

void init_syscalls(void);
void syscall_handler(registers_t *regs);

#endif
