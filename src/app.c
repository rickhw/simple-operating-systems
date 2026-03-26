// [Day20] add - start
// 這是完全獨立於 Kernel 的程式碼！沒有 #include 任何核心的標頭檔。

// 我們不叫 main，叫 _start，因為我們沒有標準函式庫 (libc) 幫我們啟動 main。
void _start() {
    // 呼叫我們在 Kernel 寫好的 Syscall 2 (印出字串)
    char* msg = "Hello from an EXTERNAL ELF File running in Ring 3!";

    __asm__ volatile (
        "mov $2, %%eax\n"
        "mov %0, %%ebx\n"
        "int $0x80\n"
        : : "r" (msg) : "eax", "ebx"
    );

    // 應用程式結束後進入無窮迴圈
    while(1);
}
// [Day20] add - end
