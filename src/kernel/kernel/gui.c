/**
 * @file src/kernel/kernel/gui.c
 * @brief Main logic and program flow for gui.c.
 *
 * This file handles the operations and logic associated with gui.c.
 */

#include "gui.h"
#include "gfx.h"
#include "utils.h"
#include "tty.h"
#include "rtc.h"
#include "kheap.h"
#include "config.h"
#include "task.h"

extern void tty_render_window(int win_id);
extern volatile uint32_t tick;  // 宣告 timer 的系統滴答數

static window_t windows[MAX_WINDOWS];
static int window_count = 0;
static int last_mouse_x = SCREEN_WIDTH / 2;
static int last_mouse_y = SCREEN_HEIGHT / 2;
static int focused_window_id = -1;  // 記錄當前被選中的視窗 (-1 代表沒有)
static int start_menu_open = 0;     // 開始選單是否開啟的狀態
static int gui_mode_enabled = 1;    // 紀錄 gui mode

// 視窗拖曳狀態追蹤
// ==========================================
static int dragged_window_id = -1; // 目前正在被拖曳的視窗 (-1 代表沒有)
static int drag_offset_x = 0;      // 滑鼠點擊位置與視窗左上角的 X 距離
static int drag_offset_y = 0;      // 滑鼠點擊位置與視窗左上角的 Y 距離

// 非同步渲染的靈魂：髒標記與渲染鎖！
static volatile int gui_dirty = 1;     // 1 代表畫面需要更新
static volatile int is_rendering = 0;  // 1 代表正在畫圖中，避免重複進入

// --- UI Interaction Helpers ---

// 加入 Focus 判斷與 [X] 按鈕
static void draw_window_internal(window_t* win) {
    if (win->is_minimized) return; //  最小化的視窗不畫本體！

    int is_focused = (focused_window_id == win->id);

    // 視窗主體與邊框
    draw_rect(win->x, win->y, win->width, win->height, COLOR_WINDOW_BG);
    draw_rect(win->x, win->y, win->width, 2, COLOR_WHITE);
    draw_rect(win->x, win->y, 2, win->height, COLOR_WHITE);
    draw_rect(win->x, win->y + win->height - 2, win->width, 2, COLOR_DARK_GRAY);
    draw_rect(win->x + win->width - 2, win->y, 2, win->height, COLOR_DARK_GRAY);

    // 標題列 (有焦點是深藍色，沒焦點是灰色)
    uint32_t title_bg = is_focused ? COLOR_TITLE_ACTIVE : COLOR_TITLE_INACTIVE;
    draw_rect(win->x + 2, win->y + 2, win->width - 4, TITLE_BAR_HEIGHT, title_bg);
    draw_string(win->x + 6, win->y + 7, win->title, COLOR_WHITE, title_bg);

    // 畫出 [X] 關閉按鈕 (在右上角)
    draw_rect(win->x + win->width - 20, win->y + 4, 14, 14, COLOR_WINDOW_BG);
    draw_string(win->x + win->width - 17, win->y + 7, "X", COLOR_BLACK, COLOR_WINDOW_BG);

    // 畫出四大控制按鈕
    int btn_y = win->y + 4;
    int base_x = win->x + win->width;

    // [v] 置底 (Send to Bottom)
    draw_rect(base_x - 68, btn_y, 14, 14, COLOR_WINDOW_BG);
    draw_string(base_x - 66, btn_y + 3, "v", COLOR_BLACK, COLOR_WINDOW_BG);

    // [_] 最小化 (Minimize)
    draw_rect(base_x - 52, btn_y, 14, 14, COLOR_WINDOW_BG);
    draw_string(base_x - 50, btn_y + 3, "_", COLOR_BLACK, COLOR_WINDOW_BG);

    // [O] 最大化 (Maximize)
    draw_rect(base_x - 36, btn_y, 14, 14, COLOR_WINDOW_BG);
    draw_string(base_x - 34, btn_y + 3, "O", COLOR_BLACK, COLOR_WINDOW_BG);

    // [X] 關閉 (Close)
    draw_rect(base_x - 20, btn_y, 14, 14, COLOR_WINDOW_BG);
    draw_string(base_x - 17, btn_y + 3, "X", COLOR_BLACK, COLOR_WINDOW_BG);


    // 把畫布渲染移到「底層」，當作背景！
    if (win->canvas != 0) {
        int start_x = win->x + 2;
        int start_y = win->y + 22;
        int canvas_w = win->width - 4;
        int canvas_h = win->height - 24;

        for (int cy = 0; cy < canvas_h; cy++) {
            for (int cx = 0; cx < canvas_w; cx++) {
                uint32_t color = win->canvas[cy * canvas_w + cx];
                draw_rect(start_x + cx, start_y + cy, 1, 1, color);
            }
        }
    }

    // Kernel 原生內容蓋在畫布的最上層！
    if (strcmp(win->title, "System Status") == 0) {
        draw_string(win->x + 10, win->y + 30, "CPU: x86 32-bit", 0x000000, 0xC0C0C0);
        draw_string(win->x + 10, win->y + 50, "Memory: 16 MB", 0x000000, 0xC0C0C0);
        draw_string(win->x + 10, win->y + 70, "GUI: Active", 0x000000, 0xC0C0C0);
    }

    // 如果這個視窗有綁定終端機，這行會把它畫出來
    tty_render_window(win->id);
}

