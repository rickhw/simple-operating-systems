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

// ==========================================
// 呼叫 31 號 Syscall，並透過 ebx 傳遞 ip 指標
// ==========================================
void sys_ping(uint8_t* ip) {
    __asm__ volatile (
        "int $128"
        : // 沒有回傳值，所以空白
        : "a"(SYS_NET_PING), "b"((uint32_t)ip) // eax = 31, ebx = ip
        : "memory"
    );
}
