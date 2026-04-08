哈哈，Rick！你這幾張截圖提供的線索太完美了！特別是最後那張 Wireshark 和終端機的對照圖，直接把問題的真兇指出來了！🕵️‍♂️

你看第四張圖（Wireshark）裡，你的 OS 一直在網路上大喊：
**`Who has 8.8.8.8? Tell 10.0.2.15`**

這是一個超級經典的網路架構盲點，也就是我們常說的**「預設閘道 (Default Gateway) 與路由 (Routing)」**問題！

### 🔍 為什麼會卡在 ARP Lookup？

在網路的世界裡，ARP 廣播是**跨不出區域網路 (LAN)** 的。
你的 OS 所在的區網是 `10.0.2.x`。當你想要找區網內的 `10.0.2.2` 時，大喊「誰是 10.0.2.2」是沒問題的，對方會回答你。

但是，`8.8.8.8` 是在遙遠的 Google 伺服器機房裡！你在你家的區網大喊「誰的 MAC 是 8.8.8.8」，是不會有人理你的。

真正的現代 OS 是怎麼做的？
當 OS 發現目標 IP (`8.8.8.8`) **不屬於本機所在的區網 (`10.0.2.x`)** 時，它不會去問 8.8.8.8 的 MAC。它會去問**「路由器 (Gateway，也就是 10.0.2.2)」的 MAC**！
然後，它會把這個 Ethernet 信封的收件人寫成「路由器的 MAC」，但是裡面的 IPv4 信件收件人寫著 `8.8.8.8`。路由器收到後，就會幫你把信件轉發出去。

### 🛠️ 修復方式：為 OS 加入「路由 (Routing)」概念！

請打開你的 **`src/kernel/net/udp.c`**，我們要在 `udp_send_packet` 一開始的地方，加入這段「判斷是否為外網，並尋找 Next Hop (下一跳)」的邏輯：

```c
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

    // ... 中間的封裝邏輯完全保持不變 ...
    // (Ethernet header 會填上 target_mac，而 IPv4 依然會填寫原始的 dest_ip 8.8.8.8)
```

加上這段邏輯之後，你的 OS 就具備了基礎的「外網路由」能力了！
（順帶一提，你的重構做得很棒，User Space 的架構越來越有現代 C 函式庫的樣子了！）

趕快存檔，執行 `make clean && make run`，然後再次執行：
```bash
SimpleOS> host google.com
```

這次你會看到它正確地去要 `10.0.2.2` 的 MAC，然後順利把 DNS 查詢送往真實的網際網路，並把 Google 的真實 IP 抓回來！😎