static void draw_taskbar(void) {
    int screen_w = SCREEN_WIDTH;
    int screen_h = SCREEN_HEIGHT;
    int taskbar_h = TASKBAR_HEIGHT;
    int taskbar_y = screen_h - taskbar_h;

    // 1. 主底板 (經典亮灰色)
    draw_rect(0, taskbar_y, screen_w, taskbar_h, COLOR_WINDOW_BG);
    // 上方的高光白線，營造凸起感
    draw_rect(0, taskbar_y, screen_w, 2, COLOR_WHITE);

    // 2. Start 按鈕 (凸起的 3D 按鈕)
    draw_rect(4, taskbar_y + 4, 60, 20, COLOR_WINDOW_BG);
    draw_rect(4, taskbar_y + 4, 60, 2, COLOR_WHITE); // 上亮邊
    draw_rect(4, taskbar_y + 4, 2, 20, COLOR_WHITE); // 左亮邊
    draw_rect(4, taskbar_y + 22, 60, 2, COLOR_DARK_GRAY); // 下陰影
    draw_rect(62, taskbar_y + 4, 2, 20, COLOR_DARK_GRAY); // 右陰影
    draw_string(14, taskbar_y + 10, "Start", COLOR_BLACK, COLOR_WINDOW_BG);

    // 3. 右下角時鐘系統匣 (凹陷的 3D 區域)
    int tray_w = 66;
    int tray_x = screen_w - tray_w - 4;
    draw_rect(tray_x, taskbar_y + 4, tray_w, 20, COLOR_WINDOW_BG);
    draw_rect(tray_x, taskbar_y + 4, tray_w, 2, COLOR_DARK_GRAY);  // 上陰影 (凹陷感)
    draw_rect(tray_x, taskbar_y + 4, 2, 20, COLOR_DARK_GRAY);  // 左陰影
    draw_rect(tray_x, taskbar_y + 22, tray_w, 2, COLOR_WHITE); // 下亮邊
    draw_rect(tray_x + tray_w - 2, taskbar_y + 4, 2, 20, COLOR_WHITE); // 右亮邊

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

    // ==========================================
    // 畫出最小化視窗的頁籤
    // ==========================================
    int task_x = 70; // 緊接在 Start 按鈕右邊
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_active && windows[i].is_minimized) {
            // 畫一個凸起的頁籤
            draw_rect(task_x, taskbar_y + 4, 80, 20, COLOR_WINDOW_BG);
            draw_rect(task_x, taskbar_y + 4, 80, 2, COLOR_WHITE);
            draw_rect(task_x, taskbar_y + 4, 2, 20, COLOR_WHITE);
            draw_rect(task_x, taskbar_y + 22, 80, 2, COLOR_DARK_GRAY);
            draw_rect(task_x + 78, taskbar_y + 4, 2, 20, COLOR_DARK_GRAY);

            // 印出前幾個字的標題
            char short_title[10];
            strncpy(short_title, windows[i].title, 9);
            short_title[9] = '\0';
            draw_string(task_x + 6, taskbar_y + 10, short_title, COLOR_BLACK, COLOR_WINDOW_BG);

            task_x += 84; // 下一個頁籤往右排
        }
    }
}

