// 系統呼叫封裝
int sys_open(char* filename) {
    int fd;
    __asm__ volatile (
        "int $0x80"
        : "=a" (fd)             // 輸出：執行完後，把 eax 的值放進 fd
        : "a" (3),              // 輸入：執行前，把 3 放進 eax
          "b" (filename)        // 輸入：執行前，把 filename 放進 ebx
        : "memory"              // 告訴編譯器記憶體可能被改變
    );
    return fd;
}

int sys_read(int fd, char* buffer, int size) {
    int bytes_read;
    __asm__ volatile (
        "int $0x80"
        : "=a" (bytes_read)     // 輸出：把 eax 的值放進 bytes_read
        : "a" (4),              // 輸入：eax = 4
          "b" (fd),             // 輸入：ebx = fd
          "c" (buffer),         // 輸入：ecx = buffer
          "d" (size)            // 輸入：edx = size
        : "memory"
    );
    return bytes_read;
}

void sys_print(char* msg) {
    __asm__ volatile (
        "int $0x80"
        :                       // 沒有輸出變數
        : "a" (2),              // 輸入：eax = 2
          "b" (msg)             // 輸入：ebx = msg
        : "memory"
    );
}

void _start() {
    sys_print("[User App] Hello! I am running in Ring 3!\n");
    sys_print("[User App] Requesting Kernel to open 'hello.txt'...\n");

    int fd = sys_open("hello.txt");

    if (fd == -1) {
        sys_print("[User App] Failed to open file!\n");
    } else {
        sys_print("[User App] File opened! FD received. Reading data...\n");

        char buffer[128];
        for(int i=0; i<128; i++) buffer[i] = 0; // 清空

        sys_read(fd, buffer, 100);

        sys_print("[User App] File Content -> ");
        sys_print(buffer);
        sys_print("\n");
    }

    while(1);
}
