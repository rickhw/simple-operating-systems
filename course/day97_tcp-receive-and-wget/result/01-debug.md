出現 segmentation fault, dump.pacp 沒有東西，看起來還沒開始就壞掉了 XD

---
src/kernel/drivers/net/rtl8139.c
```c
#include "io.h"
#include "utils.h"
#include "pci.h"
#include "tty.h"
#include "rtl8139.h"
#include "ethernet.h"
#include "arp.h"
#include "ipv4.h"
#include "icmp.h"
#include "udp.h"
#include "tcp.h"

// 儲存網卡的 I/O 基準位址與 MAC 位址
static uint32_t rtl_iobase = 0;
static uint8_t mac_addr[6];

// ==========================================
// 接收環狀緩衝區 (Rx Ring Buffer)
// 網卡要求最少 8K (8192) bytes，外加 16 bytes 的檔頭與 1500 bytes 容錯
// __attribute__((aligned(4096))) 確保它在實體記憶體上是 4K 對齊的
// ==========================================
static uint8_t rx_buffer[8192 + 16 + 1500] __attribute__((aligned(4096)));

//  用來追蹤目前讀取到緩衝區的哪個位置
static uint16_t rx_offset = 0;

// ==========================================
// 網卡發送函式 (支援 4 槽輪詢)
// ==========================================
static uint8_t tx_buffer[1536] __attribute__((aligned(4096)));
static uint8_t tx_cur = 0; // 記錄現在輪到哪一個發送槽 (0~3)


void init_rtl8139(uint8_t bus, uint8_t slot) {
    kprintf("[RTL8139] Initializing driver...\n");

    // 1. 取得 I/O Base Address
    // 網卡的 Base Address Register 0 (BAR0) 存在 PCI 設定空間的 Offset 0x10
    // 因為 BAR0 是 32-bit，我們用兩次 16-bit 讀取來組合它
    uint32_t bar0_low = pci_read_config_word(bus, slot, 0, 0x10);
    uint32_t bar0_high = pci_read_config_word(bus, slot, 0, 0x12);

    // 組合出 32-bit 的位址，並清除最低 2 個 bit (PCI 規範，用來標示這是一塊 I/O 空間)
    rtl_iobase = (bar0_low | (bar0_high << 16)) & ~3;
    kprintf("[RTL8139] I/O Base Address found at: [%x]\n", rtl_iobase);

    // ==========================================
    // 啟用 PCI Bus Mastering！
    // 允許網卡使用 DMA 跨過 CPU 直接讀取主記憶體
    // ==========================================
    uint16_t pci_cmd = pci_read_config_word(bus, slot, 0, 0x04);
    // 0x0004 就是 Bit 2 (Bus Master Enable)
    pci_write_config_word(bus, slot, 0, 0x04, pci_cmd | 0x0004);

    // 2. 開機 (Power On)
    // RTL8139 的 CONFIG1 暫存器 (Offset 0x52)，寫入 0x00 即可喚醒
    outb(rtl_iobase + 0x52, 0x00);

    // 3. 軟體重置 (Software Reset)
    // 寫入 0x10 到 Command 暫存器 (Offset 0x37) 觸發重置
    outb(rtl_iobase + 0x37, 0x10);

    // 不斷讀取，直到 0x10 這個 bit 被硬體清空，代表重置完成
    while ((inb(rtl_iobase + 0x37) & 0x10) != 0) {
        // 等待硬體重置...
    }
    kprintf("[RTL8139] Device reset successful.\n");

    // 4. 讀取 MAC 位址
    // RTL8139 的 MAC 位址直接存在 I/O Base 的最前面 6 個 Byte (Offset 0x00 ~ 0x05)
    for (int i = 0; i < 6; i++) {
        mac_addr[i] = inb(rtl_iobase + i);
    }

    kprintf("[RTL8139] MAC Address: [%x:%x:%x:%x:%x:%x]\n",
            mac_addr[0], mac_addr[1], mac_addr[2],
            mac_addr[3], mac_addr[4], mac_addr[5]);


    // ==========================================
    // 網卡接收引擎與中斷設定
    // ==========================================

    // 5. 將我們準備好的 Rx Buffer 實體位址告訴網卡 (寫入 RBSTART 暫存器 0x30)
    // 註：這裡假設你的 Kernel 是 Identity Mapped (虛擬位址 == 實體位址)
    outl(rtl_iobase + 0x30, (uint32_t)rx_buffer);

    // 6. 設定中斷遮罩 (IMR - Interrupt Mask Register, 0x3C)
    // 0x0005 代表我們只想監聽 "Rx OK (接收成功)" 和 "Tx OK (發送成功)" 的中斷
    outw(rtl_iobase + 0x3C, 0x0005);

    // 7. 設定接收配置暫存器 (RCR - Receive Configuration Register, 0x44)
    // (AB|AM|APM|AAP) = 0xF 代表允許接收：廣播、多播、MAC符合的封包
    // WRAP (bit 7) = 1 代表當 Buffer 滿了時，網卡會自動從頭覆寫 (環狀機制)
    outl(rtl_iobase + 0x44, 0xf | (1 << 7));

    // 8. 正式啟動接收 (Rx) 與發送 (Tx) 引擎！
    // Command Register (0x37), RE(bit 3)=1, TE(bit 2)=1 -> 組合起來就是 0x0C
    outb(rtl_iobase + 0x37, 0x0C);

    // 9. 從 PCI Config Space 讀出這張網卡被分配到的 IRQ 號碼
    uint32_t irq_info = pci_read_config_word(bus, slot, 0, 0x3C);
    uint8_t irq_line = irq_info & 0xFF; // 取出最低 8 bits

    kprintf("[RTL8139] Hardware initialized. Assigned IRQ: [%d]\n", irq_line);
}


// ==========================================
// RTL8139 中斷處理常式
// ==========================================
void rtl8139_handler(void) {
    // 1. 讀取中斷狀態暫存器 (ISR, Offset 0x3E)
    uint16_t status = inw(rtl_iobase + 0x3E);

    if (!status) return;

    // 2. 寫回 ISR 來清除中斷旗標 (寫入 1 代表清除)
    outw(rtl_iobase + 0x3E, status);

    // 3. 檢查是否為 "Receive OK" (Bit 0)
    if (status & 0x01) {

        // 不斷讀取，直到硬體告訴我們 Buffer 空了 (CR 暫存器的 Bit 0 為 1 代表空)
        while ((inb(rtl_iobase + 0x37) & 0x01) == 0) {

            // RTL8139 封包的開頭會有 4 個 bytes 的硬體檔頭：
            // [ 16-bit Rx Status ] [ 16-bit Packet Length ]
            uint16_t* rx_header = (uint16_t*)(rx_buffer + rx_offset);
            uint16_t rx_status = rx_header[0];
            uint16_t rx_length = rx_header[1];

            // ==========================================
            // 防禦性編程：避免無限迴圈！
            // 如果長度為 0，或者 Rx Status 的 Bit 0 (ROK) 不為 1，
            // 代表這是一筆尚未完成 DMA 或無效的髒資料，立刻中斷讀取！
            // ==========================================
            if (rx_length == 0 || (rx_status & 0x01) == 0) {
                break;
            }

            // 實際的網路封包位址 (跳過那 4 bytes 硬體檔頭)
            uint8_t* packet_data = rx_buffer + rx_offset + 4;

            // 將封包轉型為乙太網路標頭！
            ethernet_header_t* eth = (ethernet_header_t*)packet_data;

            kprintf("[Net] Packet RX! Len: [%d] bytes\n", rx_length);
            kprintf("  -> Dest MAC: [%x:%x:%x:%x:%x:%x]\n",
                    eth->dest_mac[0], eth->dest_mac[1], eth->dest_mac[2],
                    eth->dest_mac[3], eth->dest_mac[4], eth->dest_mac[5]);

            uint16_t type = ntohs(eth->ethertype);
            if (type == ETHERTYPE_ARP) {
                // 轉型為 ARP 封包
                arp_packet_t* arp = (arp_packet_t*)(packet_data + sizeof(ethernet_header_t));
                uint16_t arp_op = ntohs(arp->opcode);

                // kprintf("  -> Protocol: ARP\n");
                // kprintf("  -> ARP OPCode: [%d]\n", arp_op);

                // 如果這是一封 ARP Reply (Opcode == 2)
                if (arp_op == ARP_REPLY) {
                    // 呼叫我們剛剛寫的 API，把對方的 IP 和 MAC 記下來！
                    extern void arp_update_table(uint8_t* ip, uint8_t* mac);
                    arp_update_table(arp->src_ip, arp->src_mac);
                }
                // ==========================================
                // 如果別人在找我們，立刻回覆！
                // ==========================================
                else if (arp_op == ARP_REQUEST) {
                    // 檢查他找的 IP 是不是我們？(10.0.2.15)
                    uint8_t my_ip[4] = {10, 0, 2, 15};
                    if (memcmp(arp->dest_ip, my_ip, 4) == 0) {
                        kprintf("[ARP] Someone is asking for our MAC! Replying...\n");
                        extern void arp_send_reply(uint8_t* target_ip, uint8_t* target_mac);
                        // 把發問者的 IP 和 MAC 傳給回覆函式
                        arp_send_reply(arp->src_ip, arp->src_mac);
                    }
                }
            }
            else if (type == ETHERTYPE_IPv4) {
                // 跳過 Ethernet Header
                ipv4_header_t* ip = (ipv4_header_t*)(packet_data + sizeof(ethernet_header_t));

                if (ip->protocol == IPV4_PROTO_ICMP) { // 1 代表 ICMP
                    icmp_header_t* icmp = (icmp_header_t*)(packet_data + sizeof(ethernet_header_t) + (ip->ihl * 4));
                    if (icmp->type == 0) { // 0 代表 Echo Reply
                        kprintf("[Ping] Reply received from [%d.%d.%d.%d]!\n\n",
                                ip->src_ip[0], ip->src_ip[1], ip->src_ip[2], ip->src_ip[3]);
                    }
                }

                // ==========================================
                // 【Day 94 新增】攔截並處理 TCP 封包！
                // ==========================================
                else if (ip->protocol == IPV4_PROTO_TCP) { // 6 代表 TCP
                    tcp_header_t* tcp = (tcp_header_t*)(packet_data + sizeof(ethernet_header_t) + (ip->ihl * 4));

                    uint16_t src_port = ntohs(tcp->src_port);
                    uint16_t dest_port = ntohs(tcp->dest_port);
                    uint32_t seq = ntohl(tcp->seq_num);
                    uint32_t ack = ntohl(tcp->ack_num);

                    kprintf("[TCP Rx] %d.%d.%d.%d:%d -> Port %d (Flags: %x)\n",
                            ip->src_ip[0], ip->src_ip[1], ip->src_ip[2], ip->src_ip[3],
                            src_port, dest_port, tcp->flags);

                    // 檢查 Flags 是否同時包含 SYN (0x02) 和 ACK (0x10)
                    if ((tcp->flags & TCP_SYN) && (tcp->flags & TCP_ACK)) {
                        kprintf("  -> [SYN, ACK] received! Completing handshake...\n");

                        // 【關鍵數學】
                        // 我們的下一個 Seq，就是對方期望我們送的 Ack 號碼
                        // 我們的下一個 Ack，必須是對方的 Seq + 1 (代表我們收到了他的 SYN)
                        extern void tcp_send_ack(uint8_t* dest_ip, uint16_t src_port, uint16_t dest_port, uint32_t seq_num, uint32_t ack_num);

                        // 注意參數順序：來源變目標，目標變來源
                        tcp_send_ack(ip->src_ip, dest_port, src_port, ack, seq + 1);
                    }

                    // ==========================================
                    // 【Day 97 新增】攔截 TCP Payload (PSH 旗標)
                    // ==========================================
                    if (tcp->flags & TCP_PSH) { // 0x08 就是 TCP_PSH
                        // 計算 TCP Header 長度
                        uint32_t header_len = (tcp->data_offset >> 4) * 4;
                        uint8_t* payload = (uint8_t*)tcp + header_len;
                        // 計算真實資料長度 = IP 總長 - IP 標頭 - TCP 標頭
                        int payload_len = ntohs(ip->total_length) - (ip->ihl * 4) - header_len;

                        if (payload_len > 0) {
                            extern char tcp_rx_buffer[];
                            extern int tcp_rx_len;
                            int copy_len = (payload_len < 4095) ? payload_len : 4095;
                            memcpy(tcp_rx_buffer, payload, copy_len);
                            tcp_rx_len = copy_len;
                            tcp_rx_buffer[copy_len] = '\0'; // 字串結尾

                            kprintf("[TCP Rx] Received %d bytes of Web Data!\n", payload_len);

                            // 【關鍵】收到資料後，必須回傳 ACK 告訴伺服器「我收到了」
                            // 我們的 ACK 號碼要是對方的 Seq + 剛收到的資料長度
                            extern void tcp_send_ack(uint8_t*, uint16_t, uint16_t, uint32_t, uint32_t);
                            tcp_send_ack(ip->src_ip, dest_port, src_port, ack, seq + payload_len);
                        }
                    }
                }

                // 【Day 91 新增】17 代表 UDP！
                else if (ip->protocol == IPV4_PROTO_UDP) {
                    // 跳過 Ethernet Header 和 IPv4 Header
                    udp_header_t* udp = (udp_header_t*)(packet_data + sizeof(ethernet_header_t) + (ip->ihl * 4));

                    // Payload 的位置在 UDP Header 之後
                    uint8_t* payload = (uint8_t*)udp + sizeof(udp_header_t);

                    // 計算真實資料的長度 (UDP 總長度 - UDP 標頭長度)
                    uint16_t payload_len = ntohs(udp->length) - sizeof(udp_header_t);

                    // 【Day 92 修改】不要直接 kprintf，交給 UDP 模組處理！
                    extern void udp_handle_receive(uint16_t dest_port, uint8_t* payload, uint16_t payload_len);
                    udp_handle_receive(ntohs(udp->dest_port), payload, payload_len);
                }
            }
            else {
                // kprintf("  -> Protocol: Unknown (%x)\n", type);
            }

            // 4. 更新 Rx_offset
            // 長度包含 4 bytes CRC，且必須對齊 4 bytes 邊界
            rx_offset = (rx_offset + rx_length + 4 + 3) & ~3;

            // 若超過 8K (8192) 邊界，會繞回開頭 (Wrap around)
            if (rx_offset >= 8192) {
                rx_offset -= 8192;
            }

            // 5. 告訴網卡我們讀到哪裡了 (寫入 CAPR 暫存器 0x38)
            // (註：硬體設計怪癖，必須減掉 16)
            outw(rtl_iobase + 0x38, rx_offset - 16);
        }
    }

    // 發送 EOI 給中斷控制器 (PIC)
    // 因為 IRQ 11 接在 Slave PIC 上，所以要發給 Master & Slave
    outb(0xA0, 0x20);
    outb(0x20, 0x20);
}

void rtl8139_send_packet(void* data, uint32_t len) {
    uint32_t send_len = len;
    // 乙太網路最小限制 60 Bytes，不足要補 0 (Padding)
    if (send_len < 60) send_len = 60;

    // 計算當前要用的暫存器位址 (每個槽相差 4 bytes)
    // TSD: 0x10, 0x14, 0x18, 0x1C
    // TSAD: 0x20, 0x24, 0x28, 0x2C
    uint32_t tsd_port = rtl_iobase + 0x10 + (tx_cur * 4);
    uint32_t tsad_port = rtl_iobase + 0x20 + (tx_cur * 4);

    // 等待當前的發送槽有空 (Bit 13 為 1 代表 Owned by Host)
    while ((inl(tsd_port) & (1 << 13)) == 0) {
        // Busy wait (等待上一次這個槽的資料送完)
    }

    // 將資料拷貝到實體緩衝區 (先清零以防夾帶垃圾資料)
    memset(tx_buffer, 0, send_len);
    memcpy(tx_buffer, data, len);

    // 寫入實體位址到 TSAD
    outl(tsad_port, (uint32_t)tx_buffer);
    // 寫入長度並觸發發送 TSD
    outl(tsd_port, send_len & 0x1FFF);

    // 輪到下一個發送槽 (0->1->2->3->0)
    tx_cur = (tx_cur + 1) % 4;
}

//  讓其他模組取得本機的 MAC 位址
uint8_t* rtl8139_get_mac(void) {
    return mac_addr;
}
```


---
src/kernel/net/tcp.c
```c
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

// 【Day 97 新增】TCP 接收信箱
// ==========================================
char tcp_rx_buffer[4096]; // 準備一個夠大的 Buffer 放 HTML
int tcp_rx_len = 0;

static uint8_t my_ip[4] = {10, 0, 2, 15};   // @TODO hardcode

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


int tcp_recv_data(char* buffer) {
    if (tcp_rx_len > 0) {
        memcpy(buffer, tcp_rx_buffer, tcp_rx_len);
        int len = tcp_rx_len;
        tcp_rx_len = 0;
        return len;
    }
    return 0;
}
```