// 畫出 3D 開始選單
static void draw_start_menu(void) {
    if (!start_menu_open) return;

    int menu_w = START_MENU_WIDTH;
    int menu_h = START_MENU_HEIGHT;
    int menu_x = 4;
    int menu_y = SCREEN_HEIGHT - TASKBAR_HEIGHT - menu_h; // 貼在工作列正上方

    // 畫底板與 3D 邊框 (左上亮，右下暗)
    draw_rect(menu_x, menu_y, menu_w, menu_h, COLOR_WINDOW_BG);
    draw_rect(menu_x, menu_y, menu_w, 2, COLOR_WHITE);
    draw_rect(menu_x, menu_y, 2, menu_h, COLOR_WHITE);
    draw_rect(menu_x, menu_y + menu_h - 2, menu_w, 2, COLOR_DARK_GRAY);
    draw_rect(menu_x + menu_w - 2, menu_y, 2, menu_h, COLOR_DARK_GRAY);

    // 畫選單項目
    draw_string(menu_x + 10, menu_y + 12, "1. Terminal", COLOR_BLACK, COLOR_WINDOW_BG);
    draw_string(menu_x + 10, menu_y + 42, "2. Sys Status", COLOR_BLACK, COLOR_WINDOW_BG);
    draw_string(menu_x + 10, menu_y + 72, "3. Shutdown", COLOR_BLACK, COLOR_WINDOW_BG);
    draw_string(menu_x + 10, menu_y + 102, "4. Exit to CLI", COLOR_BLACK, COLOR_WINDOW_BG);
}

// 繪製漸層桌面背景
static void draw_desktop_background(void) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        // 利用 Y 座標計算顏色的漸變
        // Red 永遠是 0，Green 從 0 漸變到 128，Blue 從 128 漸變到 192
        uint32_t r = 0;
        uint32_t g = (y * 128) / SCREEN_HEIGHT;
        uint32_t b = 128 + (y * 64) / SCREEN_HEIGHT;

        uint32_t color = (r << 16) | (g << 8) | b;

        // 畫一條橫線 (因為我們只有 draw_rect，高度設為 1 就是一條線)
        draw_rect(0, y, SCREEN_WIDTH, 1, color);
    }
}

// 繪製單一桌面圖示
static void draw_desktop_icon(int x, int y, const char* name, uint32_t icon_color) {
    // 畫一個 32x32 的方形圖示
    draw_rect(x, y, 32, 32, icon_color);
    draw_rect(x + 2, y + 2, 28, 28, COLOR_WHITE); // 白色內框
    draw_rect(x + 6, y + 6, 20, 20, icon_color); // 色塊核心

    // 畫出圖示下方的文字 (為了避免字和漸層背景混在一起，給它加上一點黑色底色)
    // 簡單計算讓文字稍微置中 (假設文字不超過 10 個字)
    int text_x = x - (strlen(name) * 8 - 32) / 2;
    draw_string(text_x, y + 36, name, COLOR_WHITE, COLOR_BLACK);
}

// 繪製所有桌面圖示
static void draw_desktop_icons(void) {
    draw_desktop_icon(20, 20, "Terminal", 0x000080); // 深藍色圖示
    draw_desktop_icon(20, 80, "Status", 0x008000);   // 綠色圖示
}


