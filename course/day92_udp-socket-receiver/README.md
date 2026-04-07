太神啦，Rick！🎉🎉🎉

看到 macOS 的終端機和 QEMU 裡的 Simple OS 成功連線，這絕對是歷史性的一刻！你已經成功打破了「虛擬」與「現實」的次元壁，讓兩個獨立的作業系統透過標準網路協定展開了對話！

不過，你可能會發現一個小小的架構問題：現在收到訊息時，是 **Kernel (核心)** 擅自把訊息印在畫面上。在真正的作業系統裡，Kernel 應該要把訊息「轉交」給在 User Space 執行的應用程式（像是 Line 或瀏覽器），這就是大名鼎鼎的 **Socket API (通訊端點)** 的雛形！

**Day 92：打造 UDP Socket 接收器 (將訊息傳遞給 User Space)**

今天，我們要讓 User Space 的 App 能夠「綁定 (Bind)」一個 Port，並且「聆聽 (Listen)」這個 Port 傳來的訊息。我們來寫一支 `udprecv.elf` 的伺服器程式！

請跟著我進行這 4 個步驟：

---

### 步驟 1：實作核心 UDP 緩衝區 (`src/kernel/lib/udp.c`)

我們要準備一個信箱，讓 Kernel 把特定 Port 收集到的訊息存起來，等 User App 來拿。

打開你的 **`src/kernel/lib/udp.c`**，在檔案的最上方加入這些變數與函式：

```c
#include "udp.h"
#include "ipv4.h"
// ... (其他 include 保持原樣)

// ==========================================
// 【Day 92 新增】簡易 UDP Socket 信箱
// ==========================================
static uint16_t bound_port = 0;       // 目前被 User App 監聽的 Port
static char rx_msg_buffer[256];       // 存放接收到的文字
static int rx_msg_ready = 0;          // 標記是否有新訊息 (1: 有, 0: 無)

// 讓 User App 綁定 Port
void udp_bind_port(uint16_t port) {
    bound_port = port;
    rx_msg_ready = 0;
    kprintf("[Kernel UDP] Port %d bound by User Space.\n", port);
}

// 讓 User App 讀取訊息 (非阻塞)
int udp_recv_data(char* buffer) {
    if (rx_msg_ready) {
        strcpy(buffer, rx_msg_buffer);
        rx_msg_ready = 0; // 讀取完畢，清空狀態
        return 1;         // 回傳 1 代表成功讀到新訊息
    }
    return 0;             // 回傳 0 代表目前沒訊息
}

// 讓 rtl8139 驅動把封包交給這裡
void udp_handle_receive(uint16_t dest_port, uint8_t* payload, uint16_t payload_len) {
    // 檢查這個封包是不是送到我們正在監聽的 Port
    if (bound_port != 0 && dest_port == bound_port) {
        int copy_len = (payload_len < 255) ? payload_len : 255;
        memcpy(rx_msg_buffer, payload, copy_len);
        rx_msg_buffer[copy_len] = '\0'; // 確保字串結尾
        rx_msg_ready = 1;               // 舉起新訊息旗標！
    }
}

// ... 下面原本的 udp_send_packet 保持不變 ...
```

---

### 步驟 2：將網卡驅動對接 Socket 信箱 (`src/kernel/lib/rtl8139.c`)

打開 **`src/kernel/lib/rtl8139.c`**，找到昨天處理 UDP 的地方，我們不要讓 Kernel 直接印出文字了，把它轉交給 `udp_handle_receive`！

```c
                // ...
                else if (ip->protocol == 17) { // 17 代表 UDP
                    udp_header_t* udp = (udp_header_t*)(packet_data + sizeof(ethernet_header_t) + (ip->ihl * 4));
                    uint8_t* payload = (uint8_t*)udp + sizeof(udp_header_t);
                    uint16_t payload_len = ntohs(udp->length) - sizeof(udp_header_t);
                    
                    // ==========================================
                    // 【Day 92 修改】不要直接 kprintf，交給 UDP 模組處理！
                    // ==========================================
                    extern void udp_handle_receive(uint16_t dest_port, uint8_t* payload, uint16_t payload_len);
                    udp_handle_receive(ntohs(udp->dest_port), payload, payload_len);
                }
```

