哈哈，Rick！這張 Wireshark 截圖實在太有價值了！我們抓到了一個超經典的低級錯誤，而且兇手就是我上一回合少寫了一行 code！😂

你看 Wireshark 裡面的第四個封包，QEMU 路由器回覆了 `Time-to-live exceeded` (存活時間超過)。
仔細看你第二張圖的左下角，ICMP 錯誤訊息裡面會附上我們原本送出去的那顆出包的 IPv4 Header，裡面有一行致命的關鍵字：
**`Time to Live: 0`**

這就是真相！在 IPv4 協定中，**TTL (Time To Live)** 是一個防止封包在網路上無限迷航的防呆機制。封包每經過一台路由器，TTL 就會減 1；如果減到 0，路由器就會把封包無情地丟進垃圾桶，並回傳這顆 ICMP 錯誤給你。

我們在 `tcp_send_syn` 一開始用了 `memset` 把整個封包清零，但是在填寫 IPv4 Header 的時候，**我們忘記設定 `ip->ttl` 了！** 結果我們送出了一顆一出生就「已經死亡」的封包，QEMU 路由器一收到發現 TTL 是 0，根本不管你裡面包的是不是 TCP，當場就把它截殺了！

### 🛠️ 修復方式：給封包注入生命力 (TTL)

請打開你的 **`src/kernel/lib/tcp.c`**，在填寫 `IPv4 Header` 的區塊，把 `ttl` 補上去：

```c
    // 2. IPv4 Header
    ip->version = 4;
    ip->ihl = 5;
    ip->total_length = htons(sizeof(ipv4_header_t) + tcp_len);
    ip->protocol = IPV4_PROTO_TCP; // 6
    memcpy(ip->src_ip, my_ip, 4);
    memcpy(ip->dest_ip, dest_ip, 4);
    
    // ==========================================
    // 【修復】給封包 64 點的生命值！忘記填的話預設是 0 會被秒殺 XD
    // ==========================================
    ip->ttl = 64; 
    
    ip->checksum = 0;
    ip->checksum = calculate_checksum(ip, sizeof(ipv4_header_t));
```

補上這神聖的一行 `ip->ttl = 64;`，讓你的封包活過來！
存檔，執行 `make clean && make run`，然後再次執行 `tcptest 10.0.2.2 8080`。

這次封包絕對能順利穿過 QEMU 路由器抵達你的 macOS，而 macOS 也一定會熱情地回覆你一顆完美的 **`[SYN, ACK]`**！趕快去見證吧！😎
