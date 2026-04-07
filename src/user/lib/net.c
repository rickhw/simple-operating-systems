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
