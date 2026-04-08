太神啦，Rick！🎉🎉🎉

看到那排 `google.com`、`facebook.com`、`github.io` 還有 `thread.com` 精準地轉換成真實的 IP 位址，這絕對是 Simple OS 開發史上最偉大、最讓人起雞皮疙瘩的一刻！

你親手寫的程式碼，從最底層的 PCI 硬體掃描、網卡 DMA 傳輸，一路穿透 Ethernet、IP、UDP，最終成功用二進位格式解析出了全球最大科技公司的伺服器位置。這代表你的作業系統已經完全具備了在現代網際網路上自由航行的能力！

既然我們已經能用 DNS 查到 IP，也具備了 TCP 的連線能力，距離我們的終極目標 **下載真實網頁** 就只差最後一塊拼圖了：**接收 TCP 傳來的資料 (TCP Receive)**。

**Day 97：聽見網路的聲音 — TCP 接收與 `wget` 網頁下載**

之前我們實作 TCP 時，只教它怎麼「發送 (PSH)」資料，還沒教它怎麼「接收」。當 Web Server 收到我們的 HTTP 請求後，會回傳帶有 `TCP_PSH` 旗標的 HTML 原始碼。我們現在要把這個網頁抓下來！

請跟著我進行這最後 3 個步驟：

---

### 步驟 1：實作 TCP 接收信箱 (`src/kernel/net/tcp.c` & `rtl8139.c`)

就像 UDP 一樣，我們要給 TCP 準備一個全域的 Buffer 來存放收到的網頁資料。

**1. 打開 `src/kernel/net/tcp.c`，在上方加入接收 Buffer 與讀取函式：**
```c
// ==========================================
// 【Day 97 新增】TCP 接收信箱
// ==========================================
static char tcp_rx_buffer[4096]; // 準備一個夠大的 Buffer 放 HTML
static int tcp_rx_len = 0;

int tcp_recv_data(char* buffer) {
    if (tcp_rx_len > 0) {
        memcpy(buffer, tcp_rx_buffer, tcp_rx_len);
        int len = tcp_rx_len;
        tcp_rx_len = 0; 
        return len;
    }
    return 0;
}
```

**2. 打開 `src/kernel/drivers/net/rtl8139.c`，攔截帶有資料的 TCP 封包：**
找到你之前處理 `(tcp->flags & TCP_SYN) && (tcp->flags & TCP_ACK)` 的下方，加上針對 `TCP_PSH` (推送資料) 的處理：

```c
                    // ... 之前的 SYN, ACK 處理 ...
                    
                    // ==========================================
                    // 【Day 97 新增】攔截 TCP Payload (PSH 旗標)
                    // ==========================================
                    if (tcp->flags & 0x08) { // 0x08 就是 TCP_PSH
                        // 計算 TCP Header 長度
                        uint32_t header_len = (tcp->data_offset >> 4) * 4;
                        uint8_t* payload = (uint8_t*)tcp + header_len;
                        // 計算真實資料長度 = IP 總長 - IP 標頭 - TCP 標頭
                        int payload_len = ntohs(ip->total_length) - (ip->ihl * 4) - header_len;

                        if (payload_len > 0) {
                            extern char tcp_rx_buffer[];
                            extern int tcp_rx_len;
                            int copy_len = (payload_len < 4095) ? payload_len : 4095;
                            memcpy(tcp_rx_buffer, payload, copy_len);
                            tcp_rx_len = copy_len;
                            tcp_rx_buffer[copy_len] = '\0'; // 字串結尾

                            kprintf("[TCP Rx] Received %d bytes of Web Data!\n", payload_len);

                            // 【關鍵】收到資料後，必須回傳 ACK 告訴伺服器「我收到了」
                            // 我們的 ACK 號碼要是對方的 Seq + 剛收到的資料長度
                            extern void tcp_send_ack(uint8_t*, uint16_t, uint16_t, uint32_t, uint32_t);
                            tcp_send_ack(ip->src_ip, dest_port, src_port, ack, seq + payload_len);
                        }
                    }
```
*(💡 注意：要在 `tcp.c` 把 `tcp_rx_buffer` 和 `tcp_rx_len` 的 `static` 拿掉，讓 `rtl8139.c` 可以 `extern` 存取，或者寫一個 Setter 函式給 `rtl8139.c` 呼叫。)*

