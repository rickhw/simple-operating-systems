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
void pci_write_config_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t data);

#endif
