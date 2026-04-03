#ifndef _LIBC_SYSCALL_H
#define _LIBC_SYSCALL_H

#include <stdint.h>

#define SYS_NET_PING 31

int syscall(int num, int p1, int p2, int p3, int p4, int p5);

// 宣告給 user app 使用的 API
void sys_ping(uint8_t* ip);

#endif
