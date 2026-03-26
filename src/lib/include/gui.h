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

// 取得單一視窗 (給 TTY 用)
window_t* get_window(int id);

// 【加回來】取得全域視窗陣列 (給滑鼠碰撞偵測用)
window_t* get_windows(void);

void gui_render(int mouse_x, int mouse_y);
void gui_redraw(void); // 【新增】不改變滑鼠座標的原地重繪

// 【Day 56 新增】焦點與關閉控制
void set_focused_window(int id);
int get_focused_window(void);
void close_window(int id);

// 【新增】非同步渲染架構 API
void gui_handle_timer(void);
void gui_update_mouse(int x, int y);

// 檢查是否點擊了系統 UI，回傳 1 代表已處理，0 代表未攔截
int gui_check_ui_click(int x, int y);

#endif
