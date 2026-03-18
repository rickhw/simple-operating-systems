// [Day34] 封裝 sys_fork
int sys_fork() {
    int pid;
    __asm__ volatile ("int $0x80" : "=a"(pid) : "a"(8) : "memory");
    return pid;
}

// // [Day35][新增] 封裝 sys_exec (Syscall 9)
// int sys_exec(char* filename) {
//     int ret;
//     __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(9), "b"(filename) : "memory");
//     return ret;
// }

int sys_exec(char* filename, char** argv) {
    int ret;
    // ebx 傳檔名，ecx 傳字串陣列指標
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(9), "b"(filename), "c"(argv) : "memory");
    return ret;
}

// [新增] 封裝 sys_wait (Syscall 10)
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

// 【修改】將 _start 改名為 main，並回傳 int
int main(int argc, char** argv) {
    sys_print("\n======================================\n");
    sys_print("      Welcome to Simple OS Shell!     \n");
    sys_print("======================================\n");

    // 把接收到的參數印出來證明靈魂轉移成功！
    if (argc > 0) {
        sys_print("Awesome! I received arguments:\n");
        for(int i = 0; i < argc; i++) {
            sys_print("  Arg ");
            char num[2] = {i + '0', '\0'};
            sys_print(num);
            sys_print(": ");

            // 增加安全檢查，確保 argv[i] 不是 NULL
            if (argv[i] != 0) {
                sys_print(argv[i]);
            } else {
                sys_print("(null)");
            }
            sys_print("\n");
        }
        sys_print("\n");
    }

    char cmd_buffer[128];

    while (1) {
        sys_print("SimpleOS> ");

        // 讀取使用者輸入的指令
        read_line(cmd_buffer, 128);

        // 執行指令邏輯
        if (strcmp(cmd_buffer, "") == 0) {
            continue;
        }
        else if (strcmp(cmd_buffer, "help") == 0) {
            sys_print("Available commands:\n");
            sys_print("  help    - Show this message\n");
            sys_print("  cat     - Read 'hello.txt' from disk\n");
            sys_print("  about   - OS information\n");
            sys_print("  fork    - Test pure fork()\n");
            sys_print("  run     - Test fork() + exec() to spawn a new shell\n");
        }
        else if (strcmp(cmd_buffer, "about") == 0) {
            sys_print("Simple OS v1.0\nBuilt from scratch in 35 days!\n");
        }
        else if (strcmp(cmd_buffer, "cat") == 0) {
            int fd = sys_open("hello.txt");
            if (fd == -1) {
                sys_print("Error: hello.txt not found.\n");
            } else {
                char file_buf[128];
                for(int i=0; i<128; i++) file_buf[i] = 0;
                sys_read(fd, file_buf, 100);
                sys_print("--- File Content ---\n");
                sys_print(file_buf);
                sys_print("\n--------------------\n");
            }
        }
        else if (strcmp(cmd_buffer, "exit") == 0) {
            sys_print("Goodbye!\n");
            sys_exit();
        }
        else if (strcmp(cmd_buffer, "fork") == 0) {
            int pid = sys_fork();

            if (pid == 0) {
                sys_print("\n[CHILD] Hello! I am the newborn process!\n");
                sys_print("[CHILD] My work here is done, committing suicide...\n");
                sys_exit();
            } else {
                sys_print("\n[PARENT] Magic! I just created a child process!\n");
                sys_yield();
                sys_yield();
            }
        }
        // [Day35][新增] 測試 Fork-Exec 模型
        else if (strcmp(cmd_buffer, "run") == 0) {
            int pid = sys_fork();
            if (pid == 0) {
                // 準備傳遞給新程式的參數陣列 (最後必須是 0/NULL)
                char* my_args[] = {"my_app.elf", "Hello", "From", "Parent!", 0};

                sys_exec("my_app.elf", my_args);
                sys_exit();
            } else {
                sys_print("\n[PARENT] Spawned child. Waiting for it to finish...\n");
                sys_wait(pid);
                sys_print("[PARENT] Child has finished! I am back in control.\n");
            }
        }
        else {
            sys_print("Command not found: ");
            sys_print(cmd_buffer);
            sys_print("\n");
        }
    }

    return 0; // 配合 int main 的回傳值
}
