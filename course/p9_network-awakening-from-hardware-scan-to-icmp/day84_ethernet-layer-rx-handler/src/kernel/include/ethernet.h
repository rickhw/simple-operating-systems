#ifndef ETHERNET_H
#define ETHERNET_H

#include <stdint.h>

// 乙太網路標頭 (長度剛好 14 bytes)
typedef struct {
    uint8_t  dest_mac[6]; // 目標 MAC 位址
    uint8_t  src_mac[6];  // 來源 MAC 位址
    uint16_t ethertype;   // 協定類型 (如 IPv4, ARP)
} __attribute__((packed)) ethernet_header_t;

// 常見的 EtherType (Big-Endian)
#define ETHERTYPE_IPv4 0x0800
#define ETHERTYPE_ARP  0x0806

// 網路端與主機端的 Endianness 轉換
static inline uint16_t ntohs(uint16_t netshort) {
    return ((netshort & 0xFF) << 8) | ((netshort & 0xFF00) >> 8);
}
#define htons ntohs

#endif
