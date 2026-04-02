/**
 * @file src/kernel/drivers/pci/pci.c
 * @brief Main logic and program flow for pci.c.
 *
 * This file handles the operations and logic associated with pci.c.
 */

#include "pci.h"
#include "io.h"
#include "utils.h"
#include "rtl8139.h"


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
                    kprintf("[PCI] Found Network Card: Realtek RTL8139 (Bus [%d], Slot [%d])\n", bus, slot);
                    init_rtl8139(bus, slot);    // 呼叫驅動程式喚醒網卡！
                } else if (vendor == E1000_VENDOR_ID && device == E1000_DEVICE_ID) {
                    kprintf("[PCI] Found Network Card: Intel E1000 (Bus [%d], Slot [%d])\n", bus, slot);
                }
            }
        }
    }
    kprintf("[PCI] Scan complete. Found [%d] devices.\n", device_found);
}


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
