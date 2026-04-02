#include "stdio.h"
#include "unistd.h"

#define CANVAS_W 280
#define CANVAS_H 550

// 簡單的字串處理工具
int strlen(const char* s) { int i=0; while(s[i]) i++; return i; }
void strcpy(char* dest, const char* src) { while(*src) *dest++ = *src++; *dest = '\0'; }

// 專為 File Explorer 打造的微型 8x8 字型庫 (包含 a-z, A-Z, 0-9, 與常用符號)
static const unsigned char font[128][8] = {
    ['a']={0x00,0x3c,0x06,0x3e,0x66,0x3e,0x00}, ['b']={0x60,0x7c,0x66,0x66,0x66,0x7c,0x00},
    ['c']={0x00,0x3c,0x60,0x60,0x60,0x3c,0x00}, ['d']={0x06,0x3e,0x66,0x66,0x66,0x3e,0x00},
    ['e']={0x00,0x3c,0x7e,0x60,0x60,0x3c,0x00}, ['f']={0x1c,0x30,0x7c,0x30,0x30,0x30,0x00},
    ['g']={0x00,0x3e,0x66,0x66,0x3e,0x06,0x3c}, ['h']={0x60,0x7c,0x66,0x66,0x66,0x66,0x00},
    ['i']={0x18,0x00,0x38,0x18,0x18,0x3c,0x00}, ['j']={0x0c,0x00,0x1c,0x0c,0x0c,0x6c,0x38},
    ['k']={0x60,0x66,0x6c,0x78,0x6c,0x66,0x00}, ['l']={0x38,0x18,0x18,0x18,0x18,0x3c,0x00},
    ['m']={0x00,0xec,0xfe,0xd6,0xd6,0xd6,0x00}, ['n']={0x00,0x7c,0x66,0x66,0x66,0x66,0x00},
    ['o']={0x00,0x3c,0x66,0x66,0x66,0x3c,0x00}, ['p']={0x00,0x7c,0x66,0x66,0x7c,0x60,0x60},
    ['q']={0x00,0x3e,0x66,0x66,0x3e,0x06,0x06}, ['r']={0x00,0x5c,0x66,0x60,0x60,0x60,0x00},
    ['s']={0x00,0x3e,0x60,0x3c,0x06,0x7c,0x00}, ['t']={0x30,0x7c,0x30,0x30,0x30,0x1c,0x00},
    ['u']={0x00,0x66,0x66,0x66,0x66,0x3e,0x00}, ['v']={0x00,0x66,0x66,0x66,0x3c,0x18,0x00},
    ['w']={0x00,0xd6,0xd6,0xd6,0xfe,0x6c,0x00}, ['x']={0x00,0x66,0x3c,0x18,0x3c,0x66,0x00},
    ['y']={0x00,0x66,0x66,0x66,0x3e,0x06,0x3c}, ['z']={0x00,0x7e,0x0c,0x18,0x30,0x7e,0x00},
    ['0']={0x3c,0x66,0x6e,0x76,0x66,0x3c,0x00}, ['1']={0x18,0x38,0x18,0x18,0x18,0x3c,0x00},
    ['2']={0x3c,0x66,0x0c,0x18,0x30,0x7e,0x00}, ['3']={0x3c,0x66,0x1c,0x06,0x66,0x3c,0x00},
    ['4']={0x0c,0x1c,0x3c,0x6c,0x7e,0x0c,0x00}, ['5']={0x7e,0x60,0x7c,0x06,0x66,0x3c,0x00},
    ['6']={0x3c,0x60,0x7c,0x66,0x66,0x3c,0x00}, ['7']={0x7e,0x06,0x0c,0x18,0x30,0x30,0x00},
    ['8']={0x3c,0x66,0x3c,0x66,0x66,0x3c,0x00}, ['9']={0x3c,0x66,0x66,0x3e,0x06,0x3c,0x00},
    ['.']={0x00,0x00,0x00,0x00,0x18,0x18,0x00}, ['_']={0x00,0x00,0x00,0x00,0x00,0x7e,0x00},
    ['-']={0x00,0x00,0x00,0x3c,0x00,0x00,0x00}, [' ']={0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['A']={0x3c,0x66,0x66,0x7e,0x66,0x66,0x00}, ['C']={0x3c,0x66,0x60,0x60,0x66,0x3c,0x00},
    ['S']={0x3c,0x60,0x3c,0x06,0x66,0x3c,0x00}, ['/']={0x06,0x0c,0x18,0x30,0x60,0xc0,0x00},
    [':']={0x00,0x18,0x00,0x00,0x18,0x00,0x00}, ['\\']={0x60,0x30,0x18,0x0c,0x06,0x03,0x00}
};

// 畫矩形色塊
void draw_box(unsigned int* canvas, int x, int y, int w, int h, unsigned int color) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            if (x + j < CANVAS_W && y + i < CANVAS_H) {
                canvas[(y + i) * CANVAS_W + (x + j)] = color;
            }
        }
    }
}

// 利用 8x8 字型庫畫字串
void draw_text(unsigned int* canvas, int x, int y, const char* str, unsigned int color) {
    while (*str) {
        unsigned char c = *str++;

        // ==========================================
        // 【Day 75 修正】如果遇到沒有實作的大寫字母，自動降級成小寫！
        // 這樣 F 就不會變成空白了！
        // ==========================================
        if (c >= 'A' && c <= 'Z') {
            // 簡單檢查字型陣列是不是空的 (0x00)
            if (font[c][0] == 0 && font[c][1] == 0) {
                c += 32; // ASCII 大小寫剛好差 32
            }
        }

        for (int r = 0; r < 8; r++) {
            for (int c_bit = 0; c_bit < 8; c_bit++) {
                if (font[c][r] & (1 << (7 - c_bit))) {
                    int px = x + c_bit;
                    int py = y + r;
                    if (px < CANVAS_W && py < CANVAS_H) {
                        canvas[py * CANVAS_W + px] = color;
                    }
                }
            }
        }
        x += 8; // 推進游標到下一個字
    }
}

int main() {
    int win_id = create_gui_window("File Explorer", CANVAS_W + 4, CANVAS_H + 24);
    if (win_id < 0) return -1;

    unsigned int* my_canvas = (unsigned int*)sbrk(CANVAS_W * CANVAS_H * 4);

    draw_box(my_canvas, 0, 0, CANVAS_W, CANVAS_H, 0xFFFFFF);
    draw_box(my_canvas, 0, 0, CANVAS_W, 24, 0xE0E0E0);
    draw_box(my_canvas, 0, 24, CANVAS_W, 2, 0x808080);
    draw_text(my_canvas, 10, 8, "Path: /", 0x000000);

    // ==========================================
    // 【Day 76 新增】建立陣列，記住所有掃描到的檔案
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
            draw_box(my_canvas, 10, y_offset, 16, 12, 0xFFD700);
            draw_box(my_canvas, 10, y_offset - 2, 6, 2, 0xFFD700);
        } else {
            draw_box(my_canvas, 10, y_offset, 12, 16, 0x4169E1);
            draw_box(my_canvas, 12, y_offset + 2, 8, 12, 0xFFFFFF);
        }

        draw_text(my_canvas, 32, y_offset + 4, name, 0x000000);

        y_offset += 24;
        if (y_offset > CANVAS_H - 24) break;
    }

    update_gui_window(win_id, my_canvas);

    // ==========================================
    // 【Day 76 新增】動態事件迴圈 (Event Loop)
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
