#include "stdio.h"
#include "unistd.h"

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


int parse_args(char* input, char** argv) {
    int argc = 0, i = 0;
    int in_word = 0;
    int in_quote = 0; // 新增狀態：是否在雙引號內

    while (input[i] != '\0') {
        if (input[i] == '"') {
            if (in_quote) {
                input[i] = '\0'; // 遇到結尾引號，斷開字串
                in_quote = 0;
                in_word = 0;
            } else {
                in_quote = 1;
                argv[argc++] = &input[i + 1]; // 指向引號的下一個字元
                in_word = 1;
            }
        }
        else if (input[i] == ' ' && !in_quote) {
            // 只有在「引號外面」的空白，才會斷開字串
            input[i] = '\0';
            in_word = 0;
        }
        else {
            if (!in_word && !in_quote) {
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

        // 解析使用者輸入
        char* parsed_argv[16];
        int parsed_argc = parse_args(cmd_buffer, parsed_argv);
        if (parsed_argc == 0) continue; // 避免空陣列崩潰

        char* cmd = parsed_argv[0];

        // ==========================================
        // 【Day 68 擴充】檢查最後一個參數是不是 "&" (背景執行)
        // ==========================================
        int is_background = 0;
        if (parsed_argc > 1 && strcmp(parsed_argv[parsed_argc - 1], "&") == 0) {
            is_background = 1;
            parsed_argv[parsed_argc - 1] = 0; // 把 "&" 從參數清單拿掉，不傳給應用程式
            parsed_argc--;
        }

        // 內建指令 (Built-ins)
        if (strcmp(cmd, "help") == 0) {
            printf("Built-in commands: help, about, ipc, cd, pwd, exit, pid, desktop\n");
            printf("External commands: echo, cat, ping, pong, ls, touch, mkdir, ps, top\n");
        }
        else if (strcmp(cmd, "about") == 0) {
            printf("Simple OS v1.0 - Dynamic Shell Edition\n");
        }
        else if (strcmp(cmd, "exit") == 0) {
            printf("Goodbye!\n");
            exit();
        }
        else if (strcmp(cmd, "cd") == 0) {
            if (parsed_argc < 2) {
                printf("Usage: cd <directory>\n");
            } else {
                if (chdir(parsed_argv[1]) != 0) {
                    printf("cd: %s: No such directory\n", parsed_argv[1]);
                }
            }
        }
        else if (strcmp(cmd, "pwd") == 0) {
            char path_buf[256];
            getcwd(path_buf);
            printf("%s\n", path_buf);
        }
        // 檢查是不是 desktop 指令
        else if (strcmp(cmd, "desktop") == 0) {
            set_display_mode(1); // 呼叫 syscall，要求 Kernel 切換到 GUI
            continue; // 直接迴圈，不用執行其他程式
        }
        else if (strcmp(cmd, "pid") == 0) {
            printf("Shell PID: %d\n", getpid());
        }
        else if (strcmp(cmd_buffer, "ipc") == 0) {
            printf("\n--- Starting IPC Queue Test ---\n");

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
            printf("--- IPC Test Finished! ---\n\n");
        }
        // 動態執行外部程式
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
                    printf("Command not found: ");
                    printf(cmd);
                    printf("\n");
                }
                exit();
            } else {
                // ==========================================
                // 【修改】如果是背景執行，老爸就不等了！直接印出 PID 並繼續接客
                // ==========================================
                if (is_background) {
                    printf("[Running in background] PID: %d\n", pid);
                } else {
                    wait(pid); // 不是背景執行的話，老爸還是乖乖等兒子死掉
                }
            }
        }
    }
    return 0;
}
