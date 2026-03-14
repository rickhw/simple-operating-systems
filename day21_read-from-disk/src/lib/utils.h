#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>

void* memcpy(void* dstptr, const void* srcptr, size_t size);
void* memset(void* bufptr, int value, size_t size);
// void* memset(void* dest, int val, uint32_t len);
void reverse_string(char* str, int length);
void reverse_string(char* str, int length);
void itoa(int value, char* str, int base);
void kprintf(const char* format, ...);

// [Day21] add -- start
uint16_t inw(uint16_t port);
void outw(uint16_t port, uint16_t data);
// [Day21] add -- end

#endif
