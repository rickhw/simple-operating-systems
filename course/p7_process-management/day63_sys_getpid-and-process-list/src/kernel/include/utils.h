#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>

// === Memory Utils ===
void* memcpy(void* dstptr, const void* srcptr, size_t size);
void* memset(void* bufptr, int value, size_t size);

// === String Utils ===
void reverse_string(char* str, int length);
void reverse_string(char* str, int length);
int strcmp(const char *s1, const char *s2);
char *strcpy(char *dest, const char *src);
int strlen(const char* str);
// 尋找子字串 (如果找到 needle，回傳它在 haystack 中的指標，否則回傳 0)
char* strstr(const char* haystack, const char* needle);

// 核心工具：整數轉字串 (itoa), value: 要轉換的數字, str: 存放結果的陣列, base: 進位制 (10或16)
void itoa(int value, char* str, int base);

// === IO Utils ===
void kprintf(const char* format, ...);

uint16_t inw(uint16_t port);                // in write, @TODO: move to io.h
void outw(uint16_t port, uint16_t data);    // output write, @TODO: move to io.h

#endif
