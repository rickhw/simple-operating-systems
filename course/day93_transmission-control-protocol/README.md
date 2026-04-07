哈哈，Rick！我們終於來到網路世界的最終大魔王了：**TCP (Transmission Control Protocol)**！🎉

你前面實作的 UDP 就像是「寄明信片」，只要寫上地址丟進郵筒，對方有沒有收到你根本不管。但 TCP 是一通「打電話」的過程，它是有狀態的 (Stateful)，保證資料絕對不會掉、不會亂序。



要建立一通 TCP 電話，必須經過著名的**「三方交握 (3-Way Handshake)」**：
1. **你：** 喂？聽得到嗎？ (**SYN** 封包)
2. **對方：** 聽得到！那你聽得到我嗎？ (**SYN-ACK** 封包)
3. **你：** 聽得到！我們開始講話吧！ (**ACK** 封包)

**Day 93：叩關大魔王！發送你的第一顆 TCP SYN 封包**

今天我們的目標很明確：實作 TCP 的標頭，並朝你的 macOS 發出第一步的 **SYN 請求**！一旦發出，我們就可以在 Wireshark 裡面看到 macOS 秒回你一顆 `SYN-ACK`！

請跟著我進行這 3 個步驟：

---

### 步驟 1：定義 TCP 標頭與「偽標頭」(`src/kernel/include/tcp.h`)



TCP 標頭比 UDP 複雜很多，包含了序號 (Sequence Number) 和確認號 (Acknowledge Number)。
**⚠️ 這裡有一個天大的陷阱：** 昨天在寫 UDP 的時候，我們偷懶把 Checksum 設為 `0` 讓對方略過檢查。但在 TCP 的國際標準裡，**Checksum 是強制的，不填的話 macOS 會直接把你的封包當垃圾丟掉！**
而且 TCP 的 Checksum 計算很特別，它必須把 IPv4 的來源/目標 IP 抽出來，組合成一個**「偽標頭 (Pseudo Header)」**跟 TCP 標頭一起計算。

建立 **`src/kernel/include/tcp.h`**：

```c
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

#endif
```
*(💡 順手去 `ipv4.h` 補上一行 `#define IPV4_PROTO_TCP 6`)*

---

### 步驟 2：實作 TCP SYN 發送器 (`src/kernel/lib/tcp.c`)

這段程式碼的重頭戲，就是先填寫封包，再用 `pseudo_buf` 把 IP 和 TCP 拼起來計算 Checksum！

建立 **`src/kernel/lib/tcp.c`**：

```c
#include "tcp.h"
#include "ipv4.h"
#include "ethernet.h"
#include "arp.h"
#include "rtl8139.h"
#include "utils.h"
#include "tty.h"

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

    // 4. 【魔王關卡】計算包含偽標頭的 TCP Checksum
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
```
*(💡 別忘了把 `tcp.o` 加進 `src/kernel/Makefile` 中)*

---

### 步驟 3：撰寫 User Space 測試程式 (`tcptest.c`)

為了測試這顆 SYN 封包，我們快速開一個 35 號 Syscall 給 `tcptest.elf` 用。

**1. Kernel 端 (`syscall.c`)：**
```c
    // 加入 SYS_NET_TCP_CONNECT
    else if (eax == 35) { 
        extern void tcp_send_syn(uint8_t* dest_ip, uint16_t src_port, uint16_t dest_port);
        // 使用固定的本機 Port 54321 去撞目標的 Port
        tcp_send_syn((uint8_t*)regs->ebx, 54321, (uint16_t)regs->ecx);
        regs->eax = 0;
        return;
    }
```

**2. User 端 (`syscall.h` & `syscall.c`)：**
```c
// syscall.h 宣告
#define SYS_NET_TCP_CONNECT 35
void sys_tcp_connect(uint8_t* ip, uint16_t port);

// syscall.c 實作
void sys_tcp_connect(uint8_t* ip, uint16_t port) {
    __asm__ volatile ("int $128" : : "a"(SYS_NET_TCP_CONNECT), "b"((uint32_t)ip), "c"((uint32_t)port) : "memory");
}
```

**3. User App (`src/user/bin/tcptest.c`)：**
```c
#include "stdio.h"
#include "syscall.h"

extern void parse_ip(char* str, uint8_t* ip);
extern int atoi(char* str);

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: tcptest <ip> <port>\n");
        return 1;
    }

    uint8_t target_ip[4];
    parse_ip(argv[1], target_ip);
    uint16_t port = (uint16_t)atoi(argv[2]);

    printf("Initiating TCP 3-Way Handshake with %d.%d.%d.%d:%d...\n", 
           target_ip[0], target_ip[1], target_ip[2], target_ip[3], port);

    // 第一發：換 ARP
    sys_tcp_connect(target_ip, port);
    for (volatile int j = 0; j < 50000000; j++) {} 
    
    // 第二發：真實的 SYN
    sys_tcp_connect(target_ip, port);

    return 0;
}
```

---

### 🚀 驗收：見證 macOS 的熱情回覆！

在我們執行之前，請先打開你的 **macOS 終端機**，隨便開一個 TCP Server 等著被我們撞：
```bash
nc -l 8080
```

接著回到 QEMU 執行 `make clean && make run`，然後在你的 Simple OS 輸入：
```bash
SimpleOS> tcptest 10.0.2.2 8080
```

如果你的 Checksum 算對了、封裝順序對了，請立刻打開你的 Wireshark！
你將會看到一條你發出的 `[SYN]` 封包，而且它的正下方，絕對會緊跟著一條從 macOS 透過 QEMU 路由器回傳給你的 **`[SYN, ACK]`** 封包！

去見證這歷史性的交握瞬間吧！😎
