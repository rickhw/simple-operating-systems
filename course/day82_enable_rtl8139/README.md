太神啦，Rick！🎉 看到那句 `[PCI] Found Network Card: Realtek RTL8139` 完美地印在你的畫面上，這代表我們成功跨越了硬體與軟體的第一道鴻溝！你修復開機順序的邏輯非常精準，現在 Kernel 已經確實掌握了網卡的存在。

既然 PCI 點名成功，**Day 82：喚醒 RTL8139 網卡！(讀取 MAC 位址)**，今天我們要正式開始寫網卡驅動程式 (NIC Driver)。

要喚醒一張網卡，我們需要做四件事：
1. **取得 I/O Base Address**：問 PCI 網卡的控制暫存器在哪個位址。
2. **開機 (Power On)**：網卡預設可能是休眠狀態，我們要把它叫醒。
3. **軟體重置 (Software Reset)**：將網卡狀態清空，準備接收新指令。
4. **讀取 MAC 位址**：把這張網卡出廠時烙印在硬體上的「全球唯一身分證」讀出來！

請跟著我進行這 3 個步驟：

---

### 步驟 1：定義 RTL8139 標頭檔 (`src/kernel/include/rtl8139.h`)

建立 **`src/kernel/include/rtl8139.h`**，宣告我們的初始化函式：

```c
#ifndef RTL8139_H
#define RTL8139_H

#include <stdint.h>

void init_rtl8139(uint8_t bus, uint8_t slot);

#endif
```

---

### 步驟 2：實作網卡驅動核心 (`src/kernel/lib/rtl8139.c`)

這支檔案將會透過 I/O Port (`inb` / `outb`) 來直接與網卡的硬體暫存器溝通。

建立 **`src/kernel/lib/rtl8139.c`**：

```c
#include "rtl8139.h"
#include "pci.h"
#include "tty.h"

// 宣告外部的 I/O 指令 (借用你系統中已有的)
extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t data);

// 儲存網卡的 I/O 基準位址與 MAC 位址
static uint32_t rtl_iobase = 0;
static uint8_t mac_addr[6];

void init_rtl8139(uint8_t bus, uint8_t slot) {
    kprintf("[RTL8139] Initializing driver...\n");

    // 1. 取得 I/O Base Address
    // 網卡的 Base Address Register 0 (BAR0) 存在 PCI 設定空間的 Offset 0x10
    // 因為 BAR0 是 32-bit，我們用兩次 16-bit 讀取來組合它
    uint32_t bar0_low = pci_read_config_word(bus, slot, 0, 0x10);
    uint32_t bar0_high = pci_read_config_word(bus, slot, 0, 0x12);
    
    // 組合出 32-bit 的位址，並清除最低 2 個 bit (PCI 規範，用來標示這是一塊 I/O 空間)
    rtl_iobase = (bar0_low | (bar0_high << 16)) & ~3;
    kprintf("[RTL8139] I/O Base Address found at: 0x%x\n", rtl_iobase);

    // 2. 開機 (Power On)
    // RTL8139 的 CONFIG1 暫存器 (Offset 0x52)，寫入 0x00 即可喚醒
    outb(rtl_iobase + 0x52, 0x00);

    // 3. 軟體重置 (Software Reset)
    // 寫入 0x10 到 Command 暫存器 (Offset 0x37) 觸發重置
    outb(rtl_iobase + 0x37, 0x10);
    
    // 不斷讀取，直到 0x10 這個 bit 被硬體清空，代表重置完成
    while ((inb(rtl_iobase + 0x37) & 0x10) != 0) {
        // 等待硬體重置...
    }
    kprintf("[RTL8139] Device reset successful.\n");

    // 4. 讀取 MAC 位址
    // RTL8139 的 MAC 位址直接存在 I/O Base 的最前面 6 個 Byte (Offset 0x00 ~ 0x05)
    for (int i = 0; i < 6; i++) {
        mac_addr[i] = inb(rtl_iobase + i);
    }

    kprintf("[RTL8139] MAC Address: %x:%x:%x:%x:%x:%x\n", 
            mac_addr[0], mac_addr[1], mac_addr[2], 
            mac_addr[3], mac_addr[4], mac_addr[5]);
}
```
*(💡 記得在 `src/kernel/Makefile` 中把 `rtl8139.o` 加入編譯清單！)*

---

### 步驟 3：在 PCI 掃描時呼叫網卡驅動 (`src/kernel/lib/pci.c`)

最後，我們回到昨天寫的 `pci.c`，當它發現 RTL8139 時，我們就立刻把這張網卡的 `bus` 和 `slot` 交給驅動程式去啟動！

打開 **`src/kernel/lib/pci.c`**：

```c
#include "pci.h"
#include "tty.h" 
#include "rtl8139.h" // 【Day 82 新增】引入網卡驅動

// ... (前面的 outl, inl, pci_read_config_word 保持不變) ...

void init_pci(void) {
    kprintf("[PCI] Scanning PCI Bus...\n");
    int device_found = 0;

    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            uint16_t vendor = pci_read_config_word(bus, slot, 0, 0);
            
            if (vendor != 0xFFFF) {
                uint16_t device = pci_read_config_word(bus, slot, 0, 2);
                device_found++;

                // 特別攔截我們的目標：網路卡！
                if (vendor == RTL8139_VENDOR_ID && device == RTL8139_DEVICE_ID) {
                    kprintf("[PCI] Found Network Card: Realtek RTL8139 (Bus %d, Slot %d)\n", bus, slot);
                    
                    // ==========================================
                    // 【Day 82 新增】呼叫驅動程式喚醒網卡！
                    // ==========================================
                    init_rtl8139(bus, slot);
                    
                } else if (vendor == E1000_VENDOR_ID && device == E1000_DEVICE_ID) {
                    kprintf("[PCI] Found Network Card: Intel E1000 (Bus %d, Slot %d)\n", bus, slot);
                }
            }
        }
    }
    kprintf("[PCI] Scan complete. Found %d devices.\n", device_found);
}
```

---

### 🚀 驗收：見證 MAC 位址的誕生

存檔、編譯並執行 `make clean && make run`！

在進入 CLI 畫面時，仔細看剛才的 `[PCI]` 掃描訊息下方，將會連續印出我們新寫的 RTL8139 驅動 Log：
它會找出 I/O Base 位址，顯示重置成功，並且**印出由 QEMU 分配給這張虛擬網卡的 6 個 Byte MAC 位址（例如：`52:54:0:12:34:56`）！**

只要能成功讀出正確的 MAC 位址（不是全 `00` 也不是全 `ff`），就代表你的 Kernel 已經具備與這張網卡對話的能力了。截圖分享一下你的成果吧！
