int sys_fork() {
    int pid;
    __asm__ volatile ("int $0x80" : "=a"(pid) : "a"(8) : "memory");
    return pid;
}

int sys_exec(char* filename, char** argv) {
    int ret;
    // ebx 傳檔名，ecx 傳字串陣列指標
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(9), "b"(filename), "c"(argv) : "memory");
    return ret;
}

int sys_wait(int pid) {
    int ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(10), "b"(pid) : "memory");
    return ret;
}

// 系統呼叫封裝
void sys_print(char* msg) {
    __asm__ volatile ("int $0x80" : : "a"(2), "b"(msg) : "memory");
}

int sys_open(char* filename) {
    int fd;
    __asm__ volatile ("int $0x80" : "=a"(fd) : "a"(3), "b"(filename) : "memory");
    return fd;
}

int sys_read(int fd, char* buffer, int size) {
    int bytes;
    __asm__ volatile ("int $0x80" : "=a"(bytes) : "a"(4), "b"(fd), "c"(buffer), "d"(size) : "memory");
    return bytes;
}

char sys_getchar() {
    int c;
    __asm__ volatile ("int $0x80" : "=a"(c) : "a"(5) : "memory");
    return (char)c;
}

// User Space 專用的字串比對工具
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// 讀取一整行指令
void read_line(char* buffer, int max_len) {
    int i = 0;
    while (i < max_len - 1) {
        char c = sys_getchar();
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

void sys_yield() {
    __asm__ volatile ("int $0x80" : : "a"(6) : "memory");
}

void sys_exit() {
    __asm__ volatile ("int $0x80" : : "a"(7) : "memory");
}

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
        sys_print("\n======================================\n");
        sys_print("      Welcome to Simple OS Shell!     \n");
        sys_print("======================================\n");
    }

    char cmd_buffer[128];

    while (1) {
        sys_print("SimpleOS> ");
        read_line(cmd_buffer, 128);
        if (cmd_buffer[0] == '\0') continue;

        // 【核心魔法】解析使用者輸入
        char* parsed_argv[16];
        int parsed_argc = parse_args(cmd_buffer, parsed_argv);
        char* cmd = parsed_argv[0];

        // 內建指令 (Built-ins)
        if (strcmp(cmd, "help") == 0) {
            sys_print("Built-in commands: help, about, exit\n");
            sys_print("External apps    : echo, cat\n");
        }
        else if (strcmp(cmd, "about") == 0) {
            sys_print("Simple OS v1.0 - Dynamic Shell Edition\n");
        }
        else if (strcmp(cmd, "exit") == 0) {
            sys_print("Goodbye!\n");
            sys_exit();
        }
        // [Day39] add -- start
        else if (strcmp(cmd_buffer, "ipc") == 0) {
            sys_print("\n--- Starting IPC Queue Test ---\n");

            // 1. 創造 Pong (收信者) - 讓它準備連續收兩封信！
            int pid_pong = sys_fork();
            if (pid_pong == 0) {
                char* args[] = {"pong.elf", 0};
                sys_exec("pong.elf", args);
                sys_exit();
            }

            sys_yield(); // 讓 Pong 先去待命

            // 2. 創造 第一個 Ping
            int pid_ping1 = sys_fork();
            if (pid_ping1 == 0) {
                char* args[] = {"ping.elf", 0};
                sys_exec("ping.elf", args);
                sys_exit();
            }

            // 3. 創造 第二個 Ping
            int pid_ping2 = sys_fork();
            if (pid_ping2 == 0) {
                // 為了區分，我們假裝它是另一個程式，但其實跑一樣的 logic，只是行程 ID 不同
                char* args[] = {"ping.elf", 0};
                sys_exec("ping.elf", args);
                sys_exit();
            }

            // 4. 老爸等待所有人結束
            sys_wait(pid_pong);
            sys_wait(pid_ping1);
            sys_wait(pid_ping2);
            sys_print("--- IPC Test Finished! ---\n\n");
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

            int pid = sys_fork();
            if (pid == 0) {
                int err = sys_exec(elf_name, parsed_argv);
                if (err == -1) {
                    sys_print("Command not found: ");
                    sys_print(cmd);
                    sys_print("\n");
                }
                sys_exit();
            } else {
                sys_wait(pid);
            }
        }
    }
    return 0;
}


