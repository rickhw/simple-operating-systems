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
    int owner_pid; // 【Day 68 新增】視窗背後的靈魂 (PID)
    uint32_t* canvas; // 【Day 69 新增】視窗專屬的像素緩衝區
} window_t;


// --- GUI API ----

void init_gui(void);
void gui_render(int mouse_x, int mouse_y);
void gui_redraw(void); // 不改變滑鼠座標的原地重繪

// 非同步渲染架構 API
void gui_handle_timer(void);
void gui_update_mouse(int x, int y);

// 檢查是否點擊了系統 UI，回傳 1 代表已處理，0 代表未攔截
int gui_check_ui_click(int x, int y);


// ---- Window API ----
// 註冊一個新視窗
int create_window(int x, int y, int width, int height, const char* title, int owner_pid);
void close_window(int id);

window_t* get_windows(void);    // 取得全域視窗陣列 (給滑鼠碰撞偵測用)
window_t* get_window(int id);   // 取得單一視窗 (給 TTY 用)

void set_focused_window(int id);
int get_focused_window(void);

void enable_gui_mode(int enable);

#endif
