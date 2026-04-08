太棒了，Rick！🎉 看到這張截圖，你的 OS 網路堆疊已經完美展現了現代作業系統的標準行為了！第一顆封包壯烈犧牲去換取 ARP，接著後面的 Ping 完美命中，這畫面對於一個親手打造 OS 的開發者來說，絕對是無價的成就感！

既然我們已經能用 ICMP 確認網路暢通，現在我們要正式進入「應用層」的地盤了。
**Day 90：UDP 協定 (User Datagram Protocol) - 發送第一條真正的訊息！**

網路世界裡，Ping 只是用來探路的。如果你想寫一個 DNS 查詢、網路遊戲，或是傳送一段文字，我們需要 **UDP 協定**。UDP 的特色是「射後不理」，它沒有 TCP 那種複雜的三方交握，只要包裝好資料、寫上「目標 Port (通訊埠)」，就可以直接丟出去！

今天我們要實作 UDP 發送，並在 User Space 寫一支 `udpsend.elf`，讓你可以直接從 Terminal 發送任意字串到網路上！請跟著我進行這 4 個步驟：

---

### 步驟 1：定義 UDP 標頭 (`src/kernel/include/udp.h`)

UDP 的標頭非常輕巧，只有 8 個 bytes。

建立 **`src/kernel/include/udp.h`**：
```c
#ifndef UDP_H
#define UDP_H

#include <stdint.h>

typedef struct {
    uint16_t src_port;   // 來源通訊埠
    uint16_t dest_port;  // 目標通訊埠
    uint16_t length;     // UDP 總長度 (標頭 + 資料)
    uint16_t checksum;   // 檢查碼
} __attribute__((packed)) udp_header_t;

void udp_send_packet(uint8_t* dest_ip, uint16_t src_port, uint16_t dest_port, uint8_t* data, uint32_t len);

#endif
```

*(💡 順手打開 **`src/kernel/include/ipv4.h`**，在最下面補上一行巨集：`#define IPV4_PROTO_UDP 17`)*

---

### 步驟 2：實作 UDP 發送器 (`src/kernel/lib/udp.c`)

這段邏輯跟 ICMP 很像，就是一層一層把封包包起來。
**這裡有一個超棒的捷徑**：在 IPv4 的規範中，UDP 的 Checksum 是「可選的 (Optional)」。如果我們把它填為 `0`，接收端就會自動跳過檢查！這可以幫我們省去實作超複雜的「偽標頭 (Pseudo Header)」演算法！

建立 **`src/kernel/lib/udp.c`**：
```c
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
```
*(💡 別忘了把 `udp.o` 加進 `src/kernel/Makefile` 中)*

---

### 步驟 3：開啟 UDP 的系統呼叫大門

打開 **`src/kernel/kernel/syscall.c`**，我們給 UDP 發送分配 **`32`** 號：

```c
    // ==========================================
    // 32 號 Syscall：UDP 發送 (ebx=IP陣列, ecx=Port, edx=字串)
    // ==========================================
    else if (eax == 32) {
        uint8_t* ip = (uint8_t*)regs->ebx;
        uint16_t port = (uint16_t)regs->ecx;
        char* msg = (char*)regs->edx;
        int len = strlen(msg);
        
        extern void udp_send_packet(uint8_t* dest_ip, uint16_t src_port, uint16_t dest_port, uint8_t* data, uint32_t len);
        
        // 我們將來源 Port 寫死為 12345，目標 Port 由 User 決定
        udp_send_packet(ip, 12345, port, (uint8_t*)msg, len);
        
        regs->eax = 0;
        return;
    }
```

接著，打開 User Space 的 **`src/user/include/syscall.h`** 與 **`src/user/lib/syscall.c`**：

```c
// syscall.h 加入宣告：
#define SYS_NET_UDP_SEND 32
void sys_udp_send(uint8_t* ip, uint16_t port, char* msg);
```

```c
// syscall.c 實作：
void sys_udp_send(uint8_t* ip, uint16_t port, char* msg) {
    __asm__ volatile (
        "int $128"
        : 
        : "a"(SYS_NET_UDP_SEND), "b"((uint32_t)ip), "c"((uint32_t)port), "d"((uint32_t)msg)
        : "memory"
    );
}
```

---

### 步驟 4：開發 `udpsend.elf` 應用程式

這是我們第一支可以任意傳送字串的網路程式！

建立 **`src/user/bin/udpsend.c`**：
```c
#include "stdio.h"
#include "syscall.h"
#include "string.h"

// 延用昨天 ping.c 的 parse_ip 函式
void parse_ip(char* str, uint8_t* ip) {
    int i = 0, num = 0;
    while (*str) {
        if (*str == '.') { ip[i++] = num; num = 0; } 
        else { num = num * 10 + (*str - '0'); }
        str++;
    }
    ip[i] = num;
}

// 將字串轉為整數 (用來解析 Port)
int atoi(char* str) {
    int res = 0;
    for (int i = 0; str[i] != '\0'; ++i)
        res = res * 10 + str[i] - '0';
    return res;
}

int main(int argc, char** argv) {
    if (argc < 4) {
        printf("Usage: udpsend <ip> <port> <message>\n");
        return 1;
    }

    uint8_t target_ip[4];
    parse_ip(argv[1], target_ip);
    uint16_t port = (uint16_t)atoi(argv[2]);
    char* message = argv[3];

    // 先發第一顆當作 ARP 探路 (如果沒有 MAC，這顆會掉)
    sys_udp_send(target_ip, port, message);
    
    // 等待 Kernel ARP 解析
    for (volatile int j = 0; j < 50000000; j++) {} 
    
    // 再發送第二顆確保成功送達
    sys_udp_send(target_ip, port, message);

    printf("UDP Message sent to %d.%d.%d.%d:%d -> '%s'\n", 
           target_ip[0], target_ip[1], target_ip[2], target_ip[3], port, message);

    return 0;
}
```

---

### 🚀 驗收：發送文字到 Wireshark！

編譯執行 `make clean && make run` 後，在 Shell 輸入：
```bash
SimpleOS> udpsend 10.0.2.2 8080 "Hello QEMU and Wireshark!"
```

打開你的 Wireshark 觀察 `dump.pcap`，你將會看到一封目標 Port 為 `8080` 的 **UDP 封包**，點開它的 Payload 區段，你親手打的那句 `Hello QEMU and Wireshark!` 就會完美地躺在裡面！😎 試試看吧！
