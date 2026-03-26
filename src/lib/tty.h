#ifndef TTY_H
#define TTY_H

#include <stdint.h>
#include <stddef.h>

void update_cursor(int x, int y);
void terminal_putentryat(char c, uint8_t color, size_t x, size_t y);
void terminal_scroll();
void terminal_putchar(char c);
void terminal_writestring(const char* data);
void terminal_initialize(void);

#endif
