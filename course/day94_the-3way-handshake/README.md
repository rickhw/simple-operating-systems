哈哈，Rick！準備好迎接歷史性的一刻了嗎？🔥

看著上一回合 Wireshark 裡面那些焦急等待的 `[SYN, ACK]` 封包，以及無奈的 `[TCP Retransmission]`，我們怎麼忍心讓 macOS 的作業系統底層這樣苦苦守候呢？



**Day 94：接起電話！完成 TCP 三方交握 (The 3-Way Handshake)**

要讓連線正式進入 `ESTABLISHED` (已建立) 狀態，我們必須讓 Simple OS 學會「聆聽」對方傳來的 `[SYN, ACK]`，並且精準地回傳一顆 `[ACK]` 封包。
這裡最核心的魔法在於 **序號 (Sequence Number)** 與 **確認號 (Acknowledge Number)** 的數學遊戲：
* 對方傳來的序號是 `Seq = X`，我們回傳的確認號必須是 `Ack = X + 1`（代表我們收到了，請給我下一個 byte）。
* 對方傳來的確認號是 `Ack = Y`，這代表他期待我們下一個送出的序號，所以我們回傳的序號必須是 `Seq = Y`。

請跟著我進行這 3 個步驟，把這個大魔王徹底斬除！

---

### 步驟 1：實作 TCP ACK 發送器 (`src/kernel/lib/tcp.c`)

打開你的 **`src/kernel/lib/tcp.c`**，在原本的 `tcp_send_syn` 下方，加入這個專門用來回覆 `ACK` 的函式：

```c
// ==========================================
// 【Day 94 新增】發送 TCP ACK 封包 (完成三方交握)
// ==========================================
void tcp_send_ack(uint8_t* dest_ip, uint16_t src_port, uint16_t dest_port, uint32_t seq_num, uint32_t ack_num) {
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
```

順手打開 **`src/kernel/include/tcp.h`**，在最下面補上它的宣告：
```c
void tcp_send_ack(uint8_t* dest_ip, uint16_t src_port, uint16_t dest_port, uint32_t seq_num, uint32_t ack_num);
```

---

### 步驟 2：在網路接收器中攔截 TCP 封包 (`src/kernel/lib/rtl8139.c`)

當我們收到 `[SYN, ACK]` 時，我們要立刻解析它，並回傳剛才寫好的 `ACK`！

打開你的 **`src/kernel/lib/rtl8139.c`**，在 `IPv4` 的處理區塊裡面，加入針對 `TCP (Protocol 6)` 的處理邏輯：

```c
                // ... 前面有 ICMP (1) 和 UDP (17) 的處理 ...
                
                // ==========================================
                // 【Day 94 新增】攔截並處理 TCP 封包！
                // ==========================================
                else if (ip->protocol == 6) { // 6 代表 TCP
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
                }
```
*(💡 記得在檔案最上方確認有 `#include "tcp.h"` 以取得 `TCP_SYN` 等巨集)*

---

### 🚀 步驟 3：終極驗收！消滅 Retransmission！

這將是 Simple OS 網路堆疊中最純粹的暴力美學！

1. 確保你的 macOS 終端機依然開著 `nc -l 8080` 等待連線。
2. 執行 `make clean && make run` 啟動 OS。
3. 在 Simple OS Shell 敲下：
   ```bash
   SimpleOS> tcptest 10.0.2.2 8080
   ```

**轉頭死盯著你的 Wireshark！**

這次，劇本將會完美無瑕：
1. `[SYN]` 飛出！
2. `[SYN, ACK]` 飛回！
3. 你的 OS 瞬間攔截，印出 `[SYN, ACK] received! Completing handshake...`。
4. 你的 OS 精準計算出 `Seq` 與 `Ack`，反手打出一顆黑色的 **`[ACK]`** 封包！

你會發現，**那些討厭的紅黑色 `[TCP Retransmission]` 再也不會出現了！** 因為 macOS 收到了你的確認，它滿意地將這條連線標記為 `ESTABLISHED`！

恭喜你，Rick！你親手完成了現代網際網路最偉大、最基礎的「三方交握」機制！趕快去見證這個奇蹟吧！😎
