#include "stdio.h"
#include "unistd.h"
#include "simpleui.h"

#define CANVAS_W 340
#define CANVAS_H 280

int strlen(const char* s) { int i=0; while(s[i]) i++; return i; }
void itoa(int n, char* buf) { /* 保持原樣 */
    if (n == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    int i = 0; while (n > 0) { buf[i++] = (n % 10) + '0'; n /= 10; }
    buf[i] = '\0';
    for (int j = 0; j < i / 2; j++) { char t = buf[j]; buf[j] = buf[i - 1 - j]; buf[i - 1 - j] = t; }
}

int main() {
    int win_id = create_gui_window("Task Manager", CANVAS_W + 4, CANVAS_H + 24);
    if (win_id < 0) return -1;

    unsigned int* my_canvas = (unsigned int*)sbrk(CANVAS_W * CANVAS_H * 4);
    process_info_t p_list[32];
    int refresh_timer = 0;

    while(1) {
        if (refresh_timer % 100 == 0) {
            int p_count = get_process_list(p_list, 32);

            // 使用 SimpleUI 繪圖！
            ui_draw_rect(my_canvas, CANVAS_W, CANVAS_H, 0, 0, CANVAS_W, CANVAS_H, UI_COLOR_WHITE);
            ui_draw_rect(my_canvas, CANVAS_W, CANVAS_H, 0, 0, CANVAS_W, 20, UI_COLOR_BLUE);
            ui_draw_text(my_canvas, CANVAS_W, CANVAS_H, 10, 6, "PID   NAME       MEM (KB)   ACTION", UI_COLOR_WHITE);

            int y_offset = 25;
            for (int i = 0; i < p_count; i++) {
                char buf[16];
                itoa(p_list[i].pid, buf);
                ui_draw_text(my_canvas, CANVAS_W, CANVAS_H, 10, y_offset, buf, UI_COLOR_BLACK);
                ui_draw_text(my_canvas, CANVAS_W, CANVAS_H, 50, y_offset, p_list[i].name, UI_COLOR_BLACK);
                itoa(p_list[i].memory_used / 1024, buf);
                ui_draw_text(my_canvas, CANVAS_W, CANVAS_H, 140, y_offset, buf, UI_COLOR_BLACK);

                // ==========================================
                // 使用 SimpleUI 的立體按鈕元件！
                // ==========================================
                if (p_list[i].pid > 1 && strcmp(p_list[i].name, "taskmgr.elf") != 0) {
                    ui_button_t kill_btn = {
                        .x = 250, .y = y_offset - 2, .w = 48, .h = 14,
                        .bg_color = UI_COLOR_RED, .text_color = UI_COLOR_WHITE, .is_pressed = 0
                    };
                    // 把 "KILL" 複製進按鈕文字
                    char* src = "KILL"; char* dst = kill_btn.text;
                    while(*src) *dst++ = *src++; *dst = '\0';

                    ui_draw_button(my_canvas, CANVAS_W, CANVAS_H, &kill_btn);
                }

                y_offset += 20;
                if (y_offset > CANVAS_H - 20) break;
            }
            update_gui_window(win_id, my_canvas);
        }
        refresh_timer++;

        // 處理 IPC 點擊事件
        int cx, cy;
        if (get_window_event(win_id, &cx, &cy)) {
            if (cy >= 25) {
                int clicked_row = (cy - 25) / 20;

                // 改用按鈕物件的邏輯來檢查碰撞 (這裡簡化為直接套用座標區間)
                ui_button_t dummy_btn = { .x = 250, .y = 25 + clicked_row * 20 - 2, .w = 48, .h = 14 };

                if (ui_is_clicked(&dummy_btn, cx, cy)) {
                    int p_count = get_process_list(p_list, 32);
                    if (clicked_row >= 0 && clicked_row < p_count) {
                        int target_pid = p_list[clicked_row].pid;
                        if (target_pid > 1 && strcmp(p_list[clicked_row].name, "taskmgr.elf") != 0) {
                            kill(target_pid);
                            refresh_timer = 0;
                        }
                    }
                }
            }
        }
        yield();
    }
    return 0;
}
