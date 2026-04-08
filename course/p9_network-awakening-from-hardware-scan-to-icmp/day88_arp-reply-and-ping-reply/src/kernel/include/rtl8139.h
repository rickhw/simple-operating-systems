#ifndef RTL8139_H
#define RTL8139_H

#include <stdint.h>

void init_rtl8139(uint8_t bus, uint8_t slot);
void rtl8139_handler(void);

// [Day 85 新增]
void rtl8139_send_packet(void* data, uint32_t len);
uint8_t* rtl8139_get_mac(void);

#endif
