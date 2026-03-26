#include "stdio.h"
#include "unistd.h"

// int fork() {
//     int pid;
//     __asm__ volatile ("int $0x80" : "=a"(pid) : "a"(8) : "memory");
//     return pid;
// }

// int exec(char* filename, char** argv) {
//     int ret;
//     // ebx 傳檔名，ecx 傳字串陣列指標
//     __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(9), "b"(filename), "c"(argv) : "memory");
//     return ret;
// }

// int wait(int pid) {
//     int ret;
//     __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(10), "b"(pid) : "memory");
//     return ret;
// }

// // 系統呼叫封裝
// void print(char* msg) {
//     __asm__ volatile ("int $0x80" : : "a"(2), "b"(msg) : "memory");
// }

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

// char getchar() {
//     int c;
//     __asm__ volatile ("int $0x80" : "=a"(c) : "a"(5) : "memory");
//     return (char)c;
// }

// User Space 專用的字串比對工具
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// 讀取一整行指令
void read_line(char* buffer, int max_len) {
    int i = 0;
    while (i < max_len - 1) {
        char c = getchar();
        if (c == '\n') {
            break; // 使用者按下了 Enter
        } else if (c == '\b') {
            if (i > 0) i--; // 處理倒退鍵 (Backspace)
        } else {
            buffer[i++] = c;
        }
    }
    buffer[i] = '\0'; // 字串結尾
}

// void yield() {
//     __asm__ volatile ("int $0x80" : : "a"(6) : "memory");
// }

// void exit() {
//     __asm__ volatile ("int $0x80" : : "a"(7) : "memory");
// }

// 【新增】簡易字串切割器 (將空白替換為 \0，並把指標存入 argv)
int parse_args(char* input, char** argv) {
    int argc = 0;
    int i = 0;
    int in_word = 0;

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
    argv[argc] = 0; // 結尾補 NULL
    return argc;
}

// 【修改】將 _start 改名為 main，並回傳 int
int main(int argc, char** argv) {
    // 為了畫面乾淨，我們只在沒有參數 (頂層 Shell) 時印出歡迎詞
    if (argc <= 1) {
        print("\n======================================\n");
        print("      Welcome to Simple OS Shell!     \n");
        print("======================================\n");
    }

    char cmd_buffer[128];

    while (1) {
        print("SimpleOS> ");
        read_line(cmd_buffer, 128);
        if (cmd_buffer[0] == '\0') continue;

        // 【核心魔法】解析使用者輸入
        char* parsed_argv[16];
        int parsed_argc = parse_args(cmd_buffer, parsed_argv);
        char* cmd = parsed_argv[0];

        // 內建指令 (Built-ins)
        if (strcmp(cmd, "help") == 0) {
            print("Built-in commands: help, about, exit\n");
            print("External apps    : echo, cat\n");
        }
        else if (strcmp(cmd, "about") == 0) {
            print("Simple OS v1.0 - Dynamic Shell Edition\n");
        }
        else if (strcmp(cmd, "exit") == 0) {
            print("Goodbye!\n");
            exit();
        }
        // [Day39] add -- start
        else if (strcmp(cmd_buffer, "ipc") == 0) {
            print("\n--- Starting IPC Queue Test ---\n");

            // 1. 創造 Pong (收信者) - 讓它準備連續收兩封信！
            int pid_pong = fork();
            if (pid_pong == 0) {
                char* args[] = {"pong.elf", 0};
                exec("pong.elf", args);
                exit();
            }

            yield(); // 讓 Pong 先去待命

            // 2. 創造 第一個 Ping
            int pid_ping1 = fork();
            if (pid_ping1 == 0) {
                char* args[] = {"ping.elf", 0};
                exec("ping.elf", args);
                exit();
            }

            // 3. 創造 第二個 Ping
            int pid_ping2 = fork();
            if (pid_ping2 == 0) {
                // 為了區分，我們假裝它是另一個程式，但其實跑一樣的 logic，只是行程 ID 不同
                char* args[] = {"ping.elf", 0};
                exec("ping.elf", args);
                exit();
            }

            // 4. 老爸等待所有人結束
            wait(pid_pong);
            wait(pid_ping1);
            wait(pid_ping2);
            print("--- IPC Test Finished! ---\n\n");
        }
        // [Day39] add -- end
        // 【動態執行外部程式】
        else {
            // 自動幫指令加上 .elf 副檔名
            char elf_name[32];
            char* dest = elf_name;
            char* src = cmd;
            while(*src) *dest++ = *src++;
            *dest++ = '.'; *dest++ = 'e'; *dest++ = 'l'; *dest++ = 'f'; *dest = '\0';

            int pid = fork();
            if (pid == 0) {
                int err = exec(elf_name, parsed_argv);
                if (err == -1) {
                    print("Command not found: ");
                    print(cmd);
                    print("\n");
                }
                exit();
            } else {
                wait(pid);
            }
        }
    }
    return 0;
}
