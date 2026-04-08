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


/**
 * 因為 x86 CPU 是 Little-Endian（小端序），而網路世界的標準是 Big-Endian（大端序），
 * 所以我們必須手動把這 4 個 bytes 的順序完全反轉過來（從 A B C D 變成 D C B A）。
 * 這就是 htonl (Host to Network Long) 的工作。
 */

// ntohs: network to host short
// 說明：它的主要功能是將一個 16 位元（2 位元組） 的整數從 網路位元組順序（Network Byte Order） 轉換為 主機位元組順序（Host Byte Order）。
// 用途：通常用於處理從網路接收到的資料（如 TCP/UDP 埠號），因為網路通訊協定規定使用「大端序」（Big-Endian），而許多電腦（如 x86 架構）內部使用「小端序」（Little-Endian）。
// 參數與回傳值：它接收一個網路順序的 16 位元無符號短整數（uint16_t），並回傳該數值在當前主機系統上的正確順序。
// 平台差異：如果主機本身就使用大端序，此函式通常不執行任何操作；若主機使用小端序，它會將位元組順序反轉。
//
// htons: host to network short
// ntohl: network to host long
// htonl: host to network long
// more see: https://linux.die.net/man/3/ntohs
// 網路端與主機端的 Endianness 轉換
static inline uint16_t ntohs(uint16_t netshort) {
    return ((netshort & 0xFF) << 8) | ((netshort & 0xFF00) >> 8);
}
#define htons ntohs

// 【新增】網路端與主機端的 Endianness 轉換 (32-bit)
static inline uint32_t htonl(uint32_t hostlong) {
    return ((hostlong & 0x000000FF) << 24) |
           ((hostlong & 0x0000FF00) << 8)  |
           ((hostlong & 0x00FF0000) >> 8)  |
           ((hostlong & 0xFF000000) >> 24);
}
#define ntohl htonl

#endif