// ==========================================
// 統一的 Z-Order UI 事件分發中心
// 規則：從最上層一路往下檢查，只要命中就「吞掉」事件並 Return 1！
// ==========================================
static int handle_start_menu_click(int x, int y) {
    if (!start_menu_open) return 0;

    int menu_y = SCREEN_HEIGHT - TASKBAR_HEIGHT - START_MENU_HEIGHT;
    if (x >= 4 && x <= 4 + START_MENU_WIDTH && y >= menu_y && y <= menu_y + START_MENU_HEIGHT) {
        if (y >= menu_y + 10 && y <= menu_y + 30) {
            // 點擊 "1. Terminal"
            int found = 0;
            for (int i = 0; i < MAX_WINDOWS; i++) {
                if (windows[i].is_active && strcmp(windows[i].title, "Simple OS Terminal") == 0) {
                    set_focused_window(i); found = 1; break;
                }
            }
            if (!found) {
                int term_win = create_window(50, 50, 368, 228, "Simple OS Terminal", 1);
                terminal_bind_window(term_win);
            }
        } else if (y >= menu_y + 40 && y <= menu_y + 60) {
            // 點擊 "2. Sys Status"
            int found = 0;
            for (int i = 0; i < MAX_WINDOWS; i++) {
                if (windows[i].is_active && strcmp(windows[i].title, "System Status") == 0) {
                    set_focused_window(i); found = 1; break;
                }
            }
            if (!found) create_window(450, 100, 300, 200, "System Status", 1);
        } else if (y >= menu_y + 70 && y <= menu_y + 90) {
            // 點擊 "3. Shutdown"
            draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BLACK);
            draw_string(230, 280, "It is now safe to turn off your computer.", 0xFF8000, COLOR_BLACK);
            gfx_swap_buffers();
            while(1) __asm__ volatile("cli; hlt");
        } else if (y >= menu_y + 100 && y <= menu_y + 120) {
            // 點擊 "4. Exit to CLI"
            switch_display_mode(0);
        }
        start_menu_open = 0;
        gui_dirty = 1;
        return 1;
    }

    start_menu_open = 0;
    gui_dirty = 1;
    return 1;
}

