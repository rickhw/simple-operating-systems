#include "gui.h"
#include "gfx.h"
#include "utils.h"
#include "tty.h"
#include "rtc.h"

#define MAX_WINDOWS 10
#define TERM_BG 0x008080 // 桌面底色

extern void tty_render_window(int win_id);

static window_t windows[MAX_WINDOWS];
static int window_count = 0;
static int last_mouse_x = 400;
static int last_mouse_y = 300;
static int focused_window_id = -1;  // 記錄當前被選中的視窗 (-1 代表沒有)
static int start_menu_open = 0;     // 開始選單是否開啟的狀態
static int gui_mode_enabled = 1;    // 紀錄 gui mode

// 非同步渲染的靈魂：髒標記與渲染鎖！
static volatile int gui_dirty = 1;     // 1 代表畫面需要更新
static volatile int is_rendering = 0;  // 1 代表正在畫圖中，避免重複進入

// 加入 Focus 判斷與 [X] 按鈕
static void draw_window_internal(window_t* win) {
    int is_focused = (focused_window_id == win->id);

    // 視窗主體與邊框
    draw_rect(win->x, win->y, win->width, win->height, 0xC0C0C0);
    draw_rect(win->x, win->y, win->width, 2, 0xFFFFFF);
    draw_rect(win->x, win->y, 2, win->height, 0xFFFFFF);
    draw_rect(win->x, win->y + win->height - 2, win->width, 2, 0x808080);
    draw_rect(win->x + win->width - 2, win->y, 2, win->height, 0x808080);

    // 標題列 (有焦點是深藍色，沒焦點是灰色)
    uint32_t title_bg = is_focused ? 0x000080 : 0x808080;
    draw_rect(win->x + 2, win->y + 2, win->width - 4, 18, title_bg);
    draw_string(win->x + 6, win->y + 7, win->title, 0xFFFFFF, title_bg);

    // 【新增】畫出 [X] 關閉按鈕 (在右上角)
    draw_rect(win->x + win->width - 20, win->y + 4, 14, 14, 0xC0C0C0);
    draw_string(win->x + win->width - 17, win->y + 7, "X", 0x000000, 0xC0C0C0);

    // ==========================================
    // 【核心新增】在這裡渲染視窗專屬的內容！
    // 這樣內容就會完美服從視窗的 Z-Order，不會穿透了！
    // ==========================================
    if (strcmp(win->title, "System Status") == 0) {
        draw_string(win->x + 10, win->y + 30, "CPU: x86 32-bit", 0x000000, 0xC0C0C0);
        draw_string(win->x + 10, win->y + 50, "Memory: 16 MB", 0x000000, 0xC0C0C0);
        draw_string(win->x + 10, win->y + 70, "GUI: Active", 0x000000, 0xC0C0C0);
    }

    // 如果這個視窗有綁定終端機，這行會把它畫出來
    tty_render_window(win->id);
}

static void draw_taskbar(void) {
    int screen_w = 800;
    int screen_h = 600;
    int taskbar_h = 28;
    int taskbar_y = screen_h - taskbar_h;

    // 1. 主底板 (經典亮灰色)
    draw_rect(0, taskbar_y, screen_w, taskbar_h, 0xC0C0C0);
    // 上方的高光白線，營造凸起感
    draw_rect(0, taskbar_y, screen_w, 2, 0xFFFFFF);

    // 2. Start 按鈕 (凸起的 3D 按鈕)
    draw_rect(4, taskbar_y + 4, 60, 20, 0xC0C0C0);
    draw_rect(4, taskbar_y + 4, 60, 2, 0xFFFFFF); // 上亮邊
    draw_rect(4, taskbar_y + 4, 2, 20, 0xFFFFFF); // 左亮邊
    draw_rect(4, taskbar_y + 22, 60, 2, 0x808080); // 下陰影
    draw_rect(62, taskbar_y + 4, 2, 20, 0x808080); // 右陰影
    draw_string(14, taskbar_y + 10, "Start", 0x000000, 0xC0C0C0);

    // 3. 右下角時鐘系統匣 (凹陷的 3D 區域)
    int tray_w = 66;
    int tray_x = screen_w - tray_w - 4;
    draw_rect(tray_x, taskbar_y + 4, tray_w, 20, 0xC0C0C0);
    draw_rect(tray_x, taskbar_y + 4, tray_w, 2, 0x808080);  // 上陰影 (凹陷感)
    draw_rect(tray_x, taskbar_y + 4, 2, 20, 0x808080);  // 左陰影
    draw_rect(tray_x, taskbar_y + 22, tray_w, 2, 0xFFFFFF); // 下亮邊
    draw_rect(tray_x + tray_w - 2, taskbar_y + 4, 2, 20, 0xFFFFFF); // 右亮邊

    // 4. 取得硬體時間並轉成字串
    int h, m;
    read_time(&h, &m);

    char time_str[6] = "00:00";
    time_str[0] = (h / 10) + '0';
    time_str[1] = (h % 10) + '0';
    time_str[3] = (m / 10) + '0';
    time_str[4] = (m % 10) + '0';

    // 畫出時間
    draw_string(tray_x + 10, taskbar_y + 10, time_str, 0x000000, 0xC0C0C0);
}

