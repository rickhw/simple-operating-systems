太棒了，Rick！🔥 歡迎來到作業系統開發中最令人興奮、也最具挑戰性的領域：**網路 (Networking)**！



在我們能送出任何一個網路封包之前，Kernel 必須先找到「網卡」在哪裡。現代主機板上的裝置（顯示卡、音效卡、網卡等）都是插在 **PCI 匯流排 (PCI Bus)** 上的。
PCI 提供了一套標準的「點名機制」，作業系統只要對著特定的 I/O Port (`0xCF8` 位址埠 與 `0xCFC` 資料埠) 大喊：「Bus 0, Slot 3 的同學請報上名來！」，裝置就會回傳它的 **Vendor ID (製造商編號)** 和 **Device ID (裝置型號)**。

**Day 81：PCI 匯流排掃描**，今天我們要寫一個 PCI Scanner，把 QEMU 肚子裡的網卡給揪出來！

請跟著我進行這 4 個步驟：

---

### 步驟 1：定義 PCI 標頭檔 (`src/kernel/include/pci.h`)

我們要宣告 PCI 掃描的函式。

建立 **`src/kernel/include/pci.h`**：
```c
#ifndef PCI_H
#define PCI_H

#include <stdint.h>

// PCI Configuration Space 的標準 I/O Ports
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

// 常見的網卡硬體 ID
#define RTL8139_VENDOR_ID  0x10EC
#define RTL8139_DEVICE_ID  0x8139
#define E1000_VENDOR_ID    0x8086
#define E1000_DEVICE_ID    0x100E

void init_pci(void);
uint32_t pci_read_config_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

#endif
```

---

### 步驟 2：實作 PCI 驅動程式 (`src/kernel/lib/pci.c`)

因為 PCI 的暫存器是 32-bit 的，我們需要先準備 `outl` 和 `inl` 這兩個 32-bit I/O 指令。然後透過雙層 `for` 迴圈，把 Bus (0~255) 和 Slot (0~31) 掃過一遍。

建立 **`src/kernel/lib/pci.c`**：
```c
#include "pci.h"
#include "tty.h" // 為了使用 kprintf

// 32-bit I/O 寫入
static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

// 32-bit I/O 讀取
static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// 讀取 PCI 裝置設定空間中的 16-bit Word
uint32_t pci_read_config_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
    uint16_t tmp = 0;

    // 依照 PCI 規範組合 Address (Bit 31 必須為 1 代表 Enable)
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));

    // 將位址寫入 Address Port
    outl(PCI_CONFIG_ADDRESS, address);
    
    // 從 Data Port 讀取資料
    // (因為一次讀取 32-bit，我們透過位移與 Mask 取得正確的 16-bit 段落)
    tmp = (uint16_t)((inl(PCI_CONFIG_DATA) >> ((offset & 2) * 8)) & 0xFFFF);
    return tmp;
}

// 掃描整條 PCI 匯流排
void init_pci(void) {
    kprintf("[PCI] Scanning PCI Bus...\n");

    int device_found = 0;

    // 遍歷所有的 Bus (0~255)
    for (uint16_t bus = 0; bus < 256; bus++) {
        // 遍歷該 Bus 上所有的 Slot (0~31)
        for (uint8_t slot = 0; slot < 32; slot++) {
            
            // Offset 0 是 Vendor ID
            uint16_t vendor = pci_read_config_word(bus, slot, 0, 0);
            
            // 如果 Vendor ID 是 0xFFFF，代表這個位址沒有插任何裝置
            if (vendor != 0xFFFF) {
                // Offset 2 是 Device ID
                uint16_t device = pci_read_config_word(bus, slot, 0, 2);
                
                device_found++;
                
                // 我們可以把所有找到的硬體都印出來看看
                // kprintf("  -> Bus %d, Slot %d | Vendor: 0x%X, Device: 0x%X\n", bus, slot, vendor, device);

                // 特別攔截我們的目標：網路卡！
                if (vendor == RTL8139_VENDOR_ID && device == RTL8139_DEVICE_ID) {
                    kprintf("[PCI] Found Network Card: Realtek RTL8139 (Bus %d, Slot %d)\n", bus, slot);
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

### 步驟 3：在開機時啟動掃描 (`src/kernel/kernel.c`)

我們要讓 Kernel 在開機的早期（進入 GUI 或 Shell 之前）進行硬體點名。

打開 **`src/kernel/kernel.c`**，引入標頭檔，並在 `kernel_main` 裡呼叫 `init_pci()`：
```c
// ... 其他 include ...
#include "pci.h"  // 【Day 81 新增】

void kernel_main(uint32_t magic, multiboot_info_t* mbi) {
    // ... 前面的 init_gdt, init_idt, init_timer, init_memory 保持不變 ...
    
    // 【Day 81 新增】在啟動檔案系統或 GUI 之前，先掃描硬體！
    init_pci();
    
    // ... 後面的 setup_filesystem, init_gui 等保持不變 ...
```

*(💡 別忘了在 `src/kernel/Makefile` 中把 `pci.o` 加入編譯清單喔！)*

---

### 步驟 4：為 QEMU 裝上網路卡 (`Makefile`)

QEMU 預設不一定會掛載我們想要的特定網卡。為了確保接下來的網路課程順利，我們要在啟動腳本裡「強迫」QEMU 裝上一張 **RTL8139** 網卡 (這是 OS 開發界最經典、驅動最好寫的網卡)。

打開你的 **`Makefile`** (最外層用來執行 QEMU 的那個)，找到 `run:` 區塊，修改 QEMU 啟動參數，加入 `-netdev` 和 `-device`：

```makefile
run: kernel.bin
    qemu-system-i386 -kernel kernel.bin \
        -netdev user,id=n0 \
        -device rtl8139,netdev=n0 \
        # ... 其他你原本的參數 (如 -m 16M 等) ...
```

---

### 🚀 驗收：見證硬體甦醒的時刻！

執行 `make clean && make run`。
這一次，在進入熟悉的 Simple OS 終端機之前，請仔細看開機的那一串 `[Kernel]` Log。

你將會看到系統成功呼叫 PCI 匯流排，並且在畫面中印出：
**`[PCI] Found Network Card: Realtek RTL8139 (Bus 0, Slot 3)`**

這代表你的作業系統已經具備了「發現並識別外部硬體」的能力了！這張被揪出來的網卡，就是我們接下來征服網際網路的船艦。編譯執行後，看看它是不是乖乖現形了！
