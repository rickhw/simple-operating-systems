哈哈，Rick！準備好進入全新的 Phase 10 了嗎？🌍

在前面的日子裡，你的 Simple OS 已經可以用 IP 位址（如 `10.0.2.2` 或 `8.8.8.8`）與世界溝通了。但是，人類的大腦並不擅長記憶數字，我們喜歡記的是 `google.com` 或是 `github.com`。

**Day 96：網路世界的翻譯蒟蒻 — DNS 解析 (Domain Name System)**

今天我們要來開發一個 DNS Client 應用程式：`host.elf`。
它的任務是透過 UDP 協定，向 Google 的公開 DNS 伺服器 (`8.8.8.8`) 的 **Port 53** 發出查詢請求，然後將伺服器回傳的真實 IP 位址抓出來！



⚠️ **【重大警報：修復 Kernel 的 UDP 接收器】**
在 Day 92 我們實作 `udp_recv_data` 時，因為只是為了接收文字聊天，我們用了 `strcpy(buffer, rx_msg_buffer);`。但是 **DNS 封包是二進位資料 (Binary Data)**，裡面充滿了 `0x00` (Null Byte)！如果用 `strcpy`，讀到一半就會被截斷！

我們必須先花 1 分鐘把這個小地雷拆掉，然後再來寫 DNS！請跟著我進行這 4 個步驟：

---

### 步驟 0：將 UDP 升級為二進位接收 (`src/kernel/lib/udp.c` & `syscall`)

打開你的 **`src/kernel/lib/udp.c`**，我們把 `rx_msg_ready` 改成記錄「接收到的長度」，並換成 `memcpy`：

```c
// 修改狀態變數
static int rx_msg_len = 0; // 改為記錄長度 (0 代表沒訊息)

// 修改接收函式：回傳實際拷貝的長度
int udp_recv_data(uint8_t* buffer) {
    if (rx_msg_len > 0) {
        memcpy(buffer, rx_msg_buffer, rx_msg_len); // 用 memcpy 才不會被 0x00 截斷！
        int len = rx_msg_len;
        rx_msg_len = 0; // 讀取完畢，清空狀態
        return len;     // 回傳收到的位元組數量
    }
    return 0; 
}

// 修改網卡對接處
void udp_handle_receive(uint16_t dest_port, uint8_t* payload, uint16_t payload_len) {
    if (bound_port != 0 && dest_port == bound_port) {
        int copy_len = (payload_len < 255) ? payload_len : 255;
        memcpy(rx_msg_buffer, payload, copy_len);
        rx_msg_len = copy_len; // 記錄真正的長度！
    }
}
```

*(💡 別忘了在 User Space 的 `syscall.h` 和 `syscall.c` 裡，把 `sys_udp_recv` 的回傳值當作長度來使用！)*

---

### 步驟 1：定義 DNS 標頭 (`src/user/include/dns.h`)

DNS 的查詢和回應共用同一套標頭格式，非常工整，總共 12 bytes。
我們在 User Space 建立一個標頭檔：

建立 **`src/user/include/dns.h`**：
```c
#ifndef DNS_H
#define DNS_H

#include <stdint.h>

typedef struct {
    uint16_t transaction_id; // 交易 ID (我們自己亂數定一個，例如 0x1234)
    uint16_t flags;          // 旗標 (查詢是 0x0100)
    uint16_t questions;      // 幾個問題？ (通常是 1)
    uint16_t answer_rrs;     // 幾個回答？ (發問時是 0)
    uint16_t authority_rrs;  // 授權記錄數量
    uint16_t additional_rrs; // 額外記錄數量
} __attribute__((packed)) dns_header_t;

// User Space 用的簡易 Endianness 轉換 (從 ethernet.h 抄過來的)
static inline uint16_t htons(uint16_t hostshort) {
    return ((hostshort & 0xFF) << 8) | ((hostshort & 0xFF00) >> 8);
}
static inline uint16_t ntohs(uint16_t netshort) { return htons(netshort); }

#endif
```

---

### 步驟 2：實作網域編碼邏輯

DNS 協定有一個很特別的規定：它不用 `.` 來分隔網址，而是用「**長度前綴**」。
例如 `www.google.com` 必須被轉換成二進位的：
`[3]www[6]google[3]com[0]` (最後的 0 代表結束)。

