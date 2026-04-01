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

    // 【Day 76 新增】視窗內部事件暫存區
    int last_click_x;    // 點擊在畫布上的相對 X 座標
    int last_click_y;    // 點擊在畫布上的相對 Y 座標
    int has_new_click;   // 1 代表有新事件尚未被 App 讀取

    // 【Day 77 新增】鍵盤事件暫存區
    char last_key;
    int has_new_key;

    // 【Day 78 新增】視窗狀態與控制
    int is_minimized;  // 1 代表縮小到工作列
    int is_maximized;  // 1 代表最大化
    int orig_x;        // 記憶最大化之前的 X 座標
    int orig_y;        // 記憶最大化之前的 Y 座標
    int orig_w;        // 記憶最大化之前的寬度
    int orig_h;        // 記憶最大化之前的高度
    int z_layer;       // 1 為一般層，0 為置底層
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
void gui_handle_timer(void);

// ---- Window API ----
// 註冊一個新視窗
int create_window(int x, int y, int width, int height, const char* title, int owner_pid);
void close_window(int id);

window_t* get_windows(void);    // 取得全域視窗陣列 (給滑鼠碰撞偵測用)
window_t* get_window(int id);   // 取得單一視窗 (給 TTY 用)

void set_focused_window(int id);
int get_focused_window(void);

void enable_gui_mode(int enable);
void switch_display_mode(int is_gui);

void gui_mouse_down(int x, int y);
void gui_mouse_up(void);

void gui_close_windows_by_pid(int pid);
int gui_handle_keyboard(char c);

#endif
