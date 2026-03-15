#include <stddef.h>
#include "timer.h"
#include "io.h"
#include "utils.h"

// 記錄系統開機以來經過了多少個 Tick
volatile uint32_t tick = 0;

void timer_handler(void) {
    tick++;

    // 每 100 個 tick 就是 1 秒 (因為我們等一下會設定頻率為 100Hz)
    if (tick % 100 == 0) {
        kprintf("Uptime: %d seconds\n", tick / 100);
    }

    // [重要] 必須告訴 PIC 我們處理完這個中斷了，否則它不會送下一個 Tick 來
    outb(0x20, 0x20);
}

void init_timer(uint32_t frequency) {
    // 晶片硬體的基礎頻率是 1193180 Hz
    uint32_t divisor = 1193180 / frequency;

    // 傳送指令 byte 到 Port 0x43 (Command Register)
    // 0x36 = 0011 0110
    // 意思是：選擇 Channel 0, 先傳送低位元組再傳送高位元組, Mode 3 (方波發生器), 二進位模式
    outb(0x43, 0x36);

    // 拆分除數為低位元組 (Low byte) 與高位元組 (High byte)
    uint8_t l = (uint8_t)(divisor & 0xFF);
    uint8_t h = (uint8_t)((divisor >> 8) & 0xFF);

    // 依序寫入資料到 Port 0x40 (Channel 0 Data Register)
    outb(0x40, l);
    outb(0x40, h);
}
