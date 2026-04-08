#include "io.h"
#include "utils.h"
#include "tty.h"
#include "rtl8139.h"
#include "ethernet.h"
#include "ipv4.h"
#include "arp.h"
#include "tcp.h"

// ==========================================
// 【Day 95 新增】簡易的 TCP 單一連線狀態
// ==========================================
static uint32_t active_seq = 0;
static uint32_t active_ack = 0;

static uint8_t my_ip[4] = {10, 0, 2, 15};

void tcp_send_syn(uint8_t* dest_ip, uint16_t src_port, uint16_t dest_port) {
    uint8_t* target_mac = arp_lookup(dest_ip);
    if (target_mac == 0) {
        kprintf("[TCP] MAC unknown! Initiating ARP request...\n");
        arp_send_request(dest_ip);
        return;
    }

    uint32_t tcp_len = sizeof(tcp_header_t); // SYN 封包通常沒有 Payload 資料
    uint32_t packet_size = sizeof(ethernet_header_t) + sizeof(ipv4_header_t) + tcp_len;

    uint8_t packet[1500];
    memset(packet, 0, packet_size);

    ethernet_header_t* eth = (ethernet_header_t*)packet;
    ipv4_header_t* ip  = (ipv4_header_t*)(packet + sizeof(ethernet_header_t));
    tcp_header_t* tcp  = (tcp_header_t*)(packet + sizeof(ethernet_header_t) + sizeof(ipv4_header_t));

    // 1. Ethernet Header
    memcpy(eth->dest_mac, target_mac, 6);
    memcpy(eth->src_mac, rtl8139_get_mac(), 6);
    eth->ethertype = htons(ETHERTYPE_IPv4);

    // 2. IPv4 Header
    ip->version = 4;
    ip->ihl = 5;
    ip->total_length = htons(sizeof(ipv4_header_t) + tcp_len);
    ip->protocol = IPV4_PROTO_TCP; // 6
    memcpy(ip->src_ip, my_ip, 4);
    memcpy(ip->dest_ip, dest_ip, 4);
    ip->ttl = 64; // 【修復】給封包 64 點的生命值！忘記填的話預設是 0 會被秒殺 XD
    ip->checksum = 0;
    ip->checksum = calculate_checksum(ip, sizeof(ipv4_header_t));

    // 3. TCP Header
    tcp->src_port = htons(src_port);
    tcp->dest_port = htons(dest_port);
    tcp->seq_num = htonl(0x12345678); // 初始序號 (ISN)，真實 OS 會用隨機數，這裡我們寫死方便觀察
    tcp->ack_num = 0; // 第一步還沒收到對方東西，所以是 0
    // Header 長度是 20 bytes，也就是 5 個 32-bit word。所以高 4 bits 填 5。
    tcp->data_offset = (5 << 4);
    tcp->flags = TCP_SYN; // 【關鍵】這是一顆 SYN 封包！
    tcp->window_size = htons(8192); // 告訴對方我們的接收 Buffer 有多大
    tcp->urgent_ptr = 0;
    tcp->checksum = 0; // 計算前先歸零

    // 4. 計算包含偽標頭的 TCP Checksum
    uint8_t pseudo_buf[sizeof(tcp_pseudo_header_t) + sizeof(tcp_header_t)];
    tcp_pseudo_header_t* pseudo = (tcp_pseudo_header_t*)pseudo_buf;

    // 填寫偽標頭
    memcpy(pseudo->src_ip, my_ip, 4);
    memcpy(pseudo->dest_ip, dest_ip, 4);
    pseudo->reserved = 0;
    pseudo->protocol = IPV4_PROTO_TCP;
    pseudo->tcp_length = htons(tcp_len);

    // 把填好的 TCP 標頭複製到偽標頭的後面
    memcpy(pseudo_buf + sizeof(tcp_pseudo_header_t), tcp, tcp_len);

    // 一起算 Checksum，然後填回真正的 TCP 標頭裡！
    tcp->checksum = calculate_checksum(pseudo_buf, sizeof(pseudo_buf));

    // 發射！
    rtl8139_send_packet(packet, packet_size);
    kprintf("[TCP] SYN sent to %d.%d.%d.%d:%d\n", dest_ip[0], dest_ip[1], dest_ip[2], dest_ip[3], dest_port);
}


