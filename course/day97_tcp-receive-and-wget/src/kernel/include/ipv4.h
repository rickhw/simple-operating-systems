#ifndef IPV4_H
#define IPV4_H

#include <stdint.h>
#include "ethernet.h"

typedef struct {
    uint8_t  ihl : 4;       // 標頭長度 (Internet Header Length)
    uint8_t  version : 4;   // 版本 (IPv4 = 4)
    uint8_t  tos;           // 服務類型 (Type of Service)
    uint16_t total_length;  // 總長度 (Header + Data)
    uint16_t ident;         // 識別碼
    uint16_t flags_frag;    // 標誌與片段偏移
    uint8_t  ttl;           // 存活時間 (Time to Live)
    uint8_t  protocol;      // 協定 (ICMP = 1, TCP = 6, UDP = 17)
    uint16_t checksum;      // 標頭檢查碼
    uint8_t  src_ip[4];     // 來源 IP
    uint8_t  dest_ip[4];    // 目標 IP
} __attribute__((packed)) ipv4_header_t;

#define IPV4_PROTO_ICMP 1
#define IPV4_PROTO_TCP  6
#define IPV4_PROTO_UDP  17

#endif