// 畫出 3D 開始選單
static void draw_start_menu(void) {
    if (!start_menu_open) return;

    int menu_w = 150;
    int menu_h = 130;
    int menu_x = 4;
    int menu_y = 600 - 28 - menu_h; // 貼在工作列正上方 (Y=472)

    // 畫底板與 3D 邊框 (左上亮，右下暗)
    draw_rect(menu_x, menu_y, menu_w, menu_h, 0xC0C0C0);
    draw_rect(menu_x, menu_y, menu_w, 2, 0xFFFFFF);
    draw_rect(menu_x, menu_y, 2, menu_h, 0xFFFFFF);
    draw_rect(menu_x, menu_y + menu_h - 2, menu_w, 2, 0x808080);
    draw_rect(menu_x + menu_w - 2, menu_y, 2, menu_h, 0x808080);

    // 畫選單項目
    draw_string(menu_x + 10, menu_y + 12, "1. Terminal", 0x000000, 0xC0C0C0);
    draw_string(menu_x + 10, menu_y + 42, "2. Sys Status", 0x000000, 0xC0C0C0);
    draw_string(menu_x + 10, menu_y + 72, "3. Shutdown", 0x000000, 0xC0C0C0);
    draw_string(menu_x + 10, menu_y + 102, "4. Exit to CLI", 0x000000, 0xC0C0C0);
}

// 繪製漸層桌面背景
static void draw_desktop_background(void) {
    for (int y = 0; y < 600; y++) {
        // 利用 Y 座標計算顏色的漸變
        // Red 永遠是 0，Green 從 0 漸變到 128，Blue 從 128 漸變到 192
        uint32_t r = 0;
        uint32_t g = (y * 128) / 600;
        uint32_t b = 128 + (y * 64) / 600;

        uint32_t color = (r << 16) | (g << 8) | b;

        // 畫一條橫線 (因為我們只有 draw_rect，高度設為 1 就是一條線)
        draw_rect(0, y, 800, 1, color);
    }
}

// 繪製單一桌面圖示
static void draw_desktop_icon(int x, int y, const char* name, uint32_t icon_color) {
    // 畫一個 32x32 的方形圖示
    draw_rect(x, y, 32, 32, icon_color);
    draw_rect(x + 2, y + 2, 28, 28, 0xFFFFFF); // 白色內框
    draw_rect(x + 6, y + 6, 20, 20, icon_color); // 色塊核心

    // 畫出圖示下方的文字 (為了避免字和漸層背景混在一起，給它加上一點黑色底色)
    // 簡單計算讓文字稍微置中 (假設文字不超過 10 個字)
    int text_x = x - (strlen(name) * 8 - 32) / 2;
    draw_string(text_x, y + 36, name, 0xFFFFFF, 0x000000);
}

// 繪製所有桌面圖示
static void draw_desktop_icons(void) {
    draw_desktop_icon(20, 20, "Terminal", 0x000080); // 深藍色圖示
    draw_desktop_icon(20, 80, "Status", 0x008000);   // 綠色圖示
}

// 【新增】統籌切換模式的核心 API
void switch_display_mode(int is_gui) {
    enable_gui_mode(is_gui);
    terminal_set_mode(is_gui);

    if (is_gui) {
        // 切換回 GUI 時，確保有一個 Terminal 視窗可以接管輸入
        int found = 0;
        window_t* wins = get_windows();
        for (int i = 0; i < 10; i++) { // 假設 MAX_WINDOWS = 10
            if (wins[i].is_active && strcmp(wins[i].title, "Simple OS Terminal") == 0) {
                terminal_bind_window(i);
                set_focused_window(i);
                found = 1;
                break;
            }
        }
        if (!found) {
            int term_win = create_window(50, 50, 368, 228, "Simple OS Terminal", 1); // 預設由 PID#1 建立
            terminal_bind_window(term_win);
        }
    } else {
        // 切換到 CLI 時，解除視窗綁定，讓 TTY 全螢幕輸出
        terminal_bind_window(-1);
    }

    // 強制畫面大更新
    extern void gui_redraw(void);
    gui_redraw();
}

