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

// [新增] 封裝 sys_yield
void sys_yield() {
    __asm__ volatile ("int $0x80" : : "a"(6) : "memory");
}

// [新增] 封裝 sys_exit
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
        }
        else if (strcmp(cmd_buffer, "about") == 0) {
            sys_print("Simple OS v1.0\nBuilt from scratch in 30 days!\n");
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
            // [新增] 處理 exit 指令
            sys_print("Goodbye!\n");
            sys_exit(); // 呼叫核心，了結自己！
        }
        else {
            sys_print("Command not found: ");
            sys_print(cmd_buffer);
            sys_print("\n");
        }
    }
}