---

### 步驟 2：開通 `sys_tcp_recv` 系統呼叫

我們要給它分配 **`38`** 號系統呼叫。

**1. Kernel 端 (`src/kernel/kernel/syscall.c`)：**
```c
    else if (eax == 38) { // SYS_NET_TCP_RECV
        extern int tcp_recv_data(char* buffer);
        regs->eax = tcp_recv_data((char*)regs->ebx);
        return;
    }
```

**2. User 端 (`src/user/include/syscall.h` & `src/user/lib/net.c` / `net.h`)：**
```c
// syscall.h
#define SYS_NET_TCP_RECV 38

// net.c
int sys_tcp_recv(char* buffer) {
    int ret;
    __asm__ volatile ("int $128" : "=a"(ret) : "a"(SYS_NET_TCP_RECV), "b"((uint32_t)buffer) : "memory");
    return ret;
}

// net.h
int sys_tcp_recv(char* buffer);
```

---

### 步驟 3：撰寫終極武器 `wget.c` 

結合 DNS 與 TCP，直接向 Web Server 要網頁！為了確保成功，我們先不用 HTTPS (因為牽涉複雜的 TLS 加密)，我們向一個純 HTTP 的測試網站請求資料。

建立 **`src/user/bin/wget.c`**：
```c
#include "stdio.h"
#include "net.h"
#include "string.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: wget <ip_address>\n");
        return 1;
    }

    uint8_t target_ip[4];
    parse_ip(argv[1], target_ip); // 為了簡化，我們先手動餵剛才 DNS 查到的 IP
    uint16_t port = 80; // HTTP 預設 Port 是 80

    printf("Connecting to Web Server at %d.%d.%d.%d:80...\n", 
           target_ip[0], target_ip[1], target_ip[2], target_ip[3]);

    // 1. TCP 3-Way Handshake
    sys_tcp_connect(target_ip, port);
    for (volatile int j = 0; j < 50000000; j++) {} 
    sys_tcp_connect(target_ip, port);
    for (volatile int j = 0; j < 50000000; j++) {} 

    // 2. 組裝 HTTP GET 請求 (這就是瀏覽器底層在做的事！)
    char* http_request = 
        "GET / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Connection: close\r\n"
        "\r\n";

    printf("Sending HTTP GET Request...\n");
    sys_tcp_send(target_ip, port, http_request);

    // 3. 等待並接收 HTML 原始碼
    printf("Waiting for server response...\n");
    char rx_buf[4096];
    int rx_len = 0;
    int retry = 0;
    
    while (retry < 10) {
        rx_len = sys_tcp_recv(rx_buf);
        if (rx_len > 0) {
            printf("\n--- HTTP RESPONSE RECEIVED ---\n\n");
            printf("%s\n", rx_buf);
            printf("\n------------------------------\n");
            break;
        }
        for (volatile int i = 0; i < 20000000; i++) {} // Delay
        retry++;
    }

    if (rx_len == 0) printf("Timeout: No data received.\n");

    // 4. 掛斷電話
    sys_tcp_close(target_ip, port);
    return 0;
}
```

---

### 🚀 最終驗收：抓下網際網路的第一頁 HTML！

1. 執行 `make clean && make run`
2. 先用你剛寫好的工具查一下沒有強制跳轉 HTTPS 的測試網站 IP（例如 `example.com` 通常是 `93.184.216.34`，你可以用 `host example.com` 查一下最新的）。
3. 執行：
   ```bash
   SimpleOS> wget 93.184.216.34
   ```

看著你的 Simple OS 透過 TCP 送出純文字的 `GET / HTTP/1.1`，然後一秒鐘後，滿滿的 `<html><head><title>Example Domain</title>...` 噴發在你的畫面上！這就是萬維網 (World Wide Web) 最真實的運作原理！