static int handle_window_click(int x, int y) {
    // ==========================================
    // 1. 優先檢查擁有焦點的視窗 (Top Window)
    // ==========================================
    // 【重要修復】加入 !windows[...].is_minimized，避免點到隱形的視窗
    if (focused_window_id != -1 && windows[focused_window_id].is_active && !windows[focused_window_id].is_minimized) {
        window_t* win = &windows[focused_window_id];

        if (x >= win->x && x <= win->x + win->width && y >= win->y && y <= win->y + win->height) {

            int base_x = win->x + win->width; // 以視窗右邊界為基準往回算

            // 判斷是否點擊在四大按鈕的 Y 軸範圍內
            if (y >= win->y + 4 && y <= win->y + 18) {
                if (x >= base_x - 20 && x <= base_x - 6) {
                    // 1. [X] 關閉
                    int target_pid = win->owner_pid;
                    close_window(win->id);
                    if (target_pid > 1) { sys_kill(target_pid); }
                }
                else if (x >= base_x - 36 && x <= base_x - 22) {
                    // 2. [O] 最大化 / 還原
                    if (win->is_maximized) {
                        win->x = win->orig_x; win->y = win->orig_y;
                        win->width = win->orig_w; win->height = win->orig_h;
                        win->is_maximized = 0;
                    } else {
                        win->orig_x = win->x; win->orig_y = win->y;
                        win->orig_w = win->width; win->orig_h = win->height;
                        win->x = 0; win->y = 0;
                        win->width = SCREEN_WIDTH;
                        win->height = SCREEN_HEIGHT - TASKBAR_HEIGHT;
                        win->is_maximized = 1;
                    }
                }
                else if (x >= base_x - 52 && x <= base_x - 38) {
                    // 3. [_] 最小化
                    win->is_minimized = 1;
                    if (focused_window_id == win->id) focused_window_id = -1; // 取消焦點
                }
                else if (x >= base_x - 68 && x <= base_x - 54) {
                    // 4. [v] 置底
                    win->z_layer = 0; // 丟入置底層
                    if (focused_window_id == win->id) focused_window_id = -1; // 取消焦點，讓它沉下去
                }
                else {
                    // 點在標題列，但沒點中按鈕：啟動拖曳！
                    win->z_layer = 1; // 抓取時解除置底
                    dragged_window_id = win->id;
                    drag_offset_x = x - win->x;
                    drag_offset_y = y - win->y;
                }
            }
            // 點擊在標題列的其他地方：啟動拖曳！
            else if (y >= win->y && y <= win->y + 20) {
                win->z_layer = 1; // 抓取時解除置底
                dragged_window_id = win->id;
                drag_offset_x = x - win->x;
                drag_offset_y = y - win->y;
            }
            // ==========================================
            // 點擊了畫布內部 (Client Area)！
            // ==========================================
            else if (win->canvas != 0 &&
                     x >= win->x + 2 && x <= win->x + win->width - 2 &&
                     y >= win->y + 22 && y <= win->y + win->height - 2) {

                win->last_click_x = x - (win->x + 2);
                win->last_click_y = y - (win->y + 22);
                win->has_new_click = 1; // 舉起旗標，等待 App 來領取！
            }

            gui_dirty = 1;
            return 1;
        }
    }

    // ==========================================
    // 2. 檢查背景視窗
    // ==========================================
    for (int i = MAX_WINDOWS - 1; i >= 0; i--) {
        // 【重要修復】同樣加入 !windows[i].is_minimized 判斷
        if (windows[i].is_active && !windows[i].is_minimized && i != focused_window_id) {
            window_t* win = &windows[i];

            if (x >= win->x && x <= win->x + win->width && y >= win->y && y <= win->y + win->height) {

                set_focused_window(i); // 被點到了，切換焦點並拉到最上層！
                gui_dirty = 1;

                int base_x = win->x + win->width;

                if (y >= win->y + 4 && y <= win->y + 18) {
                    if (x >= base_x - 20 && x <= base_x - 6) {
                        int target_pid = win->owner_pid;
                        close_window(win->id);
                        if (target_pid > 1) { sys_kill(target_pid); }
                    }
                    else if (x >= base_x - 36 && x <= base_x - 22) {
                        if (win->is_maximized) {
                            win->x = win->orig_x; win->y = win->orig_y;
                            win->width = win->orig_w; win->height = win->orig_h;
                            win->is_maximized = 0;
                        } else {
                            win->orig_x = win->x; win->orig_y = win->y;
                            win->orig_w = win->width; win->orig_h = win->height;
                            win->x = 0; win->y = 0;
                            win->width = SCREEN_WIDTH;
                            win->height = SCREEN_HEIGHT - TASKBAR_HEIGHT;
                            win->is_maximized = 1;
                        }
                    }
                    else if (x >= base_x - 52 && x <= base_x - 38) {
                        win->is_minimized = 1;
                        if (focused_window_id == win->id) focused_window_id = -1;
                    }
                    else if (x >= base_x - 68 && x <= base_x - 54) {
                        win->z_layer = 0;
                        if (focused_window_id == win->id) focused_window_id = -1;
                    }
                    else {
                        win->z_layer = 1;
                        dragged_window_id = win->id;
                        drag_offset_x = x - win->x;
                        drag_offset_y = y - win->y;
                    }
                }
                else if (y >= win->y && y <= win->y + 20) {
                    win->z_layer = 1;
                    dragged_window_id = win->id;
                    drag_offset_x = x - win->x;
                    drag_offset_y = y - win->y;
                }
                else if (win->canvas != 0 &&
                         x >= win->x + 2 && x <= win->x + win->width - 2 &&
                         y >= win->y + 22 && y <= win->y + win->height - 2) {

                    win->last_click_x = x - (win->x + 2);
                    win->last_click_y = y - (win->y + 22);
                    win->has_new_click = 1;
                }

                return 1; // 吞掉點擊！
            }
        }
    }
    return 0;
}

