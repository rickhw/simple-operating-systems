#ifndef SIMPLEUI_H
#define SIMPLEUI_H

// 標準調色盤
#define UI_COLOR_WHITE      0xFFFFFF
#define UI_COLOR_BLACK      0x000000
#define UI_COLOR_GRAY       0xC0C0C0
#define UI_COLOR_DARK_GRAY  0x808080
#define UI_COLOR_RED        0xFF0000
#define UI_COLOR_BLUE       0x4169E1
#define UI_COLOR_YELLOW     0xFFD700
#define UI_COLOR_GREEN      0x00FF00

// 標準按鈕結構
typedef struct {
    int x, y, w, h;
    char text[32];
    unsigned int bg_color;
    unsigned int text_color;
    int is_pressed; // 記錄按鈕是否正在被按下 (用來畫 3D 凹陷效果)
} ui_button_t;

// 基礎繪圖 API (加上 cw, ch 來避免畫出畫布範圍)
// CW: CANVAS_W
// CH: CANVAS_H
void ui_draw_rect(unsigned int* canvas, int cw, int ch, int x, int y, int w, int h, unsigned int color);
 // void ui_draw_box(unsigned int* canvas, int x, int y, int w, int h, unsigned int color);
void ui_draw_text(unsigned int* canvas, int cw, int ch, int x, int y, const char* str, unsigned int color);

void ui_draw_digit(unsigned int* canvas, int cw, int ch, int digit, int ox, int oy, unsigned int color);

// 元件 API
void ui_draw_button(unsigned int* canvas, int cw, int ch, ui_button_t* btn);
int ui_is_clicked(ui_button_t* btn, int cx, int cy);

#endif
