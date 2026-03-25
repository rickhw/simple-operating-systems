
看起來 kprinf 有重複宣告了，在 util.c 裡也有一個 kprintf，我們應該用哪一個？

還是把 kprintf 搬到 util.c ？

```bash
==> 編譯 Kernel Lib: lib/vfs.c
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day52_bitmap-font-rendering/src:/workspace -w /workspace os-builder gcc -m32 -ffreestanding -O2 -Wall -Wextra -Ilib/include -c lib/vfs.c -o lib/vfs.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day52_bitmap-font-rendering/src:/workspace -w /workspace os-builder ld -m elf_i386 -n -T linker.ld -o myos.bin asm/boot.o asm/gdt_flush.o asm/interrupts.o asm/paging.o asm/switch_task.o asm/user_mode.o lib/ata.o lib/elf.o lib/gdt.o lib/gfx.o lib/idt.o lib/kernel.o lib/keyboard.o lib/kheap.o lib/mbr.o lib/paging.o lib/pmm.o lib/simplefs.o lib/syscall.o lib/task.o lib/timer.o lib/tty.o lib/utils.o lib/vfs.o
ld: lib/utils.o: in function `kprintf':
utils.c:(.text+0x230): multiple definition of `kprintf'; lib/tty.o:tty.c:(.text+0x220): first defined here
make: *** [myos.bin] Error 1
rm libc/stdio.o libc/unistd.o libc/syscall.o mkdir.o rm.o shell.o touch.o echo.o libc/stdlib.o cat.o pong.o ping.o ls.o
docker run --platform linux/amd64 --rm -v /Users/rickhwang/Repos/ai-lab/simple-operating-systems/day52_bitmap-font-rendering/src:/workspace -w /workspace os-builder ld -m elf_i386 -n -T linker.ld -o myos.bin asm/boot.o asm/gdt_flush.o asm/interrupts.o asm/paging.o asm/switch_task.o asm/user_mode.o lib/ata.o lib/elf.o lib/gdt.o lib/gfx.o lib/idt.o lib/kernel.o lib/keyboard.o lib/kheap.o lib/mbr.o lib/paging.o lib/pmm.o lib/simplefs.o lib/syscall.o lib/task.o lib/timer.o lib/tty.o lib/utils.o lib/vfs.o
ld: lib/utils.o: in function `kprintf':
utils.c:(.text+0x230): multiple definition of `kprintf'; lib/tty.o:tty.c:(.text+0x220): first defined here
make: *** [myos.bin] Error 1
```

---
util.c
```c
#include "tty.h"

#include <stdarg.h>
#include <stdbool.h>

// === Memory Utils ===

// 記憶體複製 (將 src 複製 size 個 bytes 到 dst)
void* memcpy(void* dstptr, const void* srcptr, size_t size) {
    unsigned char* dst = (unsigned char*) dstptr;
    const unsigned char* src = (const unsigned char*) srcptr;
    for (size_t i = 0; i < size; i++) {
        dst[i] = src[i];
    }
    return dstptr;
}

// 記憶體填充 (將 buf 的前 size 個 bytes 填入單一數值 value)
void* memset(void* bufptr, int value, size_t size) {
    unsigned char* buf = (unsigned char*) bufptr;
    for (size_t i = 0; i < size; i++) {
        buf[i] = (unsigned char) value;
    }
    return bufptr;
}

// === StringUilts ===
// 輔助函式：反轉字串
void reverse_string(char* str, int length) {
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char *strcpy(char *dest, const char *src) {
    char *saved = dest;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return saved;
}

// 核心工具：整數轉字串 (itoa)
// value: 要轉換的數字, str: 存放結果的陣列, base: 進位制 (10或16)
void itoa(int value, char* str, int base) {
    int i = 0;
    bool is_negative = false;
    unsigned int uvalue; // [新增] 用來做運算的無號整數

    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    // 處理負數 (僅限十進位)
    if (value < 0 && base == 10) {
        is_negative = true;
        uvalue = (unsigned int)(-value);
    } else {
        // [關鍵] 如果是16進位，直接把位元當作無號整數看待 (完美對應記憶體的原始狀態)
        uvalue = (unsigned int)value;
    }

    // 逐位取出餘數 (改用 uvalue)
    while (uvalue != 0) {
        int rem = uvalue % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'A' : rem + '0';
        uvalue = uvalue / base;
    }

    if (is_negative) {
        str[i++] = '-';
    }

    str[i] = '\0';
    reverse_string(str, i);
}

int strlen(const char* str) {
    int len = 0;
    while (str[len] != '\0') len++;
    return len;
}


// === IO Utils ===

// Kernel 專屬格式化輸出 (kprintf)
void kprintf(const char* format, ...) {
    va_list args;
    va_start(args, format); // 初始化不定參數列表

    for (size_t i = 0; format[i] != '\0'; i++) {
        // 如果不是 '%'，就當作一般字元直接印出
        if (format[i] != '%') {
            terminal_putchar(format[i]);
            continue;
        }

        // 遇到 '%'，看下一個字元是什麼來決定格式
        i++;
        switch (format[i]) {
            case 'd': { // 十進位整數
                int num = va_arg(args, int);
                char buffer[32];
                itoa(num, buffer, 10);
                terminal_writestring(buffer);
                break;
            }
            case 'x': { // 十六進位整數
                // [關鍵] 改用 unsigned int 取出參數
                unsigned int num = va_arg(args, unsigned int);
                char buffer[32];
                terminal_writestring("0x");
                itoa(num, buffer, 16);
                terminal_writestring(buffer);
                break;
            }
            case 's': { // 字串
                char* str = va_arg(args, char*);
                terminal_writestring(str);
                break;
            }
            case 'c': { // 單一字元
                // char 在變數傳遞時會被提升為 int
                char c = (char) va_arg(args, int);
                terminal_putchar(c);
                break;
            }
            case '%': { // 印出 '%' 本身
                terminal_putchar('%');
                break;
            }
            default: // 未知的格式，原樣印出
                terminal_putchar('%');
                terminal_putchar(format[i]);
                break;
        }
    }

    va_end(args); // 清理不定參數列表
}


uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

void outw(uint16_t port, uint16_t data) {
    __asm__ volatile ("outw %1, %0" : : "dN" (port), "a" (data));
}
```
