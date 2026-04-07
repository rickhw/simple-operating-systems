#include "stdio.h"
#include "syscall.h"
#include "stdarg.h"

/**
 * @file stdio.c
 * @brief 標準輸入輸出函式實作
 */

// Syscall 2: sys_print
void print(const char* str) {
    syscall(SYS_PRINT_STR, (int)str, 0, 0, 0, 0);
}

// Syscall 5: sys_getchar
char getchar(void) {
    return (char)syscall(SYS_GETCHAR, 0, 0, 0, 0, 0);
}

/**
 * @brief 內部工具：將整數轉為字串並印出
 * @param num 整數值
 * @param base 進位 (10 或 16)
 */
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
        num = -num;
    }

    while (num != 0) {
        int rem = num % base;
        buf[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num /= base;
    }

    if (is_neg) buf[i++] = '-';

    // 反向印出
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
            i++; 
            switch (format[i]) {
                case 'd': { // 十進位
                    int num = va_arg(args, int);
                    print_int(num, 10);
                    break;
                }
                case 'x': { // 十六進位
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
                case 'c': { // 字元
                    char c = (char)va_arg(args, int);
                    char str[2] = {c, '\0'};
                    print(str);
                    break;
                }
                case '%': { // 百分比符號
                    print("%");
                    break;
                }
                default: { // 不支援格式
                    char str[3] = {'%', format[i], '\0'};
                    print(str);
                    break;
                }
            }
        } else {
            char str[2] = {format[i], '\0'};
            print(str);
        }
    }

    va_end(args);
}
