#include "icmp.h"
#include "rtl8139.h"
#include "utils.h"
#include "tty.h"
#include "arp.h" // 【新增】引入 ARP 查詢

static uint8_t my_ip[4] = {10, 0, 2, 15};
static uint16_t ping_seq = 1;

void ping_send_request(uint8_t* target_ip) {
    // 【修復】真實 OS 邏輯：先查 ARP 電話簿！
    uint8_t* target_mac = arp_lookup(target_ip);
    if (target_mac == 0) {
        kprintf("[ICMP][Ping] Target MAC unknown! Cannot send ping to %d.%d.%d.%d\n",
                target_ip[0], target_ip[1], target_ip[2], target_ip[3]);

        // ==========================================
        // 【Day 89 升級】Kernel 自動發送 ARP 請求！
        // 並且直接 return，故意「丟棄」這第一顆 Ping 封包！
        // ==========================================
        arp_send_request(target_ip);

        return; // 不知道 MAC，直接拒絕發送！
    }

    // 總大小 = Eth(14) + IP(20) + ICMP(8) + Payload(32) = 74 bytes
    uint32_t packet_size = sizeof(ethernet_header_t) + sizeof(ipv4_header_t) + sizeof(icmp_header_t) + 32;
    uint8_t packet[74];
    memset(packet, 0, packet_size);

    ethernet_header_t* eth = (ethernet_header_t*)packet;
    ipv4_header_t* ip  = (ipv4_header_t*)(packet + sizeof(ethernet_header_t));
    icmp_header_t* icmp= (icmp_header_t*)(packet + sizeof(ethernet_header_t) + sizeof(ipv4_header_t));
    uint8_t* payload = packet + packet_size - 32;

    uint8_t* my_mac = rtl8139_get_mac();

    // 1. Ethernet Header
    memcpy(eth->dest_mac, target_mac, 6);
    memcpy(eth->src_mac, my_mac, 6);
    eth->ethertype = htons(ETHERTYPE_IPv4);

    // 2. IPv4 Header
    ip->version = 4;
    ip->ihl = 5; // 5 * 4 = 20 bytes
    ip->tos = 0;
    ip->total_length = htons(sizeof(ipv4_header_t) + sizeof(icmp_header_t) + 32);
    ip->ident = htons(0x1234);
    ip->flags_frag = 0;
    ip->ttl = 64;
    ip->protocol = IPV4_PROTO_ICMP;
    memcpy(ip->src_ip, my_ip, 4);
    memcpy(ip->dest_ip, target_ip, 4);
    ip->checksum = 0; // 計算前必須為 0
    ip->checksum = calculate_checksum(ip, sizeof(ipv4_header_t));

    // 3. ICMP Header
    icmp->type = ICMP_ECHO_REQUEST;
    icmp->code = 0;
    icmp->identifier = htons(0x1337);
    icmp->sequence = htons(ping_seq++);

    // 塞點垃圾資料當作 Payload (32 bytes)
    for(int i=0; i<32; i++) payload[i] = 'A' + (i % 26);

    // 計算 ICMP Checksum (涵蓋 ICMP 標頭與 Payload)
    icmp->checksum = 0;
    icmp->checksum = calculate_checksum(icmp, sizeof(icmp_header_t) + 32);

    // 4. 發射！
    rtl8139_send_packet(packet, packet_size);
    kprintf("[ICMP][Ping] Sending Echo Request to [%d.%d.%d.%d]...\n",
            target_ip[0], target_ip[1], target_ip[2], target_ip[3]);
}
