哈哈，Rick！這畫面真的太美了！能看到你寫的 Kernel 成功在一片漆黑中點名並喚醒了隱藏在深處的硬體，這絕對是作業系統開發最讓人熱血沸騰的時刻！

關於你提到的 MAC 位址顯示問題，你的觀察非常正確。
這其實是 C 語言標準函式庫的一個小細節：在標準的 `printf` 中，`%x` 其實**不會**自動補上 `0x`。如果程式設計師想要印出 `0x`，通常會直接寫在字串裡，像是 `printf("0x%x", num);`。

我們只要把 `utils.c` 裡面的 `terminal_writestring("0x");` 刪掉，讓它回歸純粹的十六進位字串輸出，就可以解決這個問題了！

### 🛠️ 修正 1：還原純淨的 `%x`

請直接修改你的 **`utils.c`** 的 `case 'x':`：
```c
            case 'x': { // 十六進位整數
                unsigned int num = va_arg(args, unsigned int);
                char buffer[32];
                // 刪除 terminal_writestring("0x"); 這一行！
                itoa(num, buffer, 16);
                terminal_writestring(buffer);
                break;
            }
```
*(💡 小提醒：修改後，你在 `pci.c` 或其他地方如果想顯示 `0x`，請手動把它加進 format 字串裡，例如 `kprintf("Vendor: 0x%x\n", vendor);`。而 MAC 位址的 format 就可以保持純淨的 `%x:%x:%x:%x:%x:%x` 了！)*

---

解決了顯示問題，我們立刻推進主線！
**Day 83：喚醒 RTL8139 的靈魂！(接收緩衝區與中斷設定)**

現在網卡雖然開機了，但它就像一個沒有信箱的郵差，就算有網路封包送達，它也不知道該把它丟到記憶體的哪裡。
今天我們要劃分一塊記憶體作為 **Rx Ring Buffer (接收環狀緩衝區)** 借給網卡使用，並且告訴網卡：「當你收到封包時，請透過 IRQ (硬體中斷) 拍拍 CPU 的肩膀！」

請跟著我進行這 2 個步驟：

### 步驟 1：新增 16-bit I/O 函式與全域緩衝區

RTL8139 需要一塊**實體連續**的記憶體來放封包。在我們這個階段，最簡單且安全的方法就是在 Kernel 的 Data 段宣告一個巨大且對齊的陣列。

打開 **`src/kernel/lib/rtl8139.c`**，在檔案最上方加入 `outw` 函式以及 `rx_buffer`：

```c
#include "rtl8139.h"
#include "pci.h"
#include "tty.h"

extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t data);

// 【Day 83 新增】16-bit I/O 寫入
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

// 儲存網卡的 I/O 基準位址與 MAC 位址
static uint32_t rtl_iobase = 0;
static uint8_t mac_addr[6];

// ==========================================
// 【Day 83 新增】接收環狀緩衝區 (Rx Ring Buffer)
// 網卡要求最少 8K (8192) bytes，外加 16 bytes 的檔頭與 1500 bytes 容錯
// __attribute__((aligned(4096))) 確保它在實體記憶體上是 4K 對齊的
// ==========================================
static uint8_t rx_buffer[8192 + 16 + 1500] __attribute__((aligned(4096)));
```

---

### 步驟 2：設定接收模式與啟動中斷

接下來，我們要在 `init_rtl8139` 的最下方，也就是印完 MAC Address 之後，繼續設定網卡的引擎。

繼續編輯 **`src/kernel/lib/rtl8139.c`**：

```c
void init_rtl8139(uint8_t bus, uint8_t slot) {
    // ... [前面讀取 I/O Base, Power On, Reset, 以及印出 MAC 的程式碼保持不變] ...

    // ==========================================
    // 【Day 83 新增】網卡接收引擎與中斷設定
    // ==========================================

    // 5. 將我們準備好的 Rx Buffer 實體位址告訴網卡 (寫入 RBSTART 暫存器 0x30)
    // 註：這裡假設你的 Kernel 是 Identity Mapped (虛擬位址 == 實體位址)
    outl(rtl_iobase + 0x30, (uint32_t)rx_buffer);

    // 6. 設定中斷遮罩 (IMR - Interrupt Mask Register, 0x3C)
    // 0x0005 代表我們只想監聽 "Rx OK (接收成功)" 和 "Tx OK (發送成功)" 的中斷
    outw(rtl_iobase + 0x3C, 0x0005);

    // 7. 設定接收配置暫存器 (RCR - Receive Configuration Register, 0x44)
    // (AB|AM|APM|AAP) = 0xF 代表允許接收：廣播、多播、MAC符合的封包
    // WRAP (bit 7) = 1 代表當 Buffer 滿了時，網卡會自動從頭覆寫 (環狀機制)
    outl(rtl_iobase + 0x44, 0xf | (1 << 7));

    // 8. 正式啟動接收 (Rx) 與發送 (Tx) 引擎！
    // Command Register (0x37), RE(bit 3)=1, TE(bit 2)=1 -> 組合起來就是 0x0C
    outb(rtl_iobase + 0x37, 0x0C);

    // 9. 從 PCI Config Space 讀出這張網卡被分配到的 IRQ 號碼
    uint32_t irq_info = pci_read_config_word(bus, slot, 0, 0x3C);
    uint8_t irq_line = irq_info & 0xFF; // 取出最低 8 bits
    
    kprintf("[RTL8139] Hardware initialized. Assigned IRQ: %d\n", irq_line);
}
```
*(💡 如果 `pci.c` 裡有呼叫 `outl`，你也可以直接把 `pci.c` 裡的 `outl` 拿過來用，或者直接在 `rtl8139.c` 上方宣告一個 `static inline void outl(...)`)*

---

### 🚀 驗證硬體的全面甦醒

存檔編譯並執行 `make clean && make run`！

這一次在 CLI 的開機畫面上，除了看到純淨、沒有 `0x` 搗亂的漂亮 MAC Address 之外，你還會看到最後一行印出：
**`[RTL8139] Hardware initialized. Assigned IRQ: 11`** (IRQ 號碼可能是 11 或 10，由 QEMU 決定)。

只要這個 IRQ 號碼順利印出來，就代表你的網卡硬體引擎已經在背景全速運轉了！它正在靜靜地聆聽網路線上的風吹草動。準備好把這個 IRQ 連接到我們的中斷向量表 (IDT) 了嗎？😎
