#ifndef DNS_H
#define DNS_H

#include <stdint.h>

typedef struct {
    uint16_t transaction_id; // 交易 ID (我們自己亂數定一個，例如 0x1234)
    uint16_t flags;          // 旗標 (查詢是 0x0100)
    uint16_t questions;      // 幾個問題？ (通常是 1)
    uint16_t answer_rrs;     // 幾個回答？ (發問時是 0)
    uint16_t authority_rrs;  // 授權記錄數量
    uint16_t additional_rrs; // 額外記錄數量
} __attribute__((packed)) dns_header_t;

// User Space 用的簡易 Endianness 轉換 (從 ethernet.h 抄過來的)
static inline uint16_t htons(uint16_t hostshort) {
    return ((hostshort & 0xFF) << 8) | ((hostshort & 0xFF00) >> 8);
}
static inline uint16_t ntohs(uint16_t netshort) { return htons(netshort); }

#endif
