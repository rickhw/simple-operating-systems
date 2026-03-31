#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include "task.h"


typedef struct {
    unsigned int total_frames;
    unsigned int used_frames;
    unsigned int free_frames;
    unsigned int active_universes;
} mem_info_t;


void init_syscalls(void);
void syscall_handler(registers_t *regs);

#endif
