/**
 * @file src/kernel/net/arp.c
 * @brief Main logic and program flow for arp.c.
 *
 * This file handles the operations and logic associated with arp.c.
 */

#include "arp.h"
#include "rtl8139.h"
#include "tty.h"
#include "utils.h" // 需要用到 memcpy

// 簡單的 ARP Table 結構
typedef struct {
    uint8_t ip[4];
    uint8_t mac[6];
    int is_valid;
} arp_entry_t;

#define ARP_TABLE_SIZE 16
static arp_entry_t arp_table[ARP_TABLE_SIZE];

// QEMU SLIRP 預設分配給我們虛擬機的 IP
static uint8_t my_ip[4] = {10, 0, 2, 15};

void arp_send_request(uint8_t* target_ip) {
    // 整個封包大小 = Ethernet 標頭 (14) + ARP 標頭 (28) = 42 Bytes
    uint8_t packet[sizeof(ethernet_header_t) + sizeof(arp_packet_t)];

    // 將陣列指標轉型，方便我們填寫欄位
    ethernet_header_t* eth = (ethernet_header_t*)packet;
    arp_packet_t* arp = (arp_packet_t*)(packet + sizeof(ethernet_header_t));

    uint8_t* my_mac = rtl8139_get_mac();

    // ==========================================
    // 1. 填寫 Ethernet Header (L2)
    // ==========================================
    for(int i=0; i<6; i++) {
        eth->dest_mac[i] = 0xFF; // 廣播給網路上所有人
        eth->src_mac[i] = my_mac[i];
    }
    eth->ethertype = htons(ETHERTYPE_ARP);

    // ==========================================
    // 2. 填寫 ARP Header (L2.5)
    // ==========================================
    arp->hardware_type = htons(0x0001); // Ethernet
    arp->protocol_type = htons(ETHERTYPE_IPv4);
    arp->hw_addr_len = 6;
    arp->proto_addr_len = 4;
    arp->opcode = htons(ARP_REQUEST);

    for(int i=0; i<6; i++) {
        arp->src_mac[i] = my_mac[i];
        arp->dest_mac[i] = 0x00; // 還不知道對方的 MAC，所以填 0
    }

    for(int i=0; i<4; i++) {
        arp->src_ip[i] = my_ip[i];
        arp->dest_ip[i] = target_ip[i];
    }

    // ==========================================
    // 3. 透過網卡發送！(雖然只有 42 bytes，但網卡會自動幫我們 padding 到 60 bytes)
    // ==========================================
    rtl8139_send_packet(packet, sizeof(packet));

    kprintf("[ARP] Broadcast Request sent: Who has [%d.%d.%d.%d]?\n",
            target_ip[0], target_ip[1], target_ip[2], target_ip[3]);
}


// 更新電話簿的 API
void arp_update_table(uint8_t* ip, uint8_t* mac) {
    // 尋找空位或已存在的 IP 並更新 MAC
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        if (!arp_table[i].is_valid || memcmp(arp_table[i].ip, ip, 4) == 0) {
            memcpy(arp_table[i].ip, ip, 4);
            memcpy(arp_table[i].mac, mac, 6);
            arp_table[i].is_valid = 1;
            kprintf("[ARP] Table Updated: %d.%d.%d.%d -> %x:%x:%x:%x:%x:%x\n",
                    ip[0], ip[1], ip[2], ip[3],
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            return;
        }
    }
}

// ==========================================
// 當收到別人的尋人啟事時，回傳我們的 MAC 位址！
// ==========================================
void arp_send_reply(uint8_t* target_ip, uint8_t* target_mac) {
    uint8_t packet[sizeof(ethernet_header_t) + sizeof(arp_packet_t)];
    ethernet_header_t* eth = (ethernet_header_t*)packet;
    arp_packet_t* arp = (arp_packet_t*)(packet + sizeof(ethernet_header_t));

    uint8_t* my_mac = rtl8139_get_mac();

    // 1. Ethernet Header (直接回傳給詢問者)
    memcpy(eth->dest_mac, target_mac, 6);
    memcpy(eth->src_mac, my_mac, 6);
    eth->ethertype = htons(ETHERTYPE_ARP);

    // 2. ARP Header
    arp->hardware_type = htons(0x0001);
    arp->protocol_type = htons(ETHERTYPE_IPv4);
    arp->hw_addr_len = 6;
    arp->proto_addr_len = 4;
    arp->opcode = htons(ARP_REPLY); // 【關鍵】這是一封 Reply (2)

    // 來源是我們自己
    memcpy(arp->src_mac, my_mac, 6);
    memcpy(arp->src_ip, my_ip, 4);

    // 目標是剛才發問的人
    memcpy(arp->dest_mac, target_mac, 6);
    memcpy(arp->dest_ip, target_ip, 4);

    rtl8139_send_packet(packet, sizeof(packet));
    kprintf("[ARP] Reply sent to %d.%d.%d.%d\n", target_ip[0], target_ip[1], target_ip[2], target_ip[3]);
}

// ==========================================
// 查詢電話簿 (ARP Lookup)
// ==========================================
uint8_t* arp_lookup(uint8_t* ip) {
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        if (arp_table[i].is_valid && memcmp(arp_table[i].ip, ip, 4) == 0) {
            return arp_table[i].mac; // 找到了，回傳 MAC！
        }
    }
    return 0; // 找不到
}