// ==========================================
// 【Day 94 新增】發送 TCP ACK 封包 (完成三方交握)
// ==========================================
void tcp_send_ack(uint8_t* dest_ip, uint16_t src_port, uint16_t dest_port, uint32_t seq_num, uint32_t ack_num) {
    // 【Day 95 新增】每次送出 ACK 時，把當前的序號狀態存起來，給後面的 PSH 和 FIN 使用！
    active_seq = seq_num;
    active_ack = ack_num;

    uint8_t* target_mac = arp_lookup(dest_ip);
    if (target_mac == 0) return; // 理論上已經交握過，ARP 一定查得到

    uint32_t tcp_len = sizeof(tcp_header_t);
    uint32_t packet_size = sizeof(ethernet_header_t) + sizeof(ipv4_header_t) + tcp_len;

    uint8_t packet[1500];
    memset(packet, 0, packet_size);

    ethernet_header_t* eth = (ethernet_header_t*)packet;
    ipv4_header_t* ip  = (ipv4_header_t*)(packet + sizeof(ethernet_header_t));
    tcp_header_t* tcp  = (tcp_header_t*)(packet + sizeof(ethernet_header_t) + sizeof(ipv4_header_t));

    // 1. Ethernet Header
    memcpy(eth->dest_mac, target_mac, 6);
    memcpy(eth->src_mac, rtl8139_get_mac(), 6);
    eth->ethertype = htons(ETHERTYPE_IPv4);

    // 2. IPv4 Header
    ip->version = 4;
    ip->ihl = 5;
    ip->total_length = htons(sizeof(ipv4_header_t) + tcp_len);
    ip->protocol = IPV4_PROTO_TCP; // 6
    ip->ttl = 64; // 【非常重要】賦予生命值！
    memcpy(ip->src_ip, my_ip, 4);
    memcpy(ip->dest_ip, dest_ip, 4);
    ip->checksum = 0;
    ip->checksum = calculate_checksum(ip, sizeof(ipv4_header_t));

    // 3. TCP Header
    tcp->src_port = htons(src_port);
    tcp->dest_port = htons(dest_port);
    // 【關鍵】將我們算好的 Seq 和 Ack 轉成大端序 (Big-Endian) 塞進去！
    tcp->seq_num = htonl(seq_num);
    tcp->ack_num = htonl(ack_num);

    tcp->data_offset = (5 << 4); // Header 長度 20 bytes
    tcp->flags = TCP_ACK;        // 【關鍵】這是一顆純 ACK 封包！
    tcp->window_size = htons(8192);
    tcp->urgent_ptr = 0;
    tcp->checksum = 0;

    // 4. 計算偽標頭 Checksum
    uint8_t pseudo_buf[sizeof(tcp_pseudo_header_t) + sizeof(tcp_header_t)];
    tcp_pseudo_header_t* pseudo = (tcp_pseudo_header_t*)pseudo_buf;

    memcpy(pseudo->src_ip, my_ip, 4);
    memcpy(pseudo->dest_ip, dest_ip, 4);
    pseudo->reserved = 0;
    pseudo->protocol = IPV4_PROTO_TCP;
    pseudo->tcp_length = htons(tcp_len);

    memcpy(pseudo_buf + sizeof(tcp_pseudo_header_t), tcp, tcp_len);
    tcp->checksum = calculate_checksum(pseudo_buf, sizeof(pseudo_buf));

    // 發射！
    rtl8139_send_packet(packet, packet_size);
    kprintf("[TCP] ACK sent to %d.%d.%d.%d:%d (Seq=%d, Ack=%d)\n",
            dest_ip[0], dest_ip[1], dest_ip[2], dest_ip[3], dest_port, seq_num, ack_num);
}


// 發送帶有真實資料的 TCP 封包 (PSH + ACK)
void tcp_send_data(uint8_t* dest_ip, uint16_t src_port, uint16_t dest_port, uint8_t* data, uint32_t len) {
    uint8_t* target_mac = arp_lookup(dest_ip);
    if (!target_mac) return;

    uint32_t tcp_len = sizeof(tcp_header_t) + len; // 包含 Payload 的長度
    uint32_t packet_size = sizeof(ethernet_header_t) + sizeof(ipv4_header_t) + tcp_len;
    uint8_t packet[1500]; memset(packet, 0, packet_size);

    ethernet_header_t* eth = (ethernet_header_t*)packet;
    ipv4_header_t* ip  = (ipv4_header_t*)(packet + sizeof(ethernet_header_t));
    tcp_header_t* tcp  = (tcp_header_t*)(packet + sizeof(ethernet_header_t) + sizeof(ipv4_header_t));
    uint8_t* payload   = packet + sizeof(ethernet_header_t) + sizeof(ipv4_header_t) + sizeof(tcp_header_t);

    // 1 & 2. 封裝 MAC 與 IP (與之前完全一樣)
    memcpy(eth->dest_mac, target_mac, 6);
    memcpy(eth->src_mac, rtl8139_get_mac(), 6);
    eth->ethertype = htons(ETHERTYPE_IPv4);
    ip->version = 4;
    ip->ihl = 5;
    ip->protocol = IPV4_PROTO_TCP;
    ip->ttl = 64;
    ip->total_length = htons(sizeof(ipv4_header_t) + tcp_len);
    memcpy(ip->src_ip, my_ip, 4);
    memcpy(ip->dest_ip, dest_ip, 4);
    ip->checksum = 0;
    ip->checksum = calculate_checksum(ip, sizeof(ipv4_header_t));

    // 3. TCP Header
    tcp->src_port = htons(src_port); tcp->dest_port = htons(dest_port);
    tcp->seq_num = htonl(active_seq);
    tcp->ack_num = htonl(active_ack);
    tcp->data_offset = (5 << 4);
    tcp->flags = TCP_PSH | TCP_ACK; // 【關鍵】這是一顆資料推送封包！
    tcp->window_size = htons(8192);
    tcp->urgent_ptr = 0;
    tcp->checksum = 0;

    // 4. 拷貝真實資料
    memcpy(payload, data, len);

    // 5. 計算偽標頭 Checksum
    uint8_t pseudo_buf[2000]; // 確保夠大
    tcp_pseudo_header_t* pseudo = (tcp_pseudo_header_t*)pseudo_buf;
    memcpy(pseudo->src_ip, my_ip, 4);
    memcpy(pseudo->dest_ip, dest_ip, 4);
    pseudo->reserved = 0;
    pseudo->protocol = IPV4_PROTO_TCP;
    pseudo->tcp_length = htons(tcp_len);

    // 把 TCP Header 和 Payload 一起算進 Checksum
    memcpy(pseudo_buf + sizeof(tcp_pseudo_header_t), tcp, tcp_len);
    tcp->checksum = calculate_checksum(pseudo_buf, sizeof(tcp_pseudo_header_t) + tcp_len);

    rtl8139_send_packet(packet, packet_size);
    kprintf("[TCP] Data Sent (Len: %d, Seq: %d)\n", len, active_seq);

    // 【重要數學】傳送了 len 個 bytes，我們的序號必須推進 len！
    active_seq += len;
}