// --- GUI API ----

void init_gui(void) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i].is_active = 0;
    }
}

void gui_render(int mouse_x, int mouse_y) {
    last_mouse_x = mouse_x;
    last_mouse_y = mouse_y;

    // === CLI 模式：只畫純黑底色與全螢幕文字 ===
    if (!gui_mode_enabled) {
        draw_rect(0, 0, 800, 600, 0x000000);
        tty_render_fullscreen();
        gfx_swap_buffers();
        return;
    }

    // 1. 畫上絕美的漸層背景與桌面圖示！
    draw_desktop_background();
    draw_desktop_icons();

    // 2. 先畫「沒有焦點」的活躍視窗
    // draw_rect(0, 0, 800, 600, TERM_BG);
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_active && i != focused_window_id) {
            draw_window_internal(&windows[i]);
        }
    }

    // 3. 最後畫「有焦點」的視窗 (讓它疊在最上層！)
    if (focused_window_id != -1 && windows[focused_window_id].is_active) {
        draw_window_internal(&windows[focused_window_id]);
    }

    // 4. 畫工作列 (Taskbar)
    draw_taskbar();

    // 5. 畫開始選單 (必須疊在所有視窗和工作列上面)
    draw_start_menu();

    // 6. 畫滑鼠游標 (永遠在最最上層)
    draw_cursor(mouse_x, mouse_y);

    // 交換畫布
    gfx_swap_buffers();
}

// 提供給打字機呼叫的重繪函式
// 現在它只做「標記」，0.0001 毫秒就執行完，絕對不卡 CPU！
// ==========================================
void gui_redraw(void) {
    gui_dirty = 1;
}

// 給滑鼠專用的更新函式
void gui_update_mouse(int x, int y) {
    last_mouse_x = x;
    last_mouse_y = y;
    gui_dirty = 1; // 滑鼠動了，畫面也髒了
}

// 真正的渲染引擎，交給系統時鐘來呼叫！(約 60~100 FPS)
void gui_handle_timer(void) {
    // 如果畫面是髒的，而且目前「沒有人正在渲染」
    if (gui_dirty && !is_rendering) {
        is_rendering = 1; // 上鎖 (防止 Timer 中斷重疊引發 Kernel Panic)

        gui_render(last_mouse_x, last_mouse_y);

        gui_dirty = 0;    // 清除髒標記
        is_rendering = 0; // 解鎖
    }
}

