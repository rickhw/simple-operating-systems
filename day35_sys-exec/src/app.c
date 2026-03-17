// [Day34] 封裝 sys_fork
int sys_fork() {
    int pid;
    __asm__ volatile ("int $0x80" : "=a"(pid) : "a"(8) : "memory");
    return pid;
}

// [Day35][新增] 封裝 sys_exec (Syscall 9)
int sys_exec(char* filename) {
    int ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(9), "b"(filename) : "memory");
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

void _start() {
    sys_print("\n======================================\n");
    sys_print("      Welcome to Simple OS Shell!     \n");
    sys_print("======================================\n");
    sys_print("Type 'help' to see available commands.\n\n");

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

            // if (pid == 0) {
            //     // 我是分身！我現在要洗掉自己的記憶體，轉生為一個全新的 Shell！
            //     sys_print("\n[CHILD] Executing my_app.elf to replace my soul...\n");

            //     // 執行變身魔法
            //     sys_exec("my_app.elf");

            //     // --- 以下的程式碼理論上「永遠不會」被執行到 ---
            //     // 因為只要 exec 成功，整個記憶體與 EIP 就被覆蓋成新程式的起點了！
            //     sys_print("[CHILD] ERROR: Exec failed! I am still the old me!\n");
            //     sys_exit();
            // } else {
            //     // 我是老爸！
            //     sys_print("\n[PARENT] Spawned a child to run a new instance of Shell.\n");
            //     sys_yield();
            //     sys_yield();
            // }
            if (pid == 0) {
                sys_print("\n[CHILD] Executing my_app.elf to replace my soul...\n");
                sys_exec("my_app.elf");
                sys_exit();
            } else {
                // 我是老爸！
                sys_print("\n[PARENT] Spawned child (PID ");
                // 這裡簡化印出，直接告訴大家我在等
                sys_print("). Waiting for it to finish...\n");

                // 【終極魔法】老爸陷入沉睡，把鍵盤和螢幕全部讓給小孩！
                sys_wait(pid);

                // 當執行到這裡時，代表小孩已經死了，老爸被喚醒了！
                sys_print("[PARENT] Child has finished! I am back in control.\n");
            }
        }
        else {
            sys_print("Command not found: ");
            sys_print(cmd_buffer);
            sys_print("\n");
        }
    }
}
