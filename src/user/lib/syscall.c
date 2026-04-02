/**
 * @file src/user/lib/syscall.c
 * @brief Main logic and program flow for syscall.c.
 *
 * This file handles the operations and logic associated with syscall.c.
 */

#include "syscall.h"

// 萬用系統呼叫封裝 (支援最多 5 個參數)
int syscall(int num, int p1, int p2, int p3, int p4, int p5) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a" (ret)
        : "a" (num), "b" (p1), "c" (p2), "d" (p3), "S" (p4), "D" (p5)
        : "memory"
    );
    return ret;
}
