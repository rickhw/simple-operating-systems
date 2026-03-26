#ifndef GUI_H
#define GUI_H

#include <stdint.h>

// 定義視窗結構 (就像 2D 遊戲裡的 Entity/Sprite)
typedef struct {
    int id;
    int x;
    int y;
    int width;
    int height;
    char title[32];
    int is_active;
} window_t;

// GUI 系統初始化
void init_gui(void);

// 註冊一個新視窗
int create_window(int x, int y, int width, int height, const char* title);

// 取得全域視窗陣列的指標 (給滑鼠偵測用)
window_t* get_windows(void);

void gui_render(int mouse_x, int mouse_y);

#endif
