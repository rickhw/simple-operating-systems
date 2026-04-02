/**
 * @file src/kernel/drivers/rtc/rtc.c
 * @brief Main logic and program flow for rtc.c.
 *
 * This file handles the operations and logic associated with rtc.c.
 */

// RTC: Real-Time Clock
#include "io.h"
#include "rtc.h"

#define CMOS_ADDRESS 0x70
#define CMOS_DATA    0x71

// 宣告 I/O 函式
extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t data);


static int get_update_in_progress_flag() {
    outb(CMOS_ADDRESS, 0x0A);
    return (inb(CMOS_DATA) & 0x80);
}

static uint8_t get_RTC_register(int reg) {
    outb(CMOS_ADDRESS, reg);
    return inb(CMOS_DATA);
}

// CMOS 預設使用 BCD (Binary-Coded Decimal) 格式儲存數字，必須轉回一般二進位
static uint8_t bcd_to_binary(uint8_t bcd) {
    return ((bcd & 0xF0) >> 1) + ((bcd & 0xF0) >> 3) + (bcd & 0x0F);
}

void read_time(int* hour, int* minute) {
    // 等待時鐘更新完畢
    while (get_update_in_progress_flag());

    uint8_t h = get_RTC_register(0x04);
    uint8_t m = get_RTC_register(0x02);
    uint8_t regB = get_RTC_register(0x0B);

    // 如果不是二進位模式，就從 BCD 轉換
    if (!(regB & 0x04)) {
        h = bcd_to_binary(h);
        m = bcd_to_binary(m);
    }

    // 處理 12 小時制
    if (!(regB & 0x02) && (h & 0x80)) {
        h = ((h & 0x7F) + 12) % 24;
    }

    // 將底層的 UTC 時間加上 8 小時 (轉換為本地時間)
    *hour = (h + 8) % 24;
    *minute = m;
}