// 發送結束連線的 TCP 封包 (FIN + ACK)
void tcp_send_fin(uint8_t* dest_ip, uint16_t src_port, uint16_t dest_port) {
    uint8_t* target_mac = arp_lookup(dest_ip);
    if (!target_mac) return;

    // 【修改 1】FIN 沒有 Payload，所以長度只有 TCP Header 本身
    uint32_t tcp_len = sizeof(tcp_header_t);
    uint32_t packet_size = sizeof(ethernet_header_t) + sizeof(ipv4_header_t) + tcp_len;
    uint8_t packet[1500]; memset(packet, 0, packet_size);

    ethernet_header_t* eth = (ethernet_header_t*)packet;
    ipv4_header_t* ip  = (ipv4_header_t*)(packet + sizeof(ethernet_header_t));
    tcp_header_t* tcp  = (tcp_header_t*)(packet + sizeof(ethernet_header_t) + sizeof(ipv4_header_t));
    // 【修改 2】刪除了 payload 指標，因為根本用不到

    // 1 & 2. 封裝 MAC 與 IP
    memcpy(eth->dest_mac, target_mac, 6);
    memcpy(eth->src_mac, rtl8139_get_mac(), 6);
    eth->ethertype = htons(ETHERTYPE_IPv4);
    ip->version = 4;
    ip->ihl = 5;
    ip->protocol = IPV4_PROTO_TCP;
    ip->ttl = 64;
    ip->total_length = htons(sizeof(ipv4_header_t) + tcp_len);
    memcpy(ip->src_ip, my_ip, 4);
    memcpy(ip->dest_ip, dest_ip, 4);
    ip->checksum = 0;
    ip->checksum = calculate_checksum(ip, sizeof(ipv4_header_t));

    // 3. TCP Header
    tcp->src_port = htons(src_port);
    tcp->dest_port = htons(dest_port);
    tcp->seq_num = htonl(active_seq);
    tcp->ack_num = htonl(active_ack);
    tcp->data_offset = (5 << 4);
    tcp->flags = TCP_FIN | TCP_ACK; // 【關鍵】FIN 旗標
    tcp->window_size = htons(8192);
    tcp->urgent_ptr = 0;
    tcp->checksum = 0;

    // 【修改 3】刪除了 memcpy(payload, data, len);

    // 4. 計算偽標頭 Checksum
    uint8_t pseudo_buf[2000]; // 確保夠大
    tcp_pseudo_header_t* pseudo = (tcp_pseudo_header_t*)pseudo_buf;
    memcpy(pseudo->src_ip, my_ip, 4);
    memcpy(pseudo->dest_ip, dest_ip, 4);
    pseudo->reserved = 0;
    pseudo->protocol = IPV4_PROTO_TCP;
    pseudo->tcp_length = htons(tcp_len);

    // 把 TCP Header 算進 Checksum (因為沒有 Payload，所以只算 Header)
    memcpy(pseudo_buf + sizeof(tcp_pseudo_header_t), tcp, tcp_len);
    tcp->checksum = calculate_checksum(pseudo_buf, sizeof(tcp_pseudo_header_t) + tcp_len);

    rtl8139_send_packet(packet, packet_size);
    // 【修改 4】Log 拿掉 len，印出是 FIN 封包
    kprintf("[TCP] FIN Sent (Seq: %d)\n", active_seq);

    // 【重要數學】FIN 封包雖然沒有資料，但在 TCP 狀態機裡它會「消耗 1 個序號」！
    active_seq += 1;
}
