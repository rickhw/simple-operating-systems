#include <stdint.h>
#include <stddef.h>
#include "tty.h"
#include "utils.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "timer.h"

void kernel_main(void) {
    terminal_initialize();

    init_gdt();
    init_idt();
    init_paging();

    // [新增] 初始化計時器，設定為 100 Hz (每 10 毫秒觸發一次)
    // 設定 1000, 印出 訊息速度越快
    init_timer(100);
    kprintf("Timer initialized at 100Hz.\n");

    // 執行 sti (Set Interrupt Flag) 開啟全域中斷
    // 從這行開始，CPU 會開始接聽外部硬體的呼叫！
    __asm__ volatile ("sti");

    kprintf("System is ready. Start typing!\n");
    kprintf("> ");

    // 讓核心進入無限休眠迴圈，有中斷來才會醒來做事
    while (1) {
        __asm__ volatile ("hlt");
    }
}
