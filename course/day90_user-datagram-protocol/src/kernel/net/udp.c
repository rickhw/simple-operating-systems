#include "udp.h"
#include "ipv4.h"
#include "ethernet.h"
#include "arp.h"
#include "rtl8139.h"
#include "utils.h"
#include "tty.h"

static uint8_t my_ip[4] = {10, 0, 2, 15};

void udp_send_packet(uint8_t* dest_ip, uint16_t src_port, uint16_t dest_port, uint8_t* data, uint32_t len) {
    uint8_t* target_mac = arp_lookup(dest_ip);

    if (target_mac == 0) {
        kprintf("[UDP] MAC unknown! Initiating ARP request...\n");
        arp_send_request(dest_ip);
        return; // 丟棄這顆 UDP，等 ARP 解析完 User 再重試
    }

    // 總大小 = Eth(14) + IP(20) + UDP(8) + Payload
    uint32_t packet_size = sizeof(ethernet_header_t) + sizeof(ipv4_header_t) + sizeof(udp_header_t) + len;

    // 宣告一個夠大的 Buffer
    uint8_t packet[1500];
    memset(packet, 0, packet_size);

    ethernet_header_t* eth = (ethernet_header_t*)packet;
    ipv4_header_t* ip  = (ipv4_header_t*)(packet + sizeof(ethernet_header_t));
    udp_header_t* udp  = (udp_header_t*)(packet + sizeof(ethernet_header_t) + sizeof(ipv4_header_t));
    uint8_t* payload   = packet + sizeof(ethernet_header_t) + sizeof(ipv4_header_t) + sizeof(udp_header_t);

    uint8_t* my_mac = rtl8139_get_mac();

    // 1. Ethernet Header
    memcpy(eth->dest_mac, target_mac, 6);
    memcpy(eth->src_mac, my_mac, 6);
    eth->ethertype = htons(ETHERTYPE_IPv4);

    // 2. IPv4 Header
    ip->version = 4;
    ip->ihl = 5;
    ip->tos = 0;
    ip->total_length = htons(sizeof(ipv4_header_t) + sizeof(udp_header_t) + len);
    ip->ident = htons(0x5678);
    ip->flags_frag = 0;
    ip->ttl = 64;
    ip->protocol = IPV4_PROTO_UDP; // 17 代表 UDP
    memcpy(ip->src_ip, my_ip, 4);
    memcpy(ip->dest_ip, dest_ip, 4);
    ip->checksum = 0;
    ip->checksum = calculate_checksum(ip, sizeof(ipv4_header_t));

    // 3. UDP Header
    udp->src_port = htons(src_port);
    udp->dest_port = htons(dest_port);
    udp->length = htons(sizeof(udp_header_t) + len);
    udp->checksum = 0; // IPv4 允許 UDP Checksum 為 0 (不檢查)

    // 4. Payload (塞入真正的資料)
    memcpy(payload, data, len);

    // 發射！
    rtl8139_send_packet(packet, packet_size);
}
