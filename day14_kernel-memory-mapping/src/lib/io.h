// io.h
#ifndef IO_H
#define IO_H

#include <stdint.h>

// 向指定的硬體 Port 寫入 1 byte 的資料
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

// 從指定的硬體 Port 讀取 1 byte 的資料
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

#endif
