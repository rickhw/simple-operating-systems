#include "stdio.h"
#include "unistd.h"

#define CW 120
#define CH 60

// 畫實心方塊的輔助函式 (直接操作像素陣列)
void draw_rect(unsigned int* canvas, int x, int y, int w, int h, unsigned int color) {
    for(int cy = y; cy < y + h; cy++) {
        for(int cx = x; cx < x + w; cx++) {
            if (cx >= 0 && cx < CW && cy >= 0 && cy < CH) {
                canvas[cy * CW + cx] = color;
            }
        }
    }
}

// 超迷你 3x5 像素字型引擎！(1 代表有顏色，0 代表透明)
void draw_digit(unsigned int* canvas, int digit, int ox, int oy, unsigned int color) {
    unsigned int font[10][5] = {
        {7,5,5,5,7}, // 0 (111, 101, 101, 101, 111)
        {2,2,2,2,2}, // 1 (010, 010, 010, 010, 010)
        {7,1,7,4,7}, // 2
        {7,1,7,1,7}, // 3
        {5,5,7,1,1}, // 4
        {7,4,7,1,7}, // 5
        {7,4,7,5,7}, // 6
        {7,1,1,1,1}, // 7
        {7,5,7,5,7}, // 8
        {7,5,7,1,7}  // 9
    };

    for(int y = 0; y < 5; y++) {
        for(int x = 0; x < 3; x++) {
            // 用 bitwise 取出每一格是 1 還是 0
            if (font[digit][y] & (1 << (2 - x))) {
                // 每一個邏輯像素，我們用 5x5 的真實像素來畫，讓它看起來變大！
                draw_rect(canvas, ox + x * 5, oy + y * 5, 5, 5, color);
            }
        }
    }
}

int main() {
    int win_id = create_gui_window("Digital Clock", CW + 4, CH + 24);
    if (win_id < 0) return -1;

    unsigned int* my_canvas = (unsigned int*)sbrk(CW * CH * 4);
    int blink = 0;

    while (1) {
        int h, m;
        get_time(&h, &m);

        // 1. 清空畫布 (填滿純黑)
        draw_rect(my_canvas, 0, 0, CW, CH, 0x000000);

        // 2. 畫小時 (十位數與個位數，綠色)
        draw_digit(my_canvas, h / 10, 15, 18, 0x00FF00);
        draw_digit(my_canvas, h % 10, 35, 18, 0x00FF00);

        // 3. 畫中間的冒號 (每秒閃爍一次)
        if (blink % 2 == 0) {
            draw_rect(my_canvas, 56, 23, 4, 4, 0x00FF00);
            draw_rect(my_canvas, 56, 33, 4, 4, 0x00FF00);
        }

        // 4. 畫分鐘
        draw_digit(my_canvas, m / 10, 65, 18, 0x00FF00);
        draw_digit(my_canvas, m % 10, 85, 18, 0x00FF00);

        // 5. 推送給 Window Manager
        update_gui_window(win_id, my_canvas);

        blink++;

        // 粗略等待半秒鐘 (讓冒號閃爍)
        for(volatile int i = 0; i < 3000000; i++);
    }

    return 0;
}
