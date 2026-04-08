#ifndef SIMPLEUI_H
#define SIMPLEUI_H

/**
 * @file simpleui.h
 * @brief 使用者界面基礎繪圖與視窗管理介面
 */

// ==========================================
// 標準調色盤 (Standard Palette)
// ==========================================
#define UI_COLOR_WHITE      0xFFFFFF
#define UI_COLOR_BLACK      0x000000
#define UI_COLOR_GRAY       0xC0C0C0
#define UI_COLOR_DARK_GRAY  0x808080
#define UI_COLOR_RED        0xFF0000
#define UI_COLOR_BLUE       0x4169E1
#define UI_COLOR_YELLOW     0xFFD700
#define UI_COLOR_GREEN      0x00FF00

// ==========================================
// 視窗管理 (Window Management)
// ==========================================
int create_gui_window(const char* title, int width, int height);
void update_gui_window(int win_id, unsigned int* buffer);
int get_window_event(int win_id, int* x, int* y);
int get_window_key_event(int win_id, char* key);

// ==========================================
// 繪圖元件與工具 (UI Components & Tools)
// ==========================================

// 標準按鈕結構
typedef struct {
    int x, y, w, h;
    char text[32];
    unsigned int bg_color;
    unsigned int text_color;
    int is_pressed; // 記錄按鈕是否正在被按下 (用來畫 3D 凹陷效果)
} ui_button_t;

/**
 * @brief 畫實心矩形
 */
void ui_draw_rect(unsigned int* canvas, int cw, int ch, int x, int y, int w, int h, unsigned int color);

/**
 * @brief 在畫布上繪製文字 (8x8 字型)
 */
void ui_draw_text(unsigned int* canvas, int cw, int ch, int x, int y, const char* str, unsigned int color);

/**
 * @brief 在畫布上繪製單一數字 (放大版 3x5 字型)
 */
void ui_draw_digit(unsigned int* canvas, int cw, int ch, int digit, int ox, int oy, unsigned int color);

/**
 * @brief 繪製具有 3D 效果的按鈕
 */
void ui_draw_button(unsigned int* canvas, int cw, int ch, ui_button_t* btn);

/**
 * @brief 偵測座標是否落在按鈕範圍內
 */
int ui_is_clicked(ui_button_t* btn, int cx, int cy);

#endif
