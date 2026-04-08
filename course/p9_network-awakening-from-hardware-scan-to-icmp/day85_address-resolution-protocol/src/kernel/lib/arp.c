#include "arp.h"
#include "rtl8139.h"
#include "tty.h"
#include "utils.h" // 需要用到 memcpy

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
