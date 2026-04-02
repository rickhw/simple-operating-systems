/**
 * @file src/user/bin/notepad.c
 * @brief Main logic and program flow for notepad.c.
 *
 * This file handles the operations and logic associated with notepad.c.
 */

#include "stdio.h"
#include "unistd.h"
#include "simpleui.h"

#define CANVAS_W 320
#define CANVAS_H 240


int main() {
    int win_id = create_gui_window("Simple Notepad", CANVAS_W + 4, CANVAS_H + 24);
    if (win_id < 0) return -1;

    unsigned int* my_canvas = (unsigned int*)sbrk(CANVAS_W * CANVAS_H * 4);

    char text_buffer[1024]; // 文章暫存區
    int text_len = 0;
    text_buffer[0] = '\0';

    int cursor_blink = 0;

    while (1) {
        char key;
        int needs_redraw = 0;

        // 1. 檢查是否有鍵盤輸入！
        if (get_window_key_event(win_id, &key)) {
            if (key == '\b' && text_len > 0) {
                // 倒退鍵
                text_len--;
                text_buffer[text_len] = '\0';
            }
            else if (key != '\b' && text_len < 1023) {
                // 一般字元輸入
                text_buffer[text_len++] = key;
                text_buffer[text_len] = '\0';
            }
            needs_redraw = 1;
            cursor_blink = 1; // 打字時游標強制顯示
        }

        // 2. 消耗掉滑鼠點擊 (雖然我們沒有用到，但要把事件吃掉避免卡住)
        int cx, cy;
        get_window_event(win_id, &cx, &cy);

        // 3. 處理游標閃爍 (每跑幾次迴圈切換一次)
        static int timer = 0;
        if (timer++ % 50 == 0) {
            cursor_blink = !cursor_blink;
            needs_redraw = 1;
        }

        // 4. 重繪畫面
        if (needs_redraw) {
            ui_draw_rect(my_canvas, CANVAS_W, CANVAS_H, 0, 0, CANVAS_W, CANVAS_H, UI_COLOR_WHITE);

            int draw_x = 5;
            int draw_y = 5;

            // 支援換行的繪製邏輯
            for (int i = 0; i < text_len; i++) {
                if (text_buffer[i] == '\n') {
                    draw_x = 5;
                    draw_y += 12; // 換行往下推
                } else {
                    char tmp[2] = {text_buffer[i], '\0'};
                    ui_draw_text(my_canvas, CANVAS_W, CANVAS_H, draw_x, draw_y, tmp, UI_COLOR_BLACK);
                    draw_x += 8; // 字元寬度

                    // 自動換行 (Word Wrap)
                    if (draw_x > CANVAS_W - 10) {
                        draw_x = 5;
                        draw_y += 12;
                    }
                }
            }

            // 畫出閃爍的游標 (一條底線)
            if (cursor_blink) {
                ui_draw_rect(my_canvas, CANVAS_W, CANVAS_H, draw_x, draw_y + 8, 8, 2, UI_COLOR_BLACK);
            }

            update_gui_window(win_id, my_canvas);
        }

        for(volatile int i=0; i<10000; i++); // 控制刷新率
        yield();
    }
    return 0;
}
