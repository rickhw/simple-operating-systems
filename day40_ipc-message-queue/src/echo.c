// src/echo.c
// 這是一個完全獨立的外部程式！

void sys_print(char* msg) {
    __asm__ volatile ("int $0x80" : : "a"(2), "b"(msg) : "memory");
}

// 我們的 crt0.S 已經會自動幫我們呼叫 sys_exit，
// 所以只要寫標準的 main 函式即可！
int main(int argc, char** argv) {
    sys_print("\n[ECHO Program] Start printing arguments...\n");

    // 如果沒有參數，就隨便印點東西
    if (argc <= 1) {
        sys_print("  (No arguments provided)\n");
        return 0;
    }

    // 從 i=1 開始印，因為 argv[0] 通常是程式自己的名字 ("echo.elf")
    for (int i = 1; i < argc; i++) {
        sys_print("  ");
        sys_print(argv[i]);
        sys_print("\n");
    }

    sys_print("[ECHO Program] Done.\n");
    return 0;
}
