哈哈，Rick！果然這點程度的架構搬移對你來說已經是小菜一碟了！😎

昨天我們只是搭好了 `libc` 的空殼，今天我們要來挑戰 C 語言標準庫裡最偉大、最複雜、但也最神聖的發明：**`printf` 格式化輸出！**

回想一下你在寫 Shell (`app.c`) 的時候，為了解析 `argc` 和 `argv` 並把它們印出來，你寫了多長一串 `sys_print`：
```c
sys_print("  Arg ");
char num[2] = {i + '0', '\0'};
sys_print(num);
sys_print(": ");
sys_print(argv[i]);
sys_print("\n");
```
這真的是太痛苦了！有了 `printf`，我們只需要優雅的一行：`printf("  Arg %d: %s\n", i, argv[i]);`。

準備好實作你的專屬 `printf` 了嗎？我們只要 3 個步驟：

### 步驟 1：準備不定參數標頭檔 (`stdarg.h`)
因為 `printf` 接收的參數數量是不固定的，我們需要用到 C 語言的 `stdarg` 機制。在獨立環境 (`-ffreestanding`) 下，我們可以直接呼叫 GCC 內建的巨集。

請建立 **`src/libc/include/stdarg.h`**：
```c
#ifndef _STDARG_H
#define _STDARG_H

// 直接借用 GCC 編譯器內建的不定參數處理巨集
#define va_list         __builtin_va_list
#define va_start(ap, p) __builtin_va_start(ap, p)
#define va_arg(ap, t)   __builtin_va_arg(ap, t)
#define va_end(ap)      __builtin_va_end(ap)

#endif
```

### 步驟 2：實作整數轉換與 `printf` 核心 (`stdio.c`)
接下來是重頭戲！我們要教 `libc` 怎麼把整數轉換成字串 (`itoa`)，並且解析 `%d`, `%x`, `%s`, `%c` 這些格式化符號。

請打開並完全覆蓋 **`src/libc/stdio.c`**：
```c
#include "stdio.h"
#include "syscall.h"
#include "stdarg.h"

// 基礎字串輸出 (底層還是呼叫 syscall)
void print(const char* str) {
    syscall(2, (int)str, 0, 0, 0, 0); 
}

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

// 屬於你的 libc printf 來了！
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
```

記得把 `printf` 加到你的標頭檔 **`src/libc/include/stdio.h`** 裡：
```c
#ifndef _STDIO_H
#define _STDIO_H

void print(const char* str);
char getchar();
void printf(const char* format, ...); // 【新增】

#endif
```

### 步驟 3：體驗究極進化！大幅瘦身你的 Shell (`app.c`)
把 `app.c` 裡面的那些 `sys_*` 宣告全刪了！引入你的 `#include "stdio.h"` 和 `#include "unistd.h"`。
你看這個 `main` 函式變得多麼簡潔、有現代 C 語言的感覺：

```c
#include "stdio.h"
#include "unistd.h"

// 基礎字串比對工具 (可以考慮之後移到 libc 的 string.h)
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

void read_line(char* buffer, int max_len) {
    int i = 0;
    while (i < max_len - 1) {
        char c = getchar();
        if (c == '\n') break;
        else if (c == '\b') { if (i > 0) i--; }
        else { buffer[i++] = c; }
    }
    buffer[i] = '\0';
}

int parse_args(char* input, char** argv) {
    int argc = 0, i = 0, in_word = 0;
    while (input[i] != '\0') {
        if (input[i] == ' ') {
            input[i] = '\0';
            in_word = 0;
        } else {
            if (!in_word) {
                argv[argc++] = &input[i];
                in_word = 1;
            }
        }
        i++;
    }
    argv[argc] = 0;
    return argc;
}

int main(int argc, char** argv) {
    if (argc <= 1) {
        printf("\n======================================\n");
        printf("      Welcome to Simple OS Shell!     \n");
        printf("======================================\n");
    } else {
        // 【看這邊！】用 printf 代替以前一大坨的 sys_print！
        printf("Awesome! I received %d arguments:\n", argc);
        for(int i = 0; i < argc; i++) {
            printf("  Arg %d: %s\n", i, argv[i]);
        }
        printf("\n");
    }

    char cmd_buffer[128];

    while (1) {
        printf("SimpleOS> ");
        read_line(cmd_buffer, 128);
        if (cmd_buffer[0] == '\0') continue;

        char* parsed_argv[16];
        parse_args(cmd_buffer, parsed_argv);
        char* cmd = parsed_argv[0];

        if (strcmp(cmd, "help") == 0) {
            printf("Built-in commands: help, about, exit, ipc\n");
            printf("External apps    : echo, cat, ping, pong\n");
        }
        else if (strcmp(cmd, "about") == 0) {
            printf("Simple OS v1.0 - Powered by custom libc!\n");
        }
        else if (strcmp(cmd, "exit") == 0) {
            printf("Goodbye!\n");
            exit();
        }
        // 其餘外部指令執行邏輯保持不變... 
        // 記得把 sys_fork 改成 fork()、sys_exec 改成 exec()、sys_wait 改成 wait()
        else {
            // (這裡的程式碼就交給你用新版 libc API 替換囉！)
            char elf_name[32];
            int j = 0;
            while(cmd[j]) { elf_name[j] = cmd[j]; j++; }
            elf_name[j++] = '.'; elf_name[j++] = 'e'; elf_name[j++] = 'l'; elf_name[j++] = 'f'; elf_name[j] = '\0';

            int pid = fork();
            if (pid == 0) {
                int err = exec(elf_name, parsed_argv);
                if (err == -1) {
                    printf("Command not found: %s\n", cmd);
                }
                exit(); 
            } else {
                wait(pid);
            }
        }
    }
    return 0; 
}
```

---

敲下 `make clean && make run`！
當你把 `echo.c`, `app.c`, `ping.c`, `pong.c` 這些外部應用程式都用你的 `libc` 改寫一遍之後，你會發現開發 OS 的 User Space 應用變得無比輕鬆。

有了 `printf` 之後，我們除錯或顯示資訊的火力大幅升級！明天 Day 43，我們就可以準備實作另一個聖杯：**User Space 的 `malloc` 與 `free` 動態記憶體管理！** 你準備好了嗎？