//     while (1) {
//         sys_print("SimpleOS> ");

//         // 讀取使用者輸入的指令
//         read_line(cmd_buffer, 128);

//         // 執行指令邏輯
//         if (strcmp(cmd_buffer, "") == 0) {
//             continue;
//         }
//         else if (strcmp(cmd_buffer, "help") == 0) {
//             sys_print("Available commands:\n");
//             sys_print("  help    - Show this message\n");
//             sys_print("  cat     - Read 'hello.txt' from disk\n");
//             sys_print("  about   - OS information\n");
//             sys_print("  fork    - Test pure fork()\n");
//             sys_print("  run     - Test fork() + exec() to spawn a new shell\n");
//         }
//         else if (strcmp(cmd_buffer, "about") == 0) {
//             sys_print("Simple OS v1.0\nBuilt from scratch in 35 days!\n");
//         }
//         else if (strcmp(cmd_buffer, "cat") == 0) {
//             int fd = sys_open("hello.txt");
//             if (fd == -1) {
//                 sys_print("Error: hello.txt not found.\n");
//             } else {
//                 char file_buf[128];
//                 for(int i=0; i<128; i++) file_buf[i] = 0;
//                 sys_read(fd, file_buf, 100);
//                 sys_print("--- File Content ---\n");
//                 sys_print(file_buf);
//                 sys_print("\n--------------------\n");
//             }
//         }
//         else if (strcmp(cmd_buffer, "exit") == 0) {
//             sys_print("Goodbye!\n");
//             sys_exit();
//         }
//         else if (strcmp(cmd_buffer, "fork") == 0) {
//             int pid = sys_fork();

//             if (pid == 0) {
//                 sys_print("\n[CHILD] Hello! I am the newborn process!\n");
//                 sys_print("[CHILD] My work here is done, committing suicide...\n");
//                 sys_exit();
//             } else {
//                 sys_print("\n[PARENT] Magic! I just created a child process!\n");
//                 sys_yield();
//                 sys_yield();
//             }
//         }
//         // [Day35][新增] 測試 Fork-Exec 模型
//         else if (strcmp(cmd_buffer, "run") == 0) {
//             int pid = sys_fork();
//             if (pid == 0) {
//                 // 準備傳遞給新程式的參數陣列 (最後必須是 0/NULL)
//                 char* my_args[] = {"my_app.elf", "Hello", "From", "Parent!", 0};

//                 sys_exec("my_app.elf", my_args);
//                 sys_exit();
//             } else {
//                 sys_print("\n[PARENT] Spawned child. Waiting for it to finish...\n");
//                 sys_wait(pid);
//                 sys_print("[PARENT] Child has finished! I am back in control.\n");
//             }
//         }
//         // [新增] 呼叫外部的 echo.elf
//         else if (strcmp(cmd_buffer, "echo") == 0) {
//             int pid = sys_fork();
//             if (pid == 0) {
//                 // 準備要丟給 echo.elf 的參數 (最後必須是 0 結尾)
//                 char* echo_args[] = {"echo.elf", "Hello", "Rick,", "Welcome", "to", "the", "Multiverse!", 0};

//                 // 靈魂轉移！將子行程變成 echo.elf
//                 int err = sys_exec("echo.elf", echo_args);

//                 if (err == -1) {
//                     sys_print("Error: Exec failed to load echo.elf\n");
//                 }
//                 sys_exit();
//             } else {
//                 sys_wait(pid);
//             }
//         }
//         else {
//             sys_print("Command not found: ");
//             sys_print(cmd_buffer);
//             sys_print("\n");
//         }
//     }

//     return 0; // 配合 int main 的回傳值
// }