這段轉換邏輯我們直接寫在等一下要建立的 App 裡面。

---

### 步驟 3：撰寫 DNS 解析工具 (`src/user/bin/host.c`)

這支程式會組裝剛才的 DNS Header、轉換網址格式，然後呼叫 UDP Syscall 發送到 `8.8.8.8`！

建立 **`src/user/bin/host.c`**：
```c
#include "stdio.h"
#include "syscall.h"
#include "string.h"
#include "dns.h"

// 網址轉換函數: "google.com" -> "\x06google\x03com\x00"
void format_dns_name(char* dns_name, char* host_name) {
    int lock = 0;
    for (int i = 0; i <= strlen(host_name); i++) {
        if (host_name[i] == '.' || host_name[i] == '\0') {
            *dns_name++ = i - lock;
            for (; lock < i; lock++) {
                *dns_name++ = host_name[lock];
            }
            lock++; // 跳過點
        }
    }
    *dns_name++ = '\0'; // 結尾
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: host <domain_name>\n");
        return 1;
    }

    char* domain = argv[1];
    printf("Resolving %s via 8.8.8.8...\n", domain);

    // 1. 準備 DNS 查詢封包
    uint8_t packet[256];
    memset(packet, 0, 256);
    
    dns_header_t* dns = (dns_header_t*)packet;
    dns->transaction_id = htons(0x1337); 
    dns->flags = htons(0x0100); // Standard Query
    dns->questions = htons(1);
    
    uint8_t* qname = packet + sizeof(dns_header_t);
    format_dns_name((char*)qname, domain);
    
    // QNAME 後面接著 QTYPE(A record = 1) 和 QCLASS(IN = 1)
    uint16_t* qinfo = (uint16_t*)(qname + strlen((char*)qname) + 1);
    qinfo[0] = htons(1); // Type A (IPv4)
    qinfo[1] = htons(1); // Class IN
    
    int query_len = sizeof(dns_header_t) + strlen((char*)qname) + 1 + 4;

    // 2. 綁定接收 Port 並發射 UDP 封包
    uint8_t dns_server[4] = {8, 8, 8, 8}; // Google Public DNS
    sys_udp_bind(43210); // 我們綁定本機 43210 等待回應
    
    // 先發第一顆換 ARP，等一下再發第二顆真實查詢
    sys_udp_send(dns_server, 53, (char*)packet); // Port 53 是 DNS 預設 Port
    for (volatile int j = 0; j < 50000000; j++) {}
    sys_udp_send(dns_server, 53, (char*)packet);

    // 3. 等待回應
    uint8_t rx_buf[256];
    int rx_len = 0;
    while (1) {
        rx_len = sys_udp_recv((char*)rx_buf);
        if (rx_len > 0) break;
        for (volatile int i = 0; i < 1000000; i++) {} // Delay
    }

    // 4. 極度暴力的 DNS 解析器 (尋找最後 4 個 bytes)
    // 真正的 OS 會一步一步解析 Answer Section，
    // 但因為我們知道 Type A 的 IP 一定在封包的最後 4 個 bytes，直接偷吃步！
    if (rx_len > 4) {
        uint8_t* ip = rx_buf + rx_len - 4;
        printf(">> %s has address %d.%d.%d.%d\n", domain, ip[0], ip[1], ip[2], ip[3]);
    } else {
        printf("Failed to resolve.\n");
    }

    return 0;
}
```
*(💡 記得在 `User Makefile` 裡把 `host.elf` 加進去編譯！)*

---

### 🚀 驗收：敲開真實世界的大門！

重新執行 `make clean && make run`！

在 Simple OS Shell 裡輸入：
```bash
SimpleOS> host google.com
```

如果你的 UDP 二進位修復成功，而且 QEMU 的虛擬網路能連上外網，你將會看到系統停頓一下（等待 8.8.8.8 的回應），然後印出：
`>> google.com has address 142.250.xxx.xxx`

接著你可以試試看 `host github.com` 或是 `host tw.yahoo.com`！
當這四個 IP 數字出現在你的畫面上時，這代表你的作業系統已經真正在網際網路上自由航行了！這就是網路開發最浪漫的瞬間！😎 趕快去試試看！
