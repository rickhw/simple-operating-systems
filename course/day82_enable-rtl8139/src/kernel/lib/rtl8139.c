#include "io.h"
#include "rtl8139.h"
#include "pci.h"
#include "tty.h"
#include "utils.h"

// // 宣告外部的 I/O 指令 (借用你系統中已有的)
// extern uint8_t inb(uint16_t port);
// extern void outb(uint16_t port, uint8_t data);

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
    kprintf("[RTL8139] I/O Base Address found at: [%x]\n", rtl_iobase);

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

    kprintf("[RTL8139] MAC Address: [%x:%x:%x:%x:%x:%x]\n",
            mac_addr[0], mac_addr[1], mac_addr[2],
            mac_addr[3], mac_addr[4], mac_addr[5]);
}