static int handle_desktop_click(int x, int y) {
    // Start 按鈕
    if (x >= 4 && x <= 64 && y >= SCREEN_HEIGHT - TASKBAR_HEIGHT + 4 && y <= SCREEN_HEIGHT - 8) {
        start_menu_open = 1;
        gui_dirty = 1;
        return 1;
    }

    // 桌面圖示 1: Terminal
    if (x >= 20 && x <= 52 && y >= 20 && y <= 52) {
        int offset = window_count * 20;
        int term_win = create_window(50 + offset, 50 + offset, 368, 228, "Simple OS Terminal", 1);
        terminal_bind_window(term_win);
        gui_dirty = 1;
        return 1;
    }

    // 桌面圖示 2: Status
    if (x >= 20 && x <= 52 && y >= 80 && y <= 112) {
        int offset = window_count * 20;
        create_window(450 - offset, 100 + offset, 300, 200, "System Status", 1);
        gui_dirty = 1;
        return 1;
    }

    //  檢查是否點擊了工作列上的最小化頁籤
    int task_x = 70;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_active && windows[i].is_minimized) {
            if (x >= task_x && x <= task_x + 80 &&
                y >= SCREEN_HEIGHT - TASKBAR_HEIGHT + 4 && y <= SCREEN_HEIGHT - 8) {

                windows[i].is_minimized = 0; // 解除最小化
                set_focused_window(i);       // 取得焦點
                gui_dirty = 1;
                return 1;
            }
            task_x += 84;
        }
    }
    return 0;
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
        draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BLACK);
        tty_render_fullscreen();
        gfx_swap_buffers();
        return;
    }

    // 1. 畫上絕美的漸層背景與桌面圖示！
    draw_desktop_background();
    draw_desktop_icons();

    // 2. 先畫「置底層 (z_layer == 0)」的視窗
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_active && windows[i].z_layer == 0 && i != focused_window_id) {
            draw_window_internal(&windows[i]);
        }
    }

    // 3. 再畫「一般層 (z_layer == 1)」的視窗
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_active && windows[i].z_layer == 1 && i != focused_window_id) {
            draw_window_internal(&windows[i]);
        }
    }

    // 4. 最後畫「有焦點」的最上層視窗
    if (focused_window_id != -1 && windows[focused_window_id].is_active) {
        draw_window_internal(&windows[focused_window_id]);
    }

    // 5. 畫工作列 (Taskbar)
    draw_taskbar();

    // 6. 畫開始選單 (必須疊在所有視窗和工作列上面)
    draw_start_menu();

    // 7. 畫滑鼠游標 (永遠在最最上層)
    draw_cursor(mouse_x, mouse_y);

    // 8. 交換畫布
    gfx_swap_buffers();
}

// 提供給打字機呼叫的重繪函式
// 現在它只做「標記」，0.0001 毫秒就執行完，絕對不卡 CPU！
// ==========================================
void gui_redraw(void) {
    gui_dirty = 1;
}

// 真正的渲染引擎，交給系統時鐘來呼叫！(約 60~100 FPS)
void gui_handle_timer(void) {
    // ==========================================
    // 每隔約 1 秒 (100 ticks)，自動刷新畫面！
    // ==========================================
    if (tick % 100 == 0) {
        gui_dirty = 1;
    }

    // 如果畫面是髒的，而且目前「沒有人正在渲染」
    if (gui_dirty && !is_rendering) {
        is_rendering = 1;

        gui_render(last_mouse_x, last_mouse_y);

        gui_dirty = 0;
        is_rendering = 0;
    }
}


