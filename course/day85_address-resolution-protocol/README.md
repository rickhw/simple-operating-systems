哈哈，Rick！這張 Wireshark 截圖看起來實在太紓壓了！🎉

看到 Source 變成漂亮的 `aa:aa:aa:aa:aa:aa`，且封包內容不再是全零，這代表我們已經完美征服了 PCI Bus Mastering (DMA) 的機制！硬體的隔閡已經徹底打通，接下來就是純粹的軟體協定大戰了。

**Day 85：主動出擊！(ARP 協定解析與發送)**

在網路世界裡，所有的實體傳輸都只認得 MAC 位址，不認得 IP 位址。
當我們的 OS 想要傳送資料給 QEMU 的虛擬路由器（預設 IP 是 `10.0.2.2`）時，我們必須先在網路上大喊：「**請問誰的 IP 是 10.0.2.2？請把你的 MAC 位址告訴我！**」這就是 **ARP (Address Resolution Protocol)** 的用途。

今天我們要親自封裝這個「大喊」的 ARP 請求封包！請跟著我進行這 4 個步驟：

---

### 步驟 1：公開網卡的 MAC 與發送 API (`rtl8139.h` & `rtl8139.c`)

為了讓其他網路協定能使用網卡，我們需要把 `mac_addr` 導出，並把發送函式宣告在標頭檔。

**1. 打開 `src/kernel/include/rtl8139.h`，加入這兩個宣告：**
```c
// [Day 85 新增]
void rtl8139_send_packet(void* data, uint32_t len);
uint8_t* rtl8139_get_mac(void);
```

**2. 打開 `src/kernel/lib/rtl8139.c`，在檔案最下方加入：**
```c
// [Day 85 新增] 讓其他模組取得本機的 MAC 位址
uint8_t* rtl8139_get_mac(void) {
    return mac_addr;
}
```

---

### 步驟 2：定義 ARP 標頭檔 (`src/kernel/include/arp.h`)

ARP 封包有嚴格的欄位順序，我們一樣要用 `__attribute__((packed))` 來定義它。

建立 **`src/kernel/include/arp.h`**：

```c
#ifndef ARP_H
#define ARP_H

#include <stdint.h>
#include "ethernet.h"

// ARP 封包結構 (IPv4 over Ethernet)
typedef struct {
    uint16_t hardware_type; // 0x0001 (Ethernet)
    uint16_t protocol_type; // 0x0800 (IPv4)
    uint8_t  hw_addr_len;   // 6 (MAC 位址長度)
    uint8_t  proto_addr_len;// 4 (IPv4 位址長度)
    uint16_t opcode;        // 1 代表 Request, 2 代表 Reply
    uint8_t  src_mac[6];    // 來源 MAC
    uint8_t  src_ip[4];     // 來源 IP
    uint8_t  dest_mac[6];   // 目標 MAC (Request 時填 0)
    uint8_t  dest_ip[4];    // 目標 IP
} __attribute__((packed)) arp_packet_t;

#define ARP_REQUEST 1
#define ARP_REPLY   2

void arp_send_request(uint8_t* target_ip);

#endif
```

---

### 步驟 3：實作 ARP 請求發送器 (`src/kernel/lib/arp.c`)

我們要將乙太網路標頭 (Ethernet Header) 和 ARP 標頭組合在一起，並丟給網卡發送。在 SLIRP 模式下，QEMU 通常會把我們的 OS 預設為 `10.0.2.15`。

建立 **`src/kernel/lib/arp.c`**：

```c
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
    
    kprintf("[ARP] Broadcast Request sent: Who has %d.%d.%d.%d?\n", 
            target_ip[0], target_ip[1], target_ip[2], target_ip[3]);
}
```
*(💡 記得將 `arp.o` 加進 `src/kernel/Makefile` 中)*

---

### 步驟 4：發出真實的連線請求 (`src/kernel/kernel.c`)

我們把昨天測試用的 `test_net_packet` 刪掉，改成呼叫正式的 ARP 請求，對著 QEMU 的虛擬路由器 (`10.0.2.2`) 發出尋人啟事！

打開 **`kernel.c`**：

```c
#include "arp.h" // 【新增】引入 ARP

// ... 在 kernel_main 中，init_pci() 之後 ...

    init_pci();
    
    // 【Day 85 新增】主動尋找路由器的 MAC 位址！
    uint8_t router_ip[4] = {10, 0, 2, 2};
    arp_send_request(router_ip);

// ... 後面的 init_gfx 等程式碼保持不變 ...
```

---

完成這 4 步後，編譯並執行 `make clean && make run`！

這一次，你發出去的是一顆完全符合真實世界網路規範的合法 ARP Request！
因為封包完全合法，QEMU 的 SLIRP 路由器在收到這顆封包後，**有非常高的機率會立刻回傳一顆 `ARP Reply` 給你！** 觀察你的 Terminal，看看有沒有跳出 `[Net] Packet RX!`，以及用 Wireshark 打開 `dump.pcap` 看看這場完美的對話吧！
