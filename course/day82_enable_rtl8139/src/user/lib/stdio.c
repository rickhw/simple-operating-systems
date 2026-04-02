#include "stdio.h"
#include "syscall.h"
#include "stdarg.h"

// Syscall 2: sys_print
void print(const char* str) {
    syscall(2, (int)str, 0, 0, 0, 0);
}

// Syscall 5: sys_getchar
char getchar() {
    return (char)syscall(5, 0, 0, 0, 0, 0);
}

// 內部工具：將整數轉為字串並印出 (支援 10 進位與 16 進位)
static void print_int(int num, int base) {
    char buf[32];
    int i = 0;
    int is_neg = 0;

    if (num == 0) {
        print("0");
        return;
    }

    if (num < 0 && base == 10) {
        is_neg = 1;
        num = -num; // 轉為正數處理
    }

    while (num != 0) {
        int rem = num % base;
        buf[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num /= base;
    }

    if (is_neg) buf[i++] = '-';

    // 陣列是反的，把它倒過來印出
    char str[32];
    int j = 0;
    while (i > 0) {
        str[j++] = buf[--i];
    }
    str[j] = '\0';
    print(str);
}

void printf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    for (int i = 0; format[i] != '\0'; i++) {
        if (format[i] == '%') {
            i++; // 跳到下一個字元看格式
            switch (format[i]) {
                case 'd': { // 十進位整數
                    int num = va_arg(args, int);
                    print_int(num, 10);
                    break;
                }
                case 'x': { // 十六進位整數
                    int num = va_arg(args, int);
                    print("0x");
                    print_int(num, 16);
                    break;
                }
                case 's': { // 字串
                    char* str = va_arg(args, char*);
                    if (str == 0) str = "(null)";
                    print(str);
                    break;
                }
                case 'c': { // 單一字元
                    char c = (char)va_arg(args, int);
                    char str[2] = {c, '\0'};
                    print(str);
                    break;
                }
                case '%': { // 印出 % 本身
                    print("%");
                    break;
                }
                default: { // 不支援的格式，原樣印出
                    char str[3] = {'%', format[i], '\0'};
                    print(str);
                    break;
                }
            }
        } else {
            // 普通字元直接印
            char str[2] = {format[i], '\0'};
            print(str);
        }
    }

    va_end(args);
}

// int open(char* filename) {
//     int fd;
//     __asm__ volatile ("int $0x80" : "=a"(fd) : "a"(3), "b"(filename) : "memory");
//     return fd;
// }

// int read(int fd, char* buffer, int size) {
//     int bytes;
//     __asm__ volatile ("int $0x80" : "=a"(bytes) : "a"(4), "b"(fd), "c"(buffer), "d"(size) : "memory");
//     return bytes;
// }
