#include <stdint.h>
#include <stddef.h>
#include "tty.h"
#include "utils.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "timer.h"
#include "task.h"


// 宣告兩個任務結構
task_t task_A;
task_t task_B;

// 準備給 Task B 使用的獨立 4KB 堆疊
uint8_t task_B_stack[4096];

// Task B 要執行的無窮迴圈
void task_b_main() {
    while (1) {
        kprintf(" B ");
        // 執行完一次，把控制權交還給 Task A
        switch_task(&task_B.esp, task_A.esp);
    }
}

void kernel_main(void) {
    terminal_initialize();
    kprintf("=== OS Kernel Booting ===\n");
    init_gdt();
    init_idt();
    init_paging();


    // 執行 sti (Set Interrupt Flag) 開啟全域中斷
    // 從這行開始，CPU 會開始接聽外部硬體的呼叫！
    // __asm__ volatile ("sti");
    // kprintf("System is ready. Start typing!\n");
    // kprintf("> ");


    // === 手動偽造 Task B 的初始堆疊環境 ===
    // 讓 esp 指向堆疊最頂端
    uint32_t* stack = (uint32_t*)(&task_B_stack[4096]);

    // 偽造 switch_task() 回傳時需要的狀態 (對應 pop ebp, edi, esi, ebx 以及 ret)
    *(--stack) = (uint32_t)task_b_main; // ret 彈出這個，就會跳去執行 task_b_main
    *(--stack) = 0; // ebp
    *(--stack) = 0; // edi
    *(--stack) = 0; // esi
    *(--stack) = 0; // ebx

    // 儲存 Task B 偽造好的堆疊指標
    task_B.esp = (uint32_t)stack;

    // === 開始在 Task A (Kernel Main) 與 Task B 之間切換 ===
    for (int i = 0; i < 10; i++) {
        kprintf(" A ");
        // 跳去 Task B！(這會把現在的狀態存入 task_A.esp，然後載入 task_B.esp)
        switch_task(&task_A.esp, task_B.esp);
    }

    kprintf("\nContext Switch successful! System halting.\n");


    // 讓核心進入無限休眠迴圈，有中斷來才會醒來做事
    while (1) { __asm__ volatile ("hlt"); }
}
