#ifndef UDP_H
#define UDP_H

#include <stdint.h>

typedef struct {
    uint16_t src_port;   // 來源通訊埠
    uint16_t dest_port;  // 目標通訊埠
    uint16_t length;     // UDP 總長度 (標頭 + 資料)
    uint16_t checksum;   // 檢查碼
} __attribute__((packed)) udp_header_t;

void udp_send_packet(uint8_t* dest_ip, uint16_t src_port, uint16_t dest_port, uint8_t* data, uint32_t len);

#endif
