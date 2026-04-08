哈哈，Rick！這真的是太完美了！🎉🎉🎉

你把網路中斷裡的 `kprintf` 封印之後，整個系統的架構就變得非常健康了。中斷處理常式（Top Half）只負責把資料秒速收進 Buffer，然後把繪圖跟字串輸出的工作留給 User Space 的應用程式去煩惱，這就是現代作業系統最標準的**「中斷與底層分離」**設計！

現在，你的 Simple OS 已經穩定到不會再崩潰，而且成功拿到了真實世界的 HTML 原始碼！

這代表我們已經準備好迎接 Phase 10 的最終大結局了！🔥

---

### 🌍 Day 98：終極大一統 — 將網際網路寫入實體硬碟 (Saving the Web to Disk)

在真實世界裡，`wget` 絕對不會只是把網頁印在畫面上就結束了，它的使命是**「下載檔案」**。
今天，我們要將你之前辛辛苦苦實作的**檔案系統 (SimpleFS)**，與現在的**網路系統 (TCP/IP)** 完美結合！

我們要讓 `wget.c` 讀取這包 HTTP 回應，把前面那些雜亂的 HTTP Headers 剝除，然後把純淨的 HTML Payload 寫入虛擬硬碟中的 `index.html`。

但在此之前，我翻了一下你的 `syscall.h`，發現我們有一個「歷史遺留的缺件」：**我們有 `SYS_READ` 和 `SYS_OPEN`，卻還沒有實作 `SYS_WRITE` 系統呼叫！**

請跟著我進行這最後的 3 個步驟，完成這個史詩級的整合：

---

### 步驟 1：在 Kernel 打通 `SYS_WRITE` (`src/kernel/kernel/syscall.c`)

我們要分配 **39 號** 給 `SYS_WRITE`，並將它對接到 VFS (虛擬檔案系統)。

打開 **`src/kernel/kernel/syscall.c`**，加入這個 Handler：

```c
// ==========================================
// 【Day 98 新增】寫入檔案的系統呼叫
// ==========================================
static int sys_write_handler(registers_t *regs) {
    int fd = (int)regs->ebx;
    uint8_t* buffer = (uint8_t*)regs->ecx;
    uint32_t size = (uint32_t)regs->edx;

    // 確認 fd 是合法的 (3 以上，且已被 open)
    if (fd >= 3 && fd < MAX_FD && fd_table[fd] != 0) {
        // 呼叫虛擬檔案系統的寫入函式 (假設每次都從 offset 0 覆寫)
        extern uint32_t vfs_write(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
        return vfs_write(fd_table[fd], 0, size, buffer);
    }
    return -1;
}
```

然後，在同一個檔案最下方的 `syscall_table` 陣列中，把它註冊進去：
```c
    [SYS_NET_TCP_RECV]   = sys_net_tcp_recv_handler,
    // 【新增】註冊 39 號呼叫
    [39]                 = sys_write_handler, 
    [SYS_SET_DISPLAY_MODE] = sys_set_display_mode_handler,
```

---

### 步驟 2：在 User Space 宣告 API (`src/user/include/syscall.h`)

打開你的 **`src/user/include/syscall.h`**，補上 39 號定義：

```c
#define SYS_NET_TCP_CLOSE  37
#define SYS_NET_TCP_RECV   38
#define SYS_WRITE          39  // 【Day 98 新增】
#define SYS_SET_DISPLAY_MODE 99
```

---

### 步驟 3：為 `wget.c` 裝上 HTTP 解析器與存檔引擎

我們要讓 `wget` 尋找 HTTP 協定中最重要的分水嶺：**`\r\n\r\n` (兩個換行符號)**。在這之前的是 Header，在這之後的就是我們要寫入硬碟的真實檔案！

請打開 **`src/user/bin/wget.c`**，把接收資料的部分（步驟 3）替換成以下的終極版本：

```c
    // 3. 等待並接收 HTML 原始碼
    printf("Waiting for server response...\n");
    static char rx_buf[4096];
    int rx_len = 0;
    int retry = 0;
    
    while (retry < 10) {
        rx_len = sys_tcp_recv(rx_buf);
        if (rx_len > 0) {
            printf("\n--- TCP Data Received (%d bytes) ---\n", rx_len);
            
            // 【HTTP 解析器】尋找 \r\n\r\n
            char* body = 0;
            for (int i = 0; i < rx_len - 3; i++) {
                if (rx_buf[i] == '\r' && rx_buf[i+1] == '\n' &&
                    rx_buf[i+2] == '\r' && rx_buf[i+3] == '\n') {
                    body = &rx_buf[i+4]; // Payload 的起點！
                    break;
                }
            }

            if (body != 0) {
                int body_len = rx_len - (body - rx_buf);
                printf("HTTP Headers stripped. Payload size: %d bytes.\n", body_len);
                
                // 【檔案系統整合】寫入虛擬硬碟！
                printf("Saving to 'index.html'...\n");
                
                // 1. 建立檔案 (SYS_CREATE = 14)
                syscall(14, (int)"index.html", 0, 0, 0, 0); 
                
                // 2. 開啟檔案取得 FD (SYS_OPEN = 3)
                int fd = syscall(3, (int)"index.html", 0, 0, 0, 0); 
                
                if (fd >= 3) {
                    // 3. 寫入檔案 (SYS_WRITE = 39)
                    syscall(39, fd, (int)body, body_len, 0, 0);
                    printf("Success! Webpage saved to disk.\n");
                } else {
                    printf("Error: Could not open index.html for writing.\n");
                }
            } else {
                printf("Error: Could not find HTTP body in response.\n");
            }
            break;
        }
        for (volatile int i = 0; i < 20000000; i++) {} // Delay
        retry++;
    }

    // 4. 掛斷電話 (保持不變)
```

*(💡 這裡我們直接使用底層的 `syscall(...)` 函式，避免因為缺少 User Space Wrapper 而產生編譯錯誤)*

---

### 🚀 驗收：見證 OS 開發的「聖杯」！

存檔，執行 `make clean && make run`！

進入 Simple OS 後，照順序執行這 3 個神聖的指令：

1. **`wget 93.184.216.34`**
   （你會看到系統成功剝除 HTTP Header，並印出 `Success! Webpage saved to disk.`）
2. **`ls`**
   （仔細看你的 SimpleFS 目錄列表，裡面將會歷史性地多出一個 `index.html` 檔案！）
3. **`cat index.html`**
   （把剛才存進去的檔案印出來看，這就是你剛剛從網際網路上「抓」下來的戰利品！）

當你打完這三個指令，Phase 10 就真正畫下了最完美的句點。你的作業系統不僅能連上外網，還能實實在在地把網路資源留存在自己的世界裡了！準備好迎接這份巨大的成就感吧！😎
