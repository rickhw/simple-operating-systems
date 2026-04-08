哈哈，Rick！我們終於要迎來 TCP 協定的最高潮了！🔥

在昨天的 Day 94，我們成功接通了電話，進入了 `ESTABLISHED` (已建立) 狀態。但打電話不講話是很尷尬的，而且講完電話不說再見直接掛斷（拔網路線）也是不禮貌的。

**Day 95：TCP 資料傳輸與優雅中斷 (TCP Data Transfer & Connection Teardown)**

今天，我們要讓 Simple OS 具備兩大能力：
1. **講話 (`TCP_PSH`)**：Push 旗標，代表這顆封包裡面帶有真實的 Payload 資料，請接收端立刻把資料推給應用程式（如 macOS 的 `nc`）。
2. **說再見 (`TCP_FIN`)**：Finish 旗標，代表我的話說完了，發起「四方揮手 (4-Way Handshake)」，優雅地釋放這條連線。

這裡的關鍵字是**「狀態 (State)」**。我們要讓 Kernel 記住目前通訊的**序號 (Seq)** 與**確認號 (Ack)**，這樣我們傳資料時才不會亂掉。

請跟著我進行這 4 個步驟，完成網路篇最壯觀的資料傳輸！

---

### 步驟 1：在 TCP 模組加入狀態記憶與發送函式 (`src/kernel/lib/tcp.c`)

我們要稍微修改昨天的 `tcp.c`，加上兩個全域變數來記住 `Seq` 和 `Ack`，並且實作傳送資料與 FIN 的函式。

打開 **`src/kernel/lib/tcp.c`**：

**1. 在檔案最上方加入狀態變數：**
```c
// ==========================================
// 【Day 95 新增】簡易的 TCP 單一連線狀態
// ==========================================
static uint32_t active_seq = 0;
static uint32_t active_ack = 0;
```

**2. 修改昨天的 `tcp_send_ack`，讓它順便記住狀態：**
```c
void tcp_send_ack(uint8_t* dest_ip, uint16_t src_port, uint16_t dest_port, uint32_t seq_num, uint32_t ack_num) {
    // 【Day 95 新增】每次送出 ACK 時，把當前的序號狀態存起來，給後面的 PSH 和 FIN 使用！
    active_seq = seq_num;
    active_ack = ack_num;
    
    // ... 下面原本的封裝與發射程式碼完全保持不變 ...
}
```

**3. 在檔案最下方，加入 `tcp_send_data` 與 `tcp_send_fin`：**
```c
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
    memcpy(eth->dest_mac, target_mac, 6); memcpy(eth->src_mac, rtl8139_get_mac(), 6);
    eth->ethertype = htons(ETHERTYPE_IPv4);
    ip->version = 4; ip->ihl = 5; ip->protocol = IPV4_PROTO_TCP; ip->ttl = 64;
    ip->total_length = htons(sizeof(ipv4_header_t) + tcp_len);
    memcpy(ip->src_ip, my_ip, 4); memcpy(ip->dest_ip, dest_ip, 4);
    ip->checksum = 0; ip->checksum = calculate_checksum(ip, sizeof(ipv4_header_t));

    // 3. TCP Header
    tcp->src_port = htons(src_port); tcp->dest_port = htons(dest_port);
    // 【關鍵】使用剛才存下來的狀態！
    tcp->seq_num = htonl(active_seq); 
    tcp->ack_num = htonl(active_ack); 
    tcp->data_offset = (5 << 4); 
    tcp->flags = TCP_PSH | TCP_ACK; // 【關鍵】這是一顆資料推送封包！
    tcp->window_size = htons(8192); tcp->urgent_ptr = 0; tcp->checksum = 0;

    // 4. 拷貝真實資料
    memcpy(payload, data, len);

    // 5. 計算偽標頭 Checksum
    uint8_t pseudo_buf[2000]; // 確保夠大
    tcp_pseudo_header_t* pseudo = (tcp_pseudo_header_t*)pseudo_buf;
    memcpy(pseudo->src_ip, my_ip, 4); memcpy(pseudo->dest_ip, dest_ip, 4);
    pseudo->reserved = 0; pseudo->protocol = IPV4_PROTO_TCP; pseudo->tcp_length = htons(tcp_len);
    
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
    // 這裡為了精簡版面，邏輯與 tcp_send_data 幾乎一樣，
    // 唯一的差別是沒有 Payload，且 Flags 設為：
    // tcp->flags = TCP_FIN | TCP_ACK;
    // 然後序號推進 1 (FIN 佔用一個序號)：
    // active_seq += 1;
    
    // (Rick，你可以複製 tcp_send_data 的上半段來改寫，把 payload 拿掉，改掉 Flags 即可！)
}
```

