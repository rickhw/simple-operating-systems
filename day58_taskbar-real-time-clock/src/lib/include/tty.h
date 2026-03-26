#ifndef TTY_H
#define TTY_H

#include <stdint.h>
#include <stddef.h>

void terminal_initialize(void);
void terminal_putchar(char c);
void terminal_write(const char* data, size_t size);
void terminal_writestring(const char* data);
//void tty_render(void);

// 刪掉 void tty_render(void);
void terminal_bind_window(int win_id);
void tty_render_window(int win_id);
#endif
