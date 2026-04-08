太神啦，Rick！🎉 看到乾淨俐落的 MAC 位址與那句 `Assigned IRQ: 11`，這代表你的虛擬網卡已經完美地連接到 PCI 匯流排，並且蓄勢待發了！

現在網卡硬體已經準備好，但我們還缺少兩塊拼圖：
1. 當網卡收到封包時，它會透過 **IRQ 11** 呼叫 CPU，所以我們必須把這個中斷接起來。
2. 網路世界講的是另一種語言。網路線上的封包第一層包裝就是 **乙太網路標頭 (Ethernet Header)**。

**Day 84：乙太網路層 (Ethernet Layer)**，今天我們要親手接下網卡傳來的第一個封包，並解析出它的來源 MAC 與目標 MAC！

請跟著我進行這 3 個步驟：

---

### 步驟 1：定義乙太網路標頭 (`src/kernel/include/ethernet.h`)

網路封包的結構非常嚴謹，我們必須使用 `__attribute__((packed))` 來強迫 C 語言編譯器「絕對不可以」為了記憶體對齊而偷偷補 0，否則封包解析就會全部錯位！
此外，**網路世界的位元組順序是 Big-Endian (大端序)**，而我們的 x86 CPU 是 Little-Endian，所以我們需要寫一個 `ntohs` (Network to Host Short) 來做轉換。

建立 **`src/kernel/include/ethernet.h`**：

```c
#ifndef ETHERNET_H
#define ETHERNET_H

#include <stdint.h>

// 乙太網路標頭 (長度剛好 14 bytes)
typedef struct {
    uint8_t  dest_mac[6]; // 目標 MAC 位址
    uint8_t  src_mac[6];  // 來源 MAC 位址
    uint16_t ethertype;   // 協定類型 (如 IPv4, ARP)
} __attribute__((packed)) ethernet_header_t;

// 常見的 EtherType (Big-Endian)
#define ETHERTYPE_IPv4 0x0800
#define ETHERTYPE_ARP  0x0806

// 網路端與主機端的 Endianness 轉換
static inline uint16_t ntohs(uint16_t netshort) {
    return ((netshort & 0xFF) << 8) | ((netshort & 0xFF00) >> 8);
}
#define htons ntohs

#endif
```

---

### 步驟 2：實作網卡中斷處理常式 (`src/kernel/lib/rtl8139.c`)

當封包進來時，RTL8139 會把它塞進我們昨天分配的 `rx_buffer`，然後發出中斷。我們要從 Buffer 中把封包拿出來，讀取開頭的 4 bytes 硬體狀態，接著就是真正的 Ethernet 封包了！

打開 **`src/kernel/lib/rtl8139.c`**，引入 `ethernet.h`，並在檔案最下方加入這個處理常式：

```c
#include "ethernet.h" // 【Day 84 新增】

// 用來追蹤目前讀取到緩衝區的哪個位置
static uint16_t rx_offset = 0; 

// ==========================================
// 【Day 84 新增】RTL8139 中斷處理常式
// ==========================================
void rtl8139_handler(void) {
    // 1. 讀取中斷狀態暫存器 (ISR, Offset 0x3E)
    uint16_t status = inw(rtl_iobase + 0x3E);

    if (!status) return;

    // 2. 寫回 ISR 來清除中斷旗標 (寫入 1 代表清除)
    outw(rtl_iobase + 0x3E, status);

    // 3. 檢查是否為 "Receive OK" (Bit 0)
    if (status & 0x01) {
        
        // 不斷讀取，直到硬體告訴我們 Buffer 空了 (CR 暫存器的 Bit 0 為 1 代表空)
        while ((inb(rtl_iobase + 0x37) & 0x01) == 0) {
            
            // RTL8139 封包的開頭會有 4 個 bytes 的硬體檔頭：
            // [ 16-bit Rx Status ] [ 16-bit Packet Length ]
            uint16_t* rx_header = (uint16_t*)(rx_buffer + rx_offset);
            uint16_t rx_length = rx_header[1];

            // 實際的網路封包位址 (跳過那 4 bytes 硬體檔頭)
            uint8_t* packet_data = rx_buffer + rx_offset + 4;

            // 將封包轉型為乙太網路標頭！
            ethernet_header_t* eth = (ethernet_header_t*)packet_data;

            kprintf("[Net] Packet RX! Len: %d bytes\n", rx_length);
            kprintf("  -> Dest MAC: %x:%x:%x:%x:%x:%x\n", 
                    eth->dest_mac[0], eth->dest_mac[1], eth->dest_mac[2],
                    eth->dest_mac[3], eth->dest_mac[4], eth->dest_mac[5]);
            
            uint16_t type = ntohs(eth->ethertype);
            if (type == ETHERTYPE_ARP) kprintf("  -> Protocol: ARP\n");
            else if (type == ETHERTYPE_IPv4) kprintf("  -> Protocol: IPv4\n");
            else kprintf("  -> Protocol: Unknown (0x%x)\n", type);

            // 4. 更新 Rx_offset
            // 長度包含 4 bytes CRC，且必須對齊 4 bytes 邊界
            rx_offset = (rx_offset + rx_length + 4 + 3) & ~3;

            // 若超過 8K (8192) 邊界，會繞回開頭 (Wrap around)
            if (rx_offset >= 8192) {
                rx_offset -= 8192;
            }

            // 5. 告訴網卡我們讀到哪裡了 (寫入 CAPR 暫存器 0x38)
            // (註：硬體設計怪癖，必須減掉 16)
            outw(rtl_iobase + 0x38, rx_offset - 16);
        }
    }

    // 發送 EOI 給中斷控制器 (PIC)
    // 因為 IRQ 11 接在 Slave PIC 上，所以要發給 Master & Slave
    outb(0xA0, 0x20); 
    outb(0x20, 0x20); 
}
```

---

### 步驟 3：接通 IRQ 11 到中斷表 (IDT)

你昨天的截圖顯示網卡被分配到了 **IRQ 11**。在 PIC 控制器中，IRQ 8~15 對應到我們的中斷向量 `INT 40 ~ 47`。所以 IRQ 11 剛好就是 **INT 43** (32 + 11 = 43)！

**1. 打開 `src/kernel/asm/interrupts.S`，加入 `isr43`：**
在最下方加入這段組語：
```assembly
global isr43
extern rtl8139_handler

isr43:
    pusha
    call rtl8139_handler
    popa
    iret
```

**2. 打開 `src/kernel/lib/idt.c`，註冊中斷閘：**
在 `init_idt` 函式裡面，將 43 號中斷註冊進去：
```c
extern void isr43(void);

// ... 在 init_idt() 裡面補上這行 ...
idt_set_gate(43, (uint32_t)isr43, 0x08, 0x8E);
```

---

### 🚀 驗收：見證第一個封包的降臨！

存檔編譯並執行 `make clean && make run`！

當系統開機並進入 Terminal 畫面後，**你什麼事都不用做，只要靜靜等待幾秒鐘！**
QEMU 的使用者模式網路 (SLIRP) 在背景運作時，會自動發送一些 DHCP 或是 ARP 的廣播封包到網路上。
不用多久，你的 Terminal 就會突然跳出：

```text
[Net] Packet RX! Len: 64 bytes
  -> Dest MAC: ff:ff:ff:ff:ff:ff
  -> Protocol: ARP
```

看到這段訊息的瞬間，代表你的作業系統已經成功捕獲了網路線上的電子訊號，並將它還原成了有意義的邏輯資料。這是 OS 與真實世界連線的最關鍵第一步！準備好迎接封包雨吧！
