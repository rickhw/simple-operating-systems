#ifndef TCP_H
#define TCP_H

#include <stdint.h>

// 1. 真實的 TCP 標頭
typedef struct {
    uint16_t src_port;     // 來源 Port
    uint16_t dest_port;    // 目標 Port
    uint32_t seq_num;      // 序號 (Sequence Number)
    uint32_t ack_num;      // 確認號 (Acknowledge Number)
    uint8_t  data_offset;  // 標頭長度 (高 4 bits) + 保留位元
    uint8_t  flags;        // 控制旗標 (SYN, ACK, FIN...)
    uint16_t window_size;  // 接收視窗大小
    uint16_t checksum;     // 檢查碼 (絕對不能為 0)
    uint16_t urgent_ptr;   // 緊急指標
} __attribute__((packed)) tcp_header_t;

// 2. 用來計算 Checksum 的「偽標頭」
typedef struct {
    uint8_t  src_ip[4];
    uint8_t  dest_ip[4];
    uint8_t  reserved;     // 永遠填 0
    uint8_t  protocol;     // 永遠填 6 (TCP)
    uint16_t tcp_length;   // TCP 標頭 + 資料的總長度
} __attribute__((packed)) tcp_pseudo_header_t;

// TCP 旗標定義 (Bitmask)
#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10

void tcp_send_syn(uint8_t* dest_ip, uint16_t src_port, uint16_t dest_port);
void tcp_send_ack(uint8_t* dest_ip, uint16_t src_port, uint16_t dest_port, uint32_t seq_num, uint32_t ack_num);

int tcp_recv_data(char* buffer);

#endif
