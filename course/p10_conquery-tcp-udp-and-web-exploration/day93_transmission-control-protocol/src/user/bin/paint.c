#include "stdio.h"
#include "unistd.h"
#include "simpleui.h"

#define CANVAS_W 200
#define CANVAS_H 150

int main() {
    int win_id = create_gui_window("Dynamic Paint", CANVAS_W + 4, CANVAS_H + 24);
    if (win_id < 0) {
        printf("Failed to create window.\n");
        return -1;
    }

    // ==========================================
    // 【修改】向系統申請 Heap 記憶體 (sbrk)
    // 絕對不要把 117KB 的大傢伙塞在 4KB 的 Stack 裡！
    // ==========================================
    unsigned int* my_canvas = (unsigned int*)sbrk(CANVAS_W * CANVAS_H * 4);

    int offset = 0;

    while (1) {
        // 1. 在畫布上作畫 (產生會動的彩色漸層 / XOR 紋理)
        for (int y = 0; y < CANVAS_H; y++) {
            for (int x = 0; x < CANVAS_W; x++) {
                unsigned int r = (x + offset) % 255;
                unsigned int g = (y + offset) % 255;
                unsigned int b = (x ^ y) % 255;

                my_canvas[y * CANVAS_W + x] = (r << 16) | (g << 8) | b;
            }
        }

        // 2. 把畫布交給 Kernel 去更新螢幕！
        update_gui_window(win_id, my_canvas);

        // 3. 推進動畫影格，並稍微讓出 CPU 休息一下
        offset += 3;
        for(volatile int i = 0; i < 500000; i++);
    }

    return 0;
}
