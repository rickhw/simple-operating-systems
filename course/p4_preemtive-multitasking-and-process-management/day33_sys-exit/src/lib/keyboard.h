#ifndef KEYBOARD_H
#define KEYBOARD_H
#include <stdint.h>

void keyboard_handler(void);
char keyboard_getchar(); // [新增] 讓 Syscall 可以來拿字元

#endif
