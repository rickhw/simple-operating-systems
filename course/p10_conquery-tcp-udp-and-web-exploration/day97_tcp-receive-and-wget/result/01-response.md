哈哈，Rick！這個畫面實在是太有 OS 開發的醍醐味了！😂

你發現了嗎？當你敲下 `wget 93.184.216.34` 時，畫面上**連第一行 `Connecting to Web Server...` 都還沒印出來**，系統就直接噴了 Segmentation Fault！而且 Wireshark 裡面空空如也，代表根本連一顆封包都還沒送出去！

兇手不是網路堆疊，而是**「記憶體與堆疊溢位 (Stack Overflow)」**！而且我們接下來還會撞到另一個**「TCP 路由盲點」**。

我們來一一把這兩個隱藏的魔王拆解掉！

---

### 🕵️ 案發現場 1：User Space 的堆疊溢位 (Stack Overflow)

在你的 `wget.c` 裡面有這麼一行：
```c
char rx_buf[4096];
```
這宣告了一個 4KB 的「區域變數 (Local Variable)」。在 C 語言中，區域變數是分配在**堆疊 (Stack)** 上的。
我們 Simple OS 分配給應用程式的 User Stack 通常非常小（可能只有 1 個 Page，也就是 4096 bytes）。當你一口氣在 Stack 塞入 4096 bytes 的陣列時，堆疊指標 (`esp`) 瞬間被推到了未映射的記憶體頁面 (`0x83FEF44`)。當程式準備呼叫 `printf` 或執行下一步時，一碰到這個不存在的頁面，Kernel 就會無情地發出 `Page not present in Write mode` 的 Segfault！

**🛠️ 修復方式：將它移到 `.bss` 區段**
只要加上 `static` 關鍵字，編譯器就會把它從 Stack 移到全域變數所在的資料區段，瞬間解除堆疊爆炸的危機！
打開 **`src/user/bin/wget.c`**：
```c
    // 3. 等待並接收 HTML 原始碼
    printf("Waiting for server response...\n");
    
    // 【修復 1】加上 static，拯救弱小的 User Stack！
    static char rx_buf[4096]; 
    
    int rx_len = 0;
```

---

### 🕵️ 案發現場 2：TCP 忘記問路了 (Gateway Routing)

就算你修好了 Segfault，封包依然出不去。為什麼？
因為在 Day 96 我們幫 **UDP** 加上了「如果目標不是 `10.x.x.x`，就去找閘道器 `10.0.2.2`」的路由邏輯，但是我們**忘記幫 TCP 加上去了！**
所以當你的 `sys_tcp_connect` 試圖去問 `93.184.216.34` 的 MAC 位址時，沒有任何人會理它。

**🛠️ 修復方式：建立 TCP 的路由小幫手**
打開 **`src/kernel/net/tcp.c`**，我們在檔案上方（全域變數區的下面）加入這個自動判斷下一跳的 Helper 函式：

```c
// ==========================================
// 【Day 97 新增】TCP 路由與 MAC 解析小幫手
// ==========================================
static uint8_t* tcp_get_target_mac(uint8_t* dest_ip) {
    uint8_t gateway_ip[4] = {10, 0, 2, 2};
    uint8_t* next_hop = (dest_ip[0] != 10) ? gateway_ip : dest_ip; // 判斷外網
    
    uint8_t* mac = arp_lookup(next_hop);
    if (!mac) {
        kprintf("[TCP] MAC unknown for next hop! Initiating ARP...\n");
        arp_send_request(next_hop);
    }
    return mac;
}
```

接著，把 `tcp_send_syn`, `tcp_send_ack`, `tcp_send_data`, `tcp_send_fin` 這 4 個函式開頭的 ARP 查詢，**全部換成呼叫這個小幫手**：

```c
// 把這段：
// uint8_t* target_mac = arp_lookup(dest_ip);
// if (target_mac == 0) { ... }

// 【修復 2】全部統一替換成這樣：
    uint8_t* target_mac = tcp_get_target_mac(dest_ip);
    if (!target_mac) return;
```

---

### 💣 案發現場 3：拆除 Kernel 的未爆彈

在你改 `tcp.c` 的時候，順手幫我拆除一顆定時炸彈！
在 `tcp_send_data` 和 `tcp_send_fin` 裡面，你寫了：
`uint8_t pseudo_buf[2000];`
這會在 **Kernel Stack** 上分配 2000 bytes！Kernel Stack 通常只有 4KB，這跟剛剛的 `wget` 是一樣危險的行為，隨時可能導致整個系統崩潰 (Kernel Panic)。

**🛠️ 修復方式：精準計算大小**
修改 `tcp_send_data` 的陣列宣告：
```c
    // 【修復 3】要多少拿多少，不要在 Kernel Stack 亂灑記憶體！
    uint8_t pseudo_buf[sizeof(tcp_pseudo_header_t) + sizeof(tcp_header_t) + len]; 
```
修改 `tcp_send_fin` 的陣列宣告 (因為沒有 len)：
```c
    // 【修復 3】FIN 封包連 Payload 都沒有，只需要這點空間！
    uint8_t pseudo_buf[sizeof(tcp_pseudo_header_t) + sizeof(tcp_header_t)]; 
```

---

### 🚀 再次出擊！

這 3 個致命的隱藏 Bug 拆除後，你的系統將會無堅不摧！
重新 `make clean && make run`，然後滿懷期待地敲下：
```bash
SimpleOS> wget 93.184.216.34
```

這一次，你將會看見真實世界的 HTML 原始碼如瀑布般傾瀉在你的螢幕上！準備好截下那張神聖的畫面吧！😎
