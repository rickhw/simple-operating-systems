哈哈，Rick！恭喜你，那個 `undefined reference` 解除後，你的系統終於完美連上線了！🎉

你現在觀察到的一個現象超級有趣：**為什麼 `[Net] Packet RX!` 會出現在 `[ARP] Broadcast Request sent` 之前呢？**

這絕對不是你眼睛花去，而是你無意間見證了作業系統中**「中斷優先權與超高速硬體」**的完美展現，這是一個經典的 OS 時間差 (Race Condition) 奇觀：
1. 你的程式碼執行 `rtl8139_send_packet`，把封包送進網卡的 I/O 暫存器。
2. 虛擬網卡 (QEMU SLIRP) **瞬間**就把這顆 ARP 請求送給了虛擬路由器 (`10.0.2.2`)。
3. 虛擬路由器發現你在找它，**立刻在 1 微秒內**回傳了一顆 ARP Reply 給你。
4. 網卡收到 Reply，立刻對 CPU 發出 **IRQ 11** 中斷。
5. 這時候，你的 CPU 甚至還來不及執行 `arp_send_request` 裡面的下一行 `kprintf`，就被強制切換到 `rtl8139_handler` 去處理接收到的封包，印出了 `[Net] Packet RX!`。
6. 中斷處理完畢，CPU 回到原本的執行緒，才把 `[ARP] Broadcast Request sent` 這句話印出來。

這證明了你的中斷機制 (IDT/PIC) 運作得極度完美，完全達到了「即時 (Real-time)」的標準！😎

不過我注意到截圖裡沒有印出 `[ARP] Table Updated`，請記得檢查一下你的 `rtl8139.c` 的 `rtl8139_handler` 中，有沒有確實把更新 ARP Table 的那段程式碼加進去喔！（就是判斷 `if (ntohs(arp->opcode) == ARP_REPLY)` 然後呼叫 `arp_update_table` 的那段）。

---

只要 ARP 電話簿準備好了，我們就可以直接進入重頭戲！
**Day 87-88：IPv4 協定與 ICMP (Ping)**

今天，我們要親手刻出網路世界最經典的指令：`ping`！
我們需要包裝 IPv4 標頭，以及 ICMP (網際網路控制訊息協定) 標頭，並計算它們的 Checksum。

請跟著我進行這 4 個步驟：

### 步驟 1：定義 IPv4 與 ICMP 標頭 (`ipv4.h`, `icmp.h`)

建立 **`src/kernel/include/ipv4.h`**：
```c
#ifndef IPV4_H
#define IPV4_H

#include <stdint.h>
#include "ethernet.h"

typedef struct {
    uint8_t  ihl : 4;       // 標頭長度 (Internet Header Length)
    uint8_t  version : 4;   // 版本 (IPv4 = 4)
    uint8_t  tos;           // 服務類型 (Type of Service)
    uint16_t total_length;  // 總長度 (Header + Data)
    uint16_t ident;         // 識別碼
    uint16_t flags_frag;    // 標誌與片段偏移
    uint8_t  ttl;           // 存活時間 (Time to Live)
    uint8_t  protocol;      // 協定 (ICMP = 1, TCP = 6, UDP = 17)
    uint16_t checksum;      // 標頭檢查碼
    uint8_t  src_ip[4];     // 來源 IP
    uint8_t  dest_ip[4];    // 目標 IP
} __attribute__((packed)) ipv4_header_t;

#define IPV4_PROTO_ICMP 1

#endif
```

建立 **`src/kernel/include/icmp.h`**：
```c
#ifndef ICMP_H
#define ICMP_H

#include <stdint.h>
#include "ipv4.h"

typedef struct {
    uint8_t  type;          // 類型 (Echo Request = 8, Echo Reply = 0)
    uint8_t  code;          // 代碼 (通常為 0)
    uint16_t checksum;      // 檢查碼
    uint16_t identifier;    // 識別碼 (用來對應 Request/Reply)
    uint16_t sequence;      // 序號
} __attribute__((packed)) icmp_header_t;

#define ICMP_ECHO_REPLY   0
#define ICMP_ECHO_REQUEST 8

void ping_send_request(uint8_t* target_ip);

#endif
```

---

### 步驟 2：網路檢查碼 (Checksum) 演算法 (`utils.c`)

IP 和 ICMP 都要求我們對封包計算 Checksum（將資料以 16-bit 為單位相加，溢位的部分加回最低位，最後取反碼）。這是網路世界確保資料沒壞掉的鐵則。

打開 **`src/kernel/lib/utils.c`**，加入這個標準函式，並在 `utils.h` 宣告它：
```c
// 計算網路 Checksum
uint16_t calculate_checksum(void* data, int count) {
    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)data;

    while (count > 1) {
        sum += *ptr++;
        count -= 2;
    }

    if (count > 0) {
        sum += *(uint8_t*)ptr;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return ~sum;
}
```

---

### 步驟 3：實作 Ping 發送器 (`icmp.c`)

建立 **`src/kernel/lib/icmp.c`**，我們將把 Ethernet、IPv4、ICMP 像俄羅斯娃娃一樣包裝起來！
*(💡 為了測試方便，我們直接把 SLIRP 路由器的 MAC 寫死，這樣就不必等 ARP Table 解析了！)*

```c
#include "icmp.h"
#include "rtl8139.h"
#include "utils.h"
#include "tty.h"

static uint8_t my_ip[4] = {10, 0, 2, 15};
// QEMU SLIRP 路由器的固定 MAC 位址
static uint8_t router_mac[6] = {0x52, 0x55, 0x0a, 0x00, 0x02, 0x02};

static uint16_t ping_seq = 1;

void ping_send_request(uint8_t* target_ip) {
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
    memcpy(eth->dest_mac, router_mac, 6);
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
    kprintf("[Ping] Sending Echo Request to %d.%d.%d.%d...\n", 
            target_ip[0], target_ip[1], target_ip[2], target_ip[3]);
}
```
*(💡 記得將 `icmp.o` 加進 `Makefile` 中)*

---

### 步驟 4：發射飛彈！(`kernel.c`)

打開 **`kernel.c`**，把剛剛的 `arp_send_request` 換成 `ping_send_request`：

```c
#include "icmp.h" // 引入 ICMP

// ...
    init_pci();
    
    // 對虛擬路由器發出 Ping 請求！
    uint8_t router_ip[4] = {10, 0, 2, 2};
    ping_send_request(router_ip);
```

---

完成後編譯執行！你可以同時看 Terminal 以及你的 Wireshark `dump.pcap`。
這一次，你將會在 Wireshark 裡面看到一顆漂亮的 **ICMP Echo Request**，而且虛擬路由器極有可能會秒回你一顆 **ICMP Echo Reply** (在你的 OS 裡會印出收到 IPv4 封包)！

這就是大名鼎鼎的 `ping` 的底層運作原理，親手把網路協定一層一層疊起來的感覺是不是超有成就感？😎
