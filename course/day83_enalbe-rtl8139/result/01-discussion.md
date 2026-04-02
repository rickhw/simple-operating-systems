
遇到一個讓我很困惑的問題，目前 utils.c, io.h, pci.c, rtl8139.c 裡有幾個長得很像的 function (整理在後面)，有些是 32bit, 有些則是 16bit ... 

比對過目前 outw 跟 utils.c 裡的長得不太一樣。

我想:

1. 先讓 rtl8139.c 可以正常編譯、符合預期
2. 把這些長得很像的 function 整理放到 io.h，但不影想期他 code


```bash
==> 編譯 Kernel Lib: src/kernel/lib/pci.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems:/workspace -w /workspace/src os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ikernel/include -Ikernel/lib -c kernel/lib/pci.c -o kernel/lib/pci.o
kernel/lib/pci.c: In function 'init_pci':
kernel/lib/pci.c:40:5: warning: implicit declaration of function 'kprintf' [-Wimplicit-function-declaration]
   40 |     kprintf("[PCI] Scanning PCI Bus...\n");
      |     ^~~~~~~
==> 編譯 Kernel Lib: src/kernel/lib/pmm.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems:/workspace -w /workspace/src os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ikernel/include -Ikernel/lib -c kernel/lib/pmm.c -o kernel/lib/pmm.o
==> 編譯 Kernel Lib: src/kernel/lib/rtc.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems:/workspace -w /workspace/src os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ikernel/include -Ikernel/lib -c kernel/lib/rtc.c -o kernel/lib/rtc.o
==> 編譯 Kernel Lib: src/kernel/lib/rtl8139.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems:/workspace -w /workspace/src os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ikernel/include -Ikernel/lib -c kernel/lib/rtl8139.c -o kernel/lib/rtl8139.o
kernel/lib/rtl8139.c:12:20: error: static declaration of 'outw' follows non-static declaration
   12 | static inline void outw(uint16_t port, uint16_t val) {
      |                    ^~~~
In file included from kernel/lib/rtl8139.c:5:
kernel/include/utils.h:31:6: note: previous declaration of 'outw' was here
   31 | void outw(uint16_t port, uint16_t data);    // output write, @TODO: move to io.h
      |      ^~~~
kernel/lib/rtl8139.c: In function 'init_rtl8139':
kernel/lib/rtl8139.c:71:5: warning: implicit declaration of function 'outl'; did you mean 'outw'? [-Wimplicit-function-declaration]
   71 |     outl(rtl_iobase + 0x30, (uint32_t)rx_buffer);
      |     ^~~~
      |     outw
make: *** [src/kernel/lib/rtl8139.o] Error 1
rm src/user/lib/unistd.o src/user/lib/syscall.o src/user/lib/simpleui.o src/user/lib/stdio.o src/user/asm/crt0.o src/user/lib/stdlib.o
```


rtl8139.c

```c
// 【Day 83 新增】16-bit I/O 寫入
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}
```


pci.c
```c
// 32-bit I/O 寫入
static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

// 32-bit I/O 讀取
static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
```

io.h
```c
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
```

---
utils.c
```c
// in write, @TODO: move to io.h
uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

// output write, @TODO: move to io.h
void outw(uint16_t port, uint16_t data) {
    __asm__ volatile ("outw %1, %0" : : "dN" (port), "a" (data));
}
```
