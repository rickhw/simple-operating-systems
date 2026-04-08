#ifndef ICMP_H
#define ICMP_H

#include <stdint.h>
#include "ipv4.h"

typedef struct {
    uint8_t  type;          // 類型 (Echo Request = 8, Echo Reply = 0)
    uint8_t  code;          // 代碼 (通常為 0)
    uint16_t checksum;      // 檢查碼
    uint16_t identifier;    // 識別碼 (用來對應 Request/Reply)
    uint16_t sequence;      // 序號
} __attribute__((packed)) icmp_header_t;

#define ICMP_ECHO_REPLY   0
#define ICMP_ECHO_REQUEST 8

void ping_send_request(uint8_t* target_ip);

#endif