---

### 步驟 2：開啟 Syscall 大門 (`syscall.c`, `syscall.h`)

我們要開通 **36 號 (TCP Send)** 和 **37 號 (TCP Close)** 系統呼叫。

**1. Kernel 端 (`src/kernel/kernel/syscall.c`)：**
```c
    else if (eax == 36) { // SYS_NET_TCP_SEND
        extern void tcp_send_data(uint8_t*, uint16_t, uint16_t, uint8_t*, uint32_t);
        char* msg = (char*)regs->edx;
        tcp_send_data((uint8_t*)regs->ebx, 54321, (uint16_t)regs->ecx, (uint8_t*)msg, strlen(msg));
        regs->eax = 0; return;
    }
    else if (eax == 37) { // SYS_NET_TCP_CLOSE
        extern void tcp_send_fin(uint8_t*, uint16_t, uint16_t);
        tcp_send_fin((uint8_t*)regs->ebx, 54321, (uint16_t)regs->ecx);
        regs->eax = 0; return;
    }
```

**2. User 端 (`src/user/include/syscall.h` & `src/user/lib/syscall.c`)：**
```c
// syscall.h
#define SYS_NET_TCP_SEND 36
#define SYS_NET_TCP_CLOSE 37
void sys_tcp_send(uint8_t* ip, uint16_t port, char* msg);
void sys_tcp_close(uint8_t* ip, uint16_t port);

// syscall.c
void sys_tcp_send(uint8_t* ip, uint16_t port, char* msg) {
    __asm__ volatile ("int $128" : : "a"(SYS_NET_TCP_SEND), "b"((uint32_t)ip), "c"((uint32_t)port), "d"((uint32_t)msg) : "memory");
}
void sys_tcp_close(uint8_t* ip, uint16_t port) {
    __asm__ volatile ("int $128" : : "a"(SYS_NET_TCP_CLOSE), "b"((uint32_t)ip), "c"((uint32_t)port) : "memory");
}
```

---

### 步驟 3：升級測試程式 (`src/user/bin/tcptest.c`)

現在，我們的 `tcptest` 不只要握手，還要講話，最後還要掛電話！

```c
#include "stdio.h"
#include "syscall.h"

extern void parse_ip(char* str, uint8_t* ip);
extern int atoi(char* str);

int main(int argc, char** argv) {
    if (argc < 3) return 1;

    uint8_t target_ip[4];
    parse_ip(argv[1], target_ip);
    uint16_t port = (uint16_t)atoi(argv[2]);

    printf("1. Initiating TCP 3-Way Handshake...\n");
    sys_tcp_connect(target_ip, port); // 觸發 ARP 
    for (volatile int j = 0; j < 50000000; j++) {} 
    sys_tcp_connect(target_ip, port); // 真正的 SYN
    
    // 等待對方回傳 SYN, ACK，Kernel 會在背景自動回傳 ACK
    for (volatile int j = 0; j < 50000000; j++) {} 

    printf("2. Sending Data (PSH)...\n");
    char* message = "Hello macOS, this is Simple OS over TCP!\n";
    sys_tcp_send(target_ip, port, message);

    // 等待對方接收
    for (volatile int j = 0; j < 50000000; j++) {} 

    printf("3. Closing Connection (FIN)...\n");
    sys_tcp_close(target_ip, port);

    printf("TCP Sequence Complete.\n");
    return 0;
}
```

---

### 🚀 驗收：見證 TCP 的暴力美學！

1. 在你的 **macOS 終端機** 開啟等待：
   ```bash
   nc -l 8080
   ```
2. 在 Simple OS 裡執行：
   ```bash
   SimpleOS> tcptest 10.0.2.2 8080
   ```

**這絕對是你目前為止看過最爽的畫面：**
1. 你的 macOS 終端機會突然印出：`Hello macOS, this is Simple OS over TCP!` (你成功把資料送進另一個作業系統了！)
2. 接著，macOS 的 `nc` 指令會**自動結束跳回提示字元**！(因為它收到了你優雅的 `FIN` 斷線通知！)
3. 打開 Wireshark，你會看到連續的：
   * `[SYN]` -> `[SYN, ACK]` -> `[ACK]` (三方交握)
   * `[PSH, ACK]` (資料傳送)
   * `[FIN, ACK]` (四方揮手)

這個完整的封包序列，就是全宇宙幾十億台電腦每天在網路上溝通的唯一標準語言。完成這一步，你的 Simple OS 就真正畢業，成為一個擁有現代網路靈魂的作業系統了！趕快去試試看吧！😎
