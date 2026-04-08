哇，Rick！這目錄結構實在是**太美了**！😍😍😍

看到這個重構後的架構，簡直就像在看精簡版的 Linux Kernel 原始碼一樣！從 `arch` (架構層)、`drivers` (驅動層)、`fs` (檔案系統)、`mm` (記憶體管理) 到 `net` (網路層) 分門別類得清清楚楚。這代表你對作業系統的宏觀架構已經有了非常深刻的理解。這種模組化的設計，會讓我們接下來開發 System Call 和 User Space App 變得非常輕鬆。

**Day 89：跨越界線！將 Ping 封裝到 User Space (`ping.elf`)**

我們現在的 Ping 測試程式碼是寫在 Kernel 裡的（Ring 0），但真實作業系統中的 `ping` 指令是一支跑在 User Space（Ring 3）的應用程式。因為 Ring 3 沒有權限直接操控 RTL8139 網卡，所以我們必須開一扇「傳送門」，也就是 **System Call (系統呼叫)**。



為了保持目前的簡單性，我們先實作「User Space 觸發，Kernel 負責發送並印出接收結果」的架構。
請跟著我進行這 4 個步驟：

---

### 步驟 1：在 Kernel 註冊網路系統呼叫 (`src/kernel/kernel/syscall.c`)

我們要分配一個系統呼叫編號（假設是 25，你可以依據你目前的清單順序調整）給 Ping 使用。

打開 **`src/kernel/kernel/syscall.c`**：
```c
#include "icmp.h" // 引入我們網路層的 ICMP 標頭檔

// 假設你的系統呼叫分派長這樣 (依據你實際的實作調整)
void syscall_handler(registers_t* regs) {
    uint32_t syscall_num = regs->eax;

    switch (syscall_num) {
        // ... 前面其他的 syscall (例如 SYS_WRITE, SYS_READ 等) ...

        case 25: { // SYS_NET_PING (假設編號 25)
            // User Space 會把 IP 陣列的指標透過 ebx 傳遞過來
            uint8_t* target_ip = (uint8_t*)regs->ebx;
            ping_send_request(target_ip);
            break;
        }
        
        default:
            // kprintf("Unknown syscall: %d\n", syscall_num);
            break;
    }
}
```

---

### 步驟 2：在 User Space 提供 Syscall API (`src/user/`)

接下來，我們要在 User Space 的 C 函式庫中，包裝這個組合語言呼叫。

**1. 打開 `src/user/include/syscall.h`**，加入宣告：
```c
#define SYS_NET_PING 25

// 宣告給 user app 使用的 API
void sys_ping(uint8_t* ip);
```

**2. 打開 `src/user/lib/syscall.c`**，實作呼叫：
```c
#include "syscall.h"

// 假設你的底層 syscall 觸發函式長這樣 (透過 int 0x80 或 int 0x82)
void sys_ping(uint8_t* ip) {
    // 呼叫 25 號 Syscall，並將 IP 陣列的記憶體位址作為第一個參數傳遞
    syscall1(SYS_NET_PING, (uint32_t)ip); 
}
```

---

### 步驟 3：撰寫 `ping.c` 應用程式 (`src/user/bin/ping.c`)

現在我們可以正式來寫這支 `ping.elf` 了！
因為 User 在 Shell 裡輸入的會是字串（例如 `ping 10.0.2.2`），我們需要先寫一個簡單的 `parse_ip` 函式，把字串轉換成 `uint8_t[4]` 的陣列。

建立 **`src/user/bin/ping.c`**：
```c
#include "stdio.h"
#include "syscall.h"
#include "string.h"

// 簡易的字串轉 IP 陣列函式 (例如 "10.0.2.2" -> {10, 0, 2, 2})
void parse_ip(char* str, uint8_t* ip) {
    int i = 0;
    int num = 0;
    while (*str) {
        if (*str == '.') {
            ip[i++] = num;
            num = 0;
        } else {
            num = num * 10 + (*str - '0');
        }
        str++;
    }
    ip[i] = num; // 存入最後一個數字
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: ping <ip_address>\n");
        return 1;
    }

    uint8_t target_ip[4];
    parse_ip(argv[1], target_ip);

    printf("PING %d.%d.%d.%d 32 bytes of data.\n", 
           target_ip[0], target_ip[1], target_ip[2], target_ip[3]);

    // 連發 4 顆 Ping (模擬 Windows/Linux 的行為)
    for (int i = 0; i < 4; i++) {
        sys_ping(target_ip);
        
        // 延遲一下，避免發送過快導致網卡信箱塞車或路由器忽略
        // (如果有 sys_sleep 可以用，沒有的話就用 busy wait)
        for (volatile int j = 0; j < 50000000; j++) {} 
    }

    return 0;
}
```

---

### 步驟 4：清理 Kernel 開機測試碼 (`src/kernel/init/main.c`)

最後一步，既然我們要把發射權交給 User Space 的 Shell 了，請把你昨天加在 Kernel 開機流程裡的那段測試迴圈給**刪除**！

打開 **`src/kernel/init/main.c`**（依據你重構後的新路徑）：
```c
    init_pci();     
    
    // ==========================================
    // 【刪除這段】我們不需要 Kernel 自動發射 Ping 了！
    // ==========================================
    // uint8_t router_ip[4] = {10, 0, 2, 2};
    // for (int i = 0; i < 4; i++) { ... }
```

---

### 🚀 驗收：從 Shell 發動網路攻擊 (X) 網路探索 (O)！

重新執行 `make clean && make run`！

這次開機後，系統會安安靜靜地停在你的 `SimpleOS>` 提示字元。
接著，請在你的 Shell 裡驕傲地敲下這行指令：

```bash
SimpleOS> ping 10.0.2.2
```

如果一切順利，你將會看到 `ping.elf` 解析了字串，並且 Kernel 在背景接手，完美的 ARP 解析與 Ping Reply 交響樂將會再次於你的畫面上奏響！趕快試試看你的第一支網路應用程式吧！😎
