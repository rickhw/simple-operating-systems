/**
 * @file src/kernel/arch/x86/timer.c
 * @brief Main logic and program flow for timer.c.
 *
 * This file handles the operations and logic associated with timer.c.
 */

#include <stddef.h>
#include "timer.h"
#include "io.h"
#include "utils.h"
#include "task.h"
#include "gui.h"

volatile uint32_t tick = 0;

void init_timer(uint32_t frequency) {
    uint32_t divisor = 1193180 / frequency;
    outb(0x43, 0x36);

    uint8_t l = (uint8_t)(divisor & 0xFF);
    uint8_t h = (uint8_t)((divisor >> 8) & 0xFF);
    outb(0x40, l);
    outb(0x40, h);
}

void timer_handler(void) {
    tick++;

    // ==========================================
    // 計算 CPU 使用率核心邏輯
    // 現在是誰在用 CPU，這個滴答的功勞就記在誰頭上！
    // ==========================================
    if (current_task != 0) {
        current_task->total_ticks++;
    }

    // 每一滴答 (Tick)，檢查並刷新畫面！
    gui_handle_timer();

    // 必須先發送 EOI，才能切換任務！否則 PIC 會被卡死
    outb(0x20, 0x20);

    // 強行切換任務
    schedule();
}