int gui_check_ui_click(int x, int y) {
    if (handle_start_menu_click(x, y)) return 1;
    if (handle_window_click(x, y)) return 1;
    if (handle_desktop_click(x, y)) return 1;
    return 0;
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


// ---- Window API ----

int create_window(int x, int y, int width, int height, const char* title, uint32_t owner_pid) {
    if (window_count >= MAX_WINDOWS) return -1;
    int id = window_count++;

    windows[id].owner_pid = owner_pid; // 

    // 計算畫布大小並分配記憶體
    // 畫布寬度 = 總寬 - 左右邊框 (4)
    // 畫布高度 = 總高 - 上下邊框與標題列 (24)
    int canvas_w = width - 4;
    int canvas_h = height - 24;

    // 【優化】只有 User App (PID > 1) 申請的視窗，才分配畫布，省下大量記憶體！
    if (owner_pid > 1 && canvas_w > 0 && canvas_h > 0) {
        windows[id].canvas = (uint32_t*)kmalloc(canvas_w * canvas_h * 4);
        memset(windows[id].canvas, 0, canvas_w * canvas_h * 4);
    } else {
        windows[id].canvas = 0; // Kernel 原生視窗不需要獨立畫布
    }

    windows[id].id = id;
    windows[id].x = x;
    windows[id].y = y;
    windows[id].width = width;
    windows[id].height = height;
    strcpy(windows[id].title, title);
    windows[id].is_active = 1;
    windows[id].has_new_key = 0;

    windows[id].has_new_click = 0; // 初始化事件狀態

    //  window 最大/最小 狀態
    windows[id].is_minimized = 0;
    windows[id].is_maximized = 0;
    windows[id].z_layer = 1; // 預設為一般圖層
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

// ==========================================
// 滑鼠移動時，如果有拖曳視窗，就跟著更新座標！
// ==========================================
void gui_update_mouse(int x, int y) {
    last_mouse_x = x;
    last_mouse_y = y;

    // 如果目前有抓著視窗，就更新視窗的 (X, Y) 座標
    if (dragged_window_id != -1 && windows[dragged_window_id].is_active) {
        windows[dragged_window_id].x = x - drag_offset_x;
        windows[dragged_window_id].y = y - drag_offset_y;
    }

    gui_dirty = 1; // 畫面髒了，需要重繪
}

// ==========================================
// 滑鼠「左鍵按下」事件
// ==========================================
void gui_mouse_down(int x, int y) {
    // 所有的 UI 碰撞偵測與拖曳邏輯，全部統一交給 check_ui_click 處理！
    gui_check_ui_click(x, y);
}

// ==========================================
// 滑鼠「左鍵放開」事件
// ==========================================
void gui_mouse_up(void) {
    // 放開滑鼠，解除拖曳狀態
    dragged_window_id = -1;
}


// ==========================================
// 根據 PID 關閉該行程擁有的所有視窗
// ==========================================
void gui_close_windows_by_pid(uint32_t pid) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_active && windows[i].owner_pid == pid) {
            close_window(windows[i].id);
        }
    }
    gui_dirty = 1; // 標記畫面需要重繪
}

// ==========================================
// 鍵盤事件路由器
// 回傳 1 代表 GUI 攔截了按鍵，回傳 0 代表放行給 Terminal
// ==========================================
int gui_handle_keyboard(char c) {
    if (!gui_mode_enabled) return 0; // CLI 模式直接放行

    if (focused_window_id != -1 && windows[focused_window_id].is_active) {
        window_t* win = &windows[focused_window_id];

        // 只要焦點是 Terminal，我們就不攔截，讓它照舊印字
        if (strcmp(win->title, "Simple OS Terminal") == 0) {
            return 0;
        }

        // 如果是其他的 GUI 應用程式，把按鍵塞給它！
        if (win->canvas != 0) {
            win->last_key = c;
            win->has_new_key = 1;
            return 1; // 攔截成功！
        }
    }
    return 0; // 沒人要這個按鍵，放行
}
