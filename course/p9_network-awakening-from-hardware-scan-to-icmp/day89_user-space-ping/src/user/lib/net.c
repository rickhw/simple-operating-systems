#include <stdint.h>
#include "net.h"

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