// UI 事件分發中心
int gui_check_ui_click(int x, int y) {
    // ==========================================
    // 1. 先檢查「目前有焦點」視窗的 [X]
    // ==========================================
    if (focused_window_id != -1 && windows[focused_window_id].is_active) {
        window_t* win = &windows[focused_window_id];
        int btn_x = win->x + win->width - 20;
        int btn_y = win->y + 4;
        if (x >= btn_x && x <= btn_x + 14 && y >= btn_y && y <= btn_y + 14) {
            int target_pid = win->owner_pid;
            close_window(win->id);
            extern int sys_kill(int pid);
            if (target_pid > 1) { sys_kill(target_pid); }
            gui_dirty = 1;
            return 1;
        }
    }

    // ==========================================
    // 2. 檢查其他視窗的 [X]，或點擊本體來「切換焦點」！
    // ==========================================
    // 從後往前找，符合畫面覆蓋 (Z-Order) 的直覺
    for (int i = MAX_WINDOWS - 1; i >= 0; i--) {
        if (windows[i].is_active && i != focused_window_id) {
            window_t* win = &windows[i];
            int btn_x = win->x + win->width - 20;
            int btn_y = win->y + 4;

            // 如果點擊了 [X]
            if (x >= btn_x && x <= btn_x + 14 && y >= btn_y && y <= btn_y + 14) {
                int target_pid = win->owner_pid;
                close_window(win->id);
                extern int sys_kill(int pid);
                if (target_pid > 1) { sys_kill(target_pid); }
                gui_dirty = 1;
                return 1;
            }

            // 如果點擊了視窗本體 -> 切換焦點並拉到最上層！
            if (x >= win->x && x <= win->x + win->width && y >= win->y && y <= win->y + win->height) {
                set_focused_window(i);
                gui_dirty = 1;
                return 1;
            }
        }
    }


    // ==========================================
    // 0. 檢查是否點擊了桌面圖示
    // ==========================================
    // 點擊 "Terminal" 圖示 (X: 20~52, Y: 20~52)
    if (x >= 20 && x <= 52 && y >= 20 && y <= 52) {
        int offset = window_count * 20;
        int term_win = create_window(50 + offset, 50 + offset, 368, 228, "Simple OS Terminal", 1); // 預設由 PID#1 建立
        terminal_bind_window(term_win);
        gui_dirty = 1;
        return 1;
    }

    // 點擊 "Status" 圖示 (X: 20~52, Y: 80~112)
    if (x >= 20 && x <= 52 && y >= 80 && y <= 112) {
        int offset = window_count * 20;
        create_window(450 - offset, 100 + offset, 300, 200, "System Status", 1); // 預設由 PID#1 建立
        gui_dirty = 1;
        return 1;
    }

    // 1. 檢查是否點擊了左下角的 Start 按鈕 (X: 4~64, Y: 576~596)
    if (x >= 4 && x <= 64 && y >= 576 && y <= 596) {
        start_menu_open = !start_menu_open; // 切換開關
        gui_dirty = 1;
        return 1; // 成功攔截這次點擊
    }

    // 2. 如果選單是開著的，檢查是否點了裡面的選項
    if (start_menu_open) {
        int menu_y = 600 - 28 - 130;

        if (x >= 4 && x <= 154 && y >= menu_y && y <= menu_y + 130) {
            // 點擊 "1. Terminal"
            if (y >= menu_y + 10 && y <= menu_y + 30) {
                // 【核心修正】尋找現有的 Terminal 視窗，把它拉到最上層！
                int found = 0;
                for (int i = 0; i < MAX_WINDOWS; i++) {
                    if (windows[i].is_active && strcmp(windows[i].title, "Simple OS Terminal") == 0) {
                        set_focused_window(i); // 找到了！拉到最上層並給予焦點
                        found = 1;
                        break;
                    }
                }

                // 如果終端機之前被手賤按 [X] 關掉了，我們就在原地幫他把框框畫回來
                // (註：因為 Shell process 其實還活在背景，所以框框畫回來，綁定回去就又看得到了！)
                if (!found) {
                    int term_win = create_window(50, 50, 368, 228, "Simple OS Terminal", 1); // 預設由 PID#1 建立
                    terminal_bind_window(term_win);
                }
            }
            // 點擊 "2. Sys Status"
            else if (y >= menu_y + 40 && y <= menu_y + 60) {
                int found = 0;
                for (int i = 0; i < MAX_WINDOWS; i++) {
                    if (windows[i].is_active && strcmp(windows[i].title, "System Status") == 0) {
                        set_focused_window(i);
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    create_window(450, 100, 300, 200, "System Status", 1); // 預設由 PID#1 建立
                }
            }
            // 點擊 "3. Shutdown"
            else if (y >= menu_y + 70 && y <= menu_y + 90) {
                draw_rect(0, 0, 800, 600, 0x000000); // 假裝關機畫面
                draw_string(230, 280, "It is now safe to turn off your computer.", 0xFF8000, 0x000000);
                gfx_swap_buffers();
                while(1) __asm__ volatile("cli; hlt"); // 凍結系統
            }

            // 點擊 "4. Exit to CLI"
            else if (y >= menu_y + 100 && y <= menu_y + 120) {
                switch_display_mode(0); // 切換為 CLI 模式 (0)
            }

            start_menu_open = 0; // 點完選項後自動收起選單
            extern void gui_redraw(void);
            gui_dirty = 1;
            return 1;
        } else {
            // 點到選單外面的任何地方，就自動收起選單
            start_menu_open = 0;
            gui_dirty = 1;
            // 這裡不 return 1，讓底下的視窗可以繼續處理這個點擊
        }
    }

    return 0; // 不是 UI 事件，放行給視窗處理
}


// ---- Window API ----

int create_window(int x, int y, int width, int height, const char* title, int owner_pid) {
    if (window_count >= MAX_WINDOWS) return -1;
    int id = window_count++;

    windows[id].owner_pid = owner_pid; // 【Day 68 新增】
    windows[id].id = id;
    windows[id].x = x;
    windows[id].y = y;
    windows[id].width = width;
    windows[id].height = height;
    strcpy(windows[id].title, title);
    windows[id].is_active = 1;

    focused_window_id = id; // 新開的視窗預設取得焦點
    return id;
}

void close_window(int id) {
    if (id >= 0 && id < MAX_WINDOWS) {
        windows[id].is_active = 0;
        if (focused_window_id == id) focused_window_id = -1;
    }
}

window_t* get_window(int id) {
    if (id >= 0 && id < MAX_WINDOWS && windows[id].is_active) {
        return &windows[id];
    }
    return 0; // NULL
}

window_t* get_windows(void) { return windows; }

void set_focused_window(int id) { focused_window_id = id; }

int get_focused_window(void) { return focused_window_id; }

// 模式切換開關
void enable_gui_mode(int enable) {
    gui_mode_enabled = enable;
}