---

### 步驟 3：開啟 Syscall 通道 (`syscall.c`, `syscall.h`)

我們要開通 **33 號 (Bind)** 和 **34 號 (Recv)** 系統呼叫。

**1. Kernel 端 (`src/kernel/kernel/syscall.c`)：**
```c
    // ... 在 syscall_handler 裡面新增：
    else if (eax == 33) { // SYS_NET_UDP_BIND
        extern void udp_bind_port(uint16_t port);
        udp_bind_port((uint16_t)regs->ebx);
        regs->eax = 0;
        return;
    }
    else if (eax == 34) { // SYS_NET_UDP_RECV
        extern int udp_recv_data(char* buffer);
        regs->eax = udp_recv_data((char*)regs->ebx); // 回傳 1 或 0
        return;
    }
```

**2. User 端 (`src/user/include/syscall.h`)：**
```c
#define SYS_NET_UDP_BIND 33
#define SYS_NET_UDP_RECV 34

void sys_udp_bind(uint16_t port);
int sys_udp_recv(char* buffer);
```

**3. User 端 (`src/user/lib/syscall.c`)：**
```c
void sys_udp_bind(uint16_t port) {
    __asm__ volatile ("int $128" : : "a"(SYS_NET_UDP_BIND), "b"((uint32_t)port) : "memory");
}

int sys_udp_recv(char* buffer) {
    int ret;
    __asm__ volatile ("int $128" : "=a"(ret) : "a"(SYS_NET_UDP_RECV), "b"((uint32_t)buffer) : "memory");
    return ret;
}
```

---

### 步驟 4：開發 UDP 伺服器應用程式 (`src/user/bin/udprecv.c`)

我們來寫一支永遠在背景等待訊息的接收器！

建立 **`src/user/bin/udprecv.c`**：
```c
#include "stdio.h"
#include "syscall.h"

int main() {
    uint16_t port = 8080;
    
    printf("Starting Simple OS UDP Server on port %d...\n", port);
    
    // 1. 綁定通訊埠
    sys_udp_bind(port);
    
    printf("Waiting for messages from macOS...\n");
    printf("(Run 'echo \"msg\" | nc -u 127.0.0.1 %d' on macOS)\n", port);
    printf("----------------------------------------\n");

    char buffer[256];

    // 2. 進入無限迴圈聆聽 (Server Loop)
    while (1) {
        // 如果成功讀取到新訊息
        if (sys_udp_recv(buffer) == 1) {
            printf(">> [INCOMING]: %s\n", buffer);
        }
        
        // 稍微延遲一下，避免 100% 佔用 CPU 資源
        for (volatile int i = 0; i < 5000000; i++) {}
    }

    return 0; // 實際上不會走到這裡
}
```
*(💡 記得在 User Makefile 的編譯清單加入這支 App！)*

---

### 🚀 驗收：打造跨系統聊天室！

重新 `make clean && make run`！

1. 在你的 Simple OS Shell 中輸入 `udprecv`。
2. 畫面會停留在 `Waiting for messages from macOS...`。
3. 切換到你的 macOS 終端機，瘋狂輸入：
   ```bash
   echo "Testing User Space Socket!" | nc -u -w 1 127.0.0.1 8080
   echo "This is so cool!" | nc -u -w 1 127.0.0.1 8080
   ```

看著你的 Simple OS 畫面，你會發現每一句來自現實世界的話語，都順利突破了 Kernel 的防線，被你的 User Space 應用程式完美接住並印出來了！這就是網路伺服器的真正核心原理，恭喜你成功實作了它！😎
