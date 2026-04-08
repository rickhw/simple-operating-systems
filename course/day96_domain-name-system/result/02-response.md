哈哈，Rick！這個 `[Malformed Packet]` 的錯誤，絕對是所有寫底層網路開發者必經的「成人禮」！🎉 

你路由 (Routing) 寫得非常完美，封包成功離開了區網，但是**封包的內容卻被無情地「截斷」了**！

你看第二張 Wireshark 的左下角，`UDP payload (3 bytes)`。
等一下，我們 DNS 的 Header 光是空的就高達 12 bytes 了，怎麼送出去的 Payload 只有 **3 bytes**？！

**🔪 真相只有一個：抓出隱形的殺手 `strlen()`**

還記得我們在 Day 90 實作 `sys_udp_send` 系統呼叫時，Kernel 端的接收器是怎麼寫的嗎？
```c
// src/kernel/kernel/syscall.c 裡面：
char* msg = (char*)regs->edx;
int len = strlen(msg);  // <---- 就是這傢伙惹的禍！！！
```
`strlen()` 是一個用來計算「字串」長度的函式，它的工作原理是一直讀取記憶體，直到遇到 **`0x00` (Null terminator)** 為止。

但是 DNS 封包是二進位 (Binary) 資料！我們來看看你的 DNS Header 前 4 個 bytes 在記憶體裡長怎樣：
1. `transaction_id = htons(0x1337)` → 變成 `0x13`, `0x37`
2. `flags = htons(0x0100)` → 變成 `0x01`, `0x00`

看到那個 **`0x00`** 了嗎？
當 Kernel 對這包 DNS 資料執行 `strlen()` 時，讀到第 4 個 byte 發現是 `0x00`，它就開心地判定：「喔！這個字串長度是 3！」
結果，精心準備的 DNS 查詢，就這樣被硬生生砍斷，只剩下 3 個 bytes 送出去了，Wireshark 當然會大喊這是一個 `Malformed` (格式錯誤) 的封包！XD

### 🛠️ 修復方式：全面升級 Syscall 支援二進位長度

我們必須讓 User Space 把「真正的長度」當作第 4 個參數 (對應暫存器 `esi`) 傳遞給 Kernel，徹底捨棄 `strlen`！

請跟著我做這 4 個修改：

---

**1. 修改 User Space 的宣告 (`src/user/include/net.h`)**
把 `sys_udp_send` 加上 `len` 參數：
```c
void sys_udp_send(uint8_t* ip, uint16_t port, uint8_t* data, int len);
```
*(💡 如果 `syscall.h` 裡也有宣告，記得一起改)*

**2. 修改 User Space 的實作 (`src/user/lib/net.c`)**
我們要把 `len` 透過 `esi` 暫存器 (`"S"`) 傳遞給 Kernel：
```c
void sys_udp_send(uint8_t* ip, uint16_t port, uint8_t* data, int len) {
    __asm__ volatile (
        "int $128"
        : 
        : "a"(SYS_NET_UDP_SEND), "b"((uint32_t)ip), "c"((uint32_t)port), "d"((uint32_t)data), "S"(len)
        : "memory"
    );
}
```

**3. 修改 Kernel Space 的接收端 (`src/kernel/kernel/syscall.c`)**
拔掉 `strlen`，直接使用從 `esi` 傳進來的長度：
```c
    else if (eax == 32) { // SYS_NET_UDP_SEND
        uint8_t* ip = (uint8_t*)regs->ebx;
        uint16_t port = (uint16_t)regs->ecx;
        uint8_t* data = (uint8_t*)regs->edx;
        int len = (int)regs->esi; // 【修復】直接讀取 User 傳來的真實長度！
        
        extern void udp_send_packet(uint8_t* dest_ip, uint16_t src_port, uint16_t dest_port, uint8_t* data, uint32_t len);
        udp_send_packet(ip, 12345, port, data, len);
        
        regs->eax = 0;
        return;
    }
```

**4. 修改我們的 DNS 應用程式 (`src/user/bin/host.c`)**
最後，把算好的 `query_len` 餵給 `sys_udp_send`：
```c
    // 先發第一顆換 ARP
    sys_udp_send(dns_server, 53, packet, query_len); // 補上 query_len
    for (volatile int j = 0; j < 50000000; j++) {}
    
    // 真實查詢
    sys_udp_send(dns_server, 53, packet, query_len); // 補上 query_len
```

---

完成這 4 個連鎖升級後，你的 Simple OS 就真正具備了發送任意二進位封包的能力了！
重新 `make clean && make run`，再次呼叫 `host google.com`。

這一次，Wireshark 裡絕對會出現完美無瑕的 `Standard query A google.com`，並且下一行就會緊跟著 Google 回傳給你的真實 IP 了！😎 趕快去體驗這個修好 Bug 的超級快感吧！
