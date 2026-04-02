/**
 * @file src/user/bin/clock.c
 * @brief Main logic and program flow for clock.c.
 *
 * This file handles the operations and logic associated with clock.c.
 */

#include "stdio.h"
#include "unistd.h"
#include "simpleui.h"

#define CW 120
#define CH 60

int main() {
    int win_id = create_gui_window("Digital Clock", CW + 4, CH + 24);
    if (win_id < 0) return -1;

    unsigned int* my_canvas = (unsigned int*)sbrk(CW * CH * 4);
    int blink = 0;

    while (1) {
        int h, m;
        get_time(&h, &m);

        // 1. 清空畫布 (填滿純黑)
        ui_draw_rect(my_canvas, CW, CH, 0, 0, CW, CH, 0x000000);

        // 2. 畫小時 (十位數與個位數，綠色)
        ui_draw_digit(my_canvas, CW, CH, h / 10, 15, 18, 0x00FF00);
        ui_draw_digit(my_canvas, CW, CH, h % 10, 35, 18, 0x00FF00);

        // 3. 畫中間的冒號 (每秒閃爍一次)
        if (blink % 2 == 0) {
            ui_draw_rect(my_canvas, CW, CH, 56, 23, 4, 4, 0x00FF00);
            ui_draw_rect(my_canvas, CW, CH, 56, 33, 4, 4, 0x00FF00);
        }

        // 4. 畫分鐘
        ui_draw_digit(my_canvas, CW, CH, m / 10, 65, 18, 0x00FF00);
        ui_draw_digit(my_canvas, CW, CH, m % 10, 85, 18, 0x00FF00);

        // 5. 推送給 Window Manager
        update_gui_window(win_id, my_canvas);

        blink++;

        // 粗略等待半秒鐘 (讓冒號閃爍)
        for(volatile int i = 0; i < 3000000; i++);
    }

    return 0;
}
