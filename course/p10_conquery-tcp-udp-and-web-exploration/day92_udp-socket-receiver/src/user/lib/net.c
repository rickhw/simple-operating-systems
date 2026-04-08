#include "net.h"
#include "syscall.h"

/**
 * @file net.c
 * @brief 網路工具與系統呼叫實作
 */

// 簡易的字串轉 IP 陣列函式 (例如 "10.0.2.2" -> {10, 0, 2, 2})
void parse_ip(char* str, uint8_t* ip) {
    int i = 0;
    int num = 0;
    while (*str) {
        if (*str == '.') {
            ip[i++] = num;
            num = 0;
        } else {
            num = num * 10 + (*str - '0');
        }
        str++;
    }
    ip[i] = num; // 存入最後一個數字
}

// 發送 Ping 系統呼叫 (編號 31)
void sys_ping(uint8_t* ip) {
    syscall(SYS_PING, (int)ip, 0, 0, 0, 0);
}


// syscall.c 實作：
void sys_udp_send(uint8_t* ip, uint16_t port, char* msg) {
    __asm__ volatile (
        "int $128"
        :
        : "a"(SYS_NET_UDP_SEND), "b"((uint32_t)ip), "c"((uint32_t)port), "d"((uint32_t)msg)
        : "memory"
    );
}
void sys_udp_bind(uint16_t port) {
    __asm__ volatile ("int $128" : : "a"(SYS_NET_UDP_BIND), "b"((uint32_t)port) : "memory");
}

int sys_udp_recv(char* buffer) {
    int ret;
    __asm__ volatile ("int $128" : "=a"(ret) : "a"(SYS_NET_UDP_RECV), "b"((uint32_t)buffer) : "memory");
    return ret;
}
