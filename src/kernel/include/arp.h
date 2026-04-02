#ifndef ARP_H
#define ARP_H

#include <stdint.h>
#include "ethernet.h"

#define ARP_REQUEST 1
#define ARP_REPLY   2

// ARP 封包結構 (IPv4 over Ethernet)
typedef struct {
    uint16_t hardware_type; // 0x0001 (Ethernet)
    uint16_t protocol_type; // 0x0800 (IPv4)
    uint8_t  hw_addr_len;   // 6 (MAC 位址長度)
    uint8_t  proto_addr_len;// 4 (IPv4 位址長度)
    uint16_t opcode;        // 1 代表 Request, 2 代表 Reply
    uint8_t  src_mac[6];    // 來源 MAC
    uint8_t  src_ip[4];     // 來源 IP
    uint8_t  dest_mac[6];   // 目標 MAC (Request 時填 0)
    uint8_t  dest_ip[4];    // 目標 IP
} __attribute__((packed)) arp_packet_t;


void arp_send_request(uint8_t* target_ip);
void arp_send_reply(uint8_t* target_ip, uint8_t* target_mac);
void arp_update_table(uint8_t* ip, uint8_t* mac);
uint8_t* arp_lookup(uint8_t* ip);

#endif
