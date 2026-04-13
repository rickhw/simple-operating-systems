#include "utils.h"
#include "tty.h"
#include "ethernet.h"
#include "rtl8139.h"
#include "ipv4.h"
#include "arp.h"
#include "udp.h"

// 【Day 92 新增】簡易 UDP Socket 信箱
// ==========================================
static uint16_t bound_port = 0;       // 目前被 User App 監聽的 Port
static char rx_msg_buffer[256];       // 存放接收到的文字
static int rx_msg_ready = 0;          // 標記是否有新訊息 (1: 有, 0: 無)
// 修改狀態變數
static int rx_msg_len = 0; // 改為記錄長度 (0 代表沒訊息)

static uint8_t my_ip[4] = {10, 0, 2, 15};

// 讓 User App 綁定 Port
void udp_bind_port(uint16_t port) {
    bound_port = port;
    // rx_msg_ready = 0;
    kprintf("[Kernel][UDP] Port [%d] bound by User Space.\n", port);
}

// 讓 User App 讀取訊息 (非阻塞)
int udp_recv_data(char* buffer) {
    if (rx_msg_len > 0) {
        memcpy(buffer, rx_msg_buffer, rx_msg_len); // 用 memcpy 才不會被 0x00 截斷！
        int len = rx_msg_len;
        rx_msg_len = 0; // 讀取完畢，清空狀態
        return len;
    }
    return 0;             // 回傳 0 代表目前沒訊息
}

// 讓 rtl8139 驅動把封包交給這裡
// 修改網卡對接處
void udp_handle_receive(uint16_t dest_port, uint8_t* payload, uint16_t payload_len) {
    if (bound_port != 0 && dest_port == bound_port) {
        int copy_len = (payload_len < 255) ? payload_len : 255;
        memcpy(rx_msg_buffer, payload, copy_len);
        rx_msg_len = copy_len; // 記錄真正的長度！
    }
}

void udp_send_packet(uint8_t* dest_ip, uint16_t src_port, uint16_t dest_port, uint8_t* data, uint32_t len) {
    // ==========================================
    // 【Day 96 新增】簡易路由表邏輯 (Routing)
    // ==========================================
    uint8_t gateway_ip[4] = {10, 0, 2, 2}; // 我們的 QEMU 虛擬路由器
    uint8_t* next_hop_ip = dest_ip;        // 預設下一跳就是目標本人

    // 檢查目標 IP 是否在同一個子網路 (假設我們是 10.x.x.x)
    // 如果不是，下一跳 (Next Hop) 就必須改成 Gateway！
    if (dest_ip[0] != 10) {
        next_hop_ip = gateway_ip;
    }

    // 注意：我們是查 next_hop_ip 的 MAC，而不是 dest_ip 的 MAC！
    uint8_t* target_mac = arp_lookup(next_hop_ip);

    if (target_mac == 0) {
        kprintf("[UDP] MAC unknown for next hop! Initiating ARP request...\n");
        // 如果連路由器的 MAC 都不知道，就去問路由器
        arp_send_request(next_hop_ip);
        return;
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
