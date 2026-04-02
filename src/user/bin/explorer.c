#include "stdio.h"
#include "unistd.h"
#include "simpleui.h"

#define CANVAS_W 280
#define CANVAS_H 550

// 簡單的字串處理工具
int strlen(const char* s) { int i=0; while(s[i]) i++; return i; }
void strcpy(char* dest, const char* src) { while(*src) *dest++ = *src++; *dest = '\0'; }

int main() {
    int win_id = create_gui_window("File Explorer", CANVAS_W + 4, CANVAS_H + 24);
    if (win_id < 0) return -1;

    unsigned int* my_canvas = (unsigned int*)sbrk(CANVAS_W * CANVAS_H * 4);

    ui_draw_rect(my_canvas, CANVAS_W, CANVAS_H, 0, 0, CANVAS_W, CANVAS_H, UI_COLOR_WHITE);
    ui_draw_rect(my_canvas, CANVAS_W, CANVAS_H, 0, 0, CANVAS_W, 24, 0xE0E0E0);
    ui_draw_rect(my_canvas, CANVAS_W, CANVAS_H, 0, 24, CANVAS_W, 2, UI_COLOR_DARK_GRAY);
    ui_draw_text(my_canvas, CANVAS_W, CANVAS_H, 10, 8, "Path: /", UI_COLOR_BLACK);

    // ==========================================
    // 建立陣列，記住所有掃描到的檔案
    // ==========================================
    char file_names[64][32];
    int total_files = 0;

    int y_offset = 35;
    char name[32];
    int size, type;

    for (int index = 0; index < 64; index++) {
        name[0] = '\0';
        if (readdir(index, name, &size, &type) == -1) break;
        if (name[0] == '\0') continue;

        // 把檔名存進陣列，方便等一下點擊時查表！
        strcpy(file_names[total_files], name);
        total_files++;

        if (type == 2) {
            ui_draw_rect(my_canvas, CANVAS_W, CANVAS_H, 10, y_offset, 16, 12, 0xFFD700);
            ui_draw_rect(my_canvas, CANVAS_W, CANVAS_H, 10, y_offset - 2, 6, 2, 0xFFD700);
        } else {
            ui_draw_rect(my_canvas, CANVAS_W, CANVAS_H, 10, y_offset, 12, 16, 0x4169E1);
            ui_draw_rect(my_canvas, CANVAS_W, CANVAS_H, 12, y_offset + 2, 8, 12, 0xFFFFFF);
        }

        ui_draw_text(my_canvas, CANVAS_W, CANVAS_H, 32, y_offset + 4, name, 0x000000);

        y_offset += 24;
        if (y_offset > CANVAS_H - 24) break;
    }

    update_gui_window(win_id, my_canvas);

    // ==========================================
    // 動態事件迴圈 (Event Loop)
    // ==========================================
    while(1) {
        int click_x, click_y;

        // 檢查 Kernel 有沒有傳來滑鼠點擊事件
        if (get_window_event(win_id, &click_x, &click_y)) {

            // 檢查點擊的 Y 座標是否落在檔案列表區
            if (click_y >= 35 && click_y < y_offset) {
                // 反推是點到了陣列中的第幾個檔案 (每個檔案佔 24px)
                int clicked_idx = (click_y - 35) / 24;

                if (clicked_idx >= 0 && clicked_idx < total_files) {
                    char* target_file = file_names[clicked_idx];

                    // 為了視覺回饋，在 Terminal 印出點擊了什麼
                    printf("[Explorer] Opening: %s\n", target_file);

                    // 判斷是不是執行檔 (.elf)
                    int len = strlen(target_file);
                    if (len > 4 && target_file[len-4]=='.' && target_file[len-3]=='e' &&
                        target_file[len-2]=='l' && target_file[len-1]=='f') {

                        // 呼叫偉大的 fork + exec！在背景啟動它！
                        int pid = fork();
                        if (pid == 0) {
                            char* args[] = {target_file, 0};
                            exec(target_file, args);
                            exit();
                        }
                    }
                }
            }
        }

        yield(); // 避免占用 CPU
    }

    return 0;
}
