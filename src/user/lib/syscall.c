#include "syscall.h"

/**
 * @file syscall.c
 * @brief 系統呼叫底層實作 (透過 int $0x80 中斷與核心通訊)
 */

/**
 * @brief 通用系統呼叫封裝
 * @details 支援最多 5 個參數，分別透過 ebx, ecx, edx, esi, edi 暫存器傳遞。
 *          中斷號碼目前統一使用 0x80 (128)。
 */
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
