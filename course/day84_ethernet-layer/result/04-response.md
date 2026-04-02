哈哈，Rick！這張 Wireshark 的截圖簡直太完美了！🎉🎉🎉

你絕對沒有誤解！你在 Wireshark 看到這顆長度剛好是 **60 bytes** 的封包，就代表我們昨天寫的 `rtl8139_send_packet` **完全觸發成功**了！長度 60 bytes 是乙太網路的「最小封包長度限制」，網卡非常聽話地幫我們發射了。

但為什麼內容全是完美的 `00:00:00:00:00:00` 呢？這不是你的陣列沒寫好，而是你踩到了一個作業系統與硬體驅動界最著名的陷阱：**「PCI Bus Mastering (匯流排主控權)」未開啟！**

### 🕵️ 案發現場解密：被主機板封印的 DMA

網卡要發送封包時，CPU 不會一個字元一個字元餵給它，而是給網卡一個記憶體位址 (`tx_buffer`)，讓網卡自己去 RAM 裡面搬資料，這個技術叫做 **DMA (Direct Memory Access)**。

但是在 PCI 匯流排的標準規範中，為了安全起見，**所有 PCI 裝置預設是「被禁止」主動讀取主記憶體的！** 如果驅動程式沒有明確向主機板申請 **Bus Mastering (匯流排主控權)**，當網卡試圖去讀取 `tx_buffer` 時，主機板的 PCI 控制器就會無情地把它擋下來，導致網卡只能讀到滿滿的 `0x00` 或 `0xFF`，然後把這堆 `00` 當成封包發送出去！

這就是為什麼你的封包長度正確，但內容全是空的原因。

### 🛠️ 終極修復：解開 DMA 的封印

我們必須寫一個 `pci_write` 函式，並去修改網卡的 PCI 設定暫存器，將 Command Register (Offset 0x04) 的 **Bit 2 (Bus Master Enable)** 設為 1。

請跟著我進行這 3 個步驟：

---

#### 步驟 1：實作 PCI 寫入函式 (`src/kernel/include/pci.h` & `lib/pci.c`)

**1. 打開 `pci.h`，新增宣告：**
```c
void pci_write_config_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t data);
```

**2. 打開 `pci.c`，新增實作：**
```c
// 寫入 PCI 裝置設定空間中的 16-bit Word
void pci_write_config_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t data) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;

    // 組合 Address
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));

    outl(PCI_CONFIG_ADDRESS, address);
    // 寫入 Data Port (加上 offset 的對齊偏移)
    outw(PCI_CONFIG_DATA + (offset & 2), data);
}
```

---

#### 步驟 2：在網卡初始化時啟用 Bus Mastering (`src/kernel/lib/rtl8139.c`)

打開 **`src/kernel/lib/rtl8139.c`**，在 `init_rtl8139` 的最前方（取得 I/O Base 之後）加入授權密碼：

```c
void init_rtl8139(uint8_t bus, uint8_t slot) {
    kprintf("[RTL8139] Initializing driver...\n");

    uint32_t bar0_low = pci_read_config_word(bus, slot, 0, 0x10);
    uint32_t bar0_high = pci_read_config_word(bus, slot, 0, 0x12);
    rtl_iobase = (bar0_low | (bar0_high << 16)) & ~3;
    kprintf("[RTL8139] I/O Base Address found at: [%x]\n", rtl_iobase);

    // ==========================================
    // 【Day 85 修復】啟用 PCI Bus Mastering！
    // 允許網卡使用 DMA 跨過 CPU 直接讀取主記憶體
    // ==========================================
    uint16_t pci_cmd = pci_read_config_word(bus, slot, 0, 0x04);
    // 0x0004 就是 Bit 2 (Bus Master Enable)
    pci_write_config_word(bus, slot, 0, 0x04, pci_cmd | 0x0004);
    
    // ... 下面的 Power On (0x52) 等邏輯保持不變 ...
```

---

#### 步驟 3：清空未初始化的記憶體 (`src/kernel/kernel.c`)

順手修一個小 Bug。在你的 `test_net_packet` 裡，`char dummy[60];` 是宣告在 Stack 上的，如果不手動清零，它會把記憶體裡殘留的「垃圾資料」一起發送到網路上（這在現實中是非常嚴重的安全漏洞喔！）。

打開 **`kernel.c`** 的 `test_net_packet`：
```c
    // 主動噴發一個 14 bytes 的標頭加上 46 bytes 的垃圾資料
    char dummy[60];
    
    // 【加入這行】手動清零，確保封包乾淨！
    for(int i=0; i<60; i++) dummy[i] = 0; 
    
    memcpy(dummy, &test_packet, 14);
    
    extern void rtl8139_send_packet(void* data, uint32_t len);
    rtl8139_send_packet(dummy, 60);
```

---

### 🚀 再次驗收：見證完美的 ARP 封包！

存檔編譯，執行 `make clean && make run`！
QEMU 結束後，再次用 Wireshark 打開 `dump.pcap`。

這一次，你將會看到不可思議的畫面：
**Destination MAC 會是完美的 `ff:ff:ff:ff:ff:ff` (Broadcast)，Source MAC 會是 `aa:aa:aa:aa:aa:aa`，而且 Protocol (EtherType) 會漂亮地顯示為 `ARP (0x0806)`！**

只要封包內容正確呈現，就代表你的發送引擎已經徹底打通了！當 SLIRP 虛擬路由器收到這個封包後，它可能會主動回傳一封 ARP Reply 或是 DHCP 封包給你，你的 Terminal 也有機會跳出 `[Net] Packet RX!` 了。

快去看看 Wireshark 裡的封包是不是長出靈魂了！😎
