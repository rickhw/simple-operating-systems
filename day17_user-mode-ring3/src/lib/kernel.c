#include <stdint.h>
#include <stddef.h>
#include "tty.h"
#include "utils.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "timer.h"
#include "pmm.h"
#include "kheap.h"
#include "syscall.h"

extern void enter_user_mode(uint32_t user_function_ptr);

// 宣告一個函數：這將是我們歷史上第一個在 Ring 3 跑起來的「應用程式」！
void my_first_user_app() {
    // [注意！] 我們現在是平民 (Ring 3) 了。
    // 如果你在這裡直接呼叫 kprintf(...)，系統會立刻因為權限不足 (Page Fault) 當機！

    char* msg = "Hello from Ring 3 User Space!";

    // 我們只能乖乖敲打防彈玻璃，呼叫昨天寫好的 Syscall 2
    __asm__ volatile (
        "mov $2, %%eax\n"
        "mov %0, %%ebx\n"
        "int $0x80\n"
        : : "r" (msg) : "eax", "ebx"
    );

    // User App 的無限迴圈 (注意：平民連執行 hlt 讓 CPU 休息的權限都沒有，只能用 while(1))
    while (1) {}
}

void kernel_main(void) {
    terminal_initialize();
    kprintf("=== OS Kernel Booting ===\n");

    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384);
    init_kheap();
    init_syscalls();

    // [極度重要] 在離開 Kernel 之前，把目前安全的核心 Stack 位址告訴 TSS。
    // 這樣 User App 執行 int 0x80 時，CPU 才知道要跳回哪裡處理。
    uint32_t current_esp;
    __asm__ volatile ("mov %%esp, %0" : "=r" (current_esp));
    set_kernel_stack(current_esp);

    kprintf("Kernel initialized. Dropping privileges to Ring 3...\n");

    // 啟動魔法！
    enter_user_mode((uint32_t)my_first_user_app);

    // 這行永遠不會被印出來，因為權限已經交出去了！
    kprintf("This should never print.\n");
}
