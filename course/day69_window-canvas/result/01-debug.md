
編譯完成，跑起來後，進入 desktop，terminal 沒有出現任何文字，也無法輸入任何文字，點桌面 status icon 出現的 window 也沒有正常顯示.
如果開機進入 Terminal 則可以正常輸入文字。

我猜是 gui.c 裡面處理 window 的邏輯有問題？

---
gui.c
```c
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

    // ==========================================
    // 【Day 69 新增】渲染 User Space 傳來的獨立畫布！
    // ==========================================
    if (win->canvas != 0) {
        int start_x = win->x + 2;
        int start_y = win->y + 22;
        int canvas_w = win->width - 4;
        int canvas_h = win->height - 24;

        for (int cy = 0; cy < canvas_h; cy++) {
            for (int cx = 0; cx < canvas_w; cx++) {
                uint32_t color = win->canvas[cy * canvas_w + cx];
                // 這裡我們暫時用 draw_rect 畫 1x1 的點
                // 如果你的 gfx.h 有 draw_pixel(x, y, color)，請換成 draw_pixel 會快很多！
                draw_rect(start_x + cx, start_y + cy, 1, 1, color);
            }
        }
    }
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

    // 【Day 69 新增】計算畫布大小並分配記憶體
    // 畫布寬度 = 總寬 - 左右邊框 (4)
    // 畫布高度 = 總高 - 上下邊框與標題列 (24)
    int canvas_w = width - 4;
    int canvas_h = height - 24;
    if (canvas_w > 0 && canvas_h > 0) {
        windows[id].canvas = (uint32_t*)kmalloc(canvas_w * canvas_h * 4); // 每個像素 4 Bytes (ARGB)
        // 預設填滿黑色
        memset(windows[id].canvas, 0, canvas_w * canvas_h * 4);
    } else {
        windows[id].canvas = 0;
    }

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
```

---
kernel.c
```c
#include "tty.h"
#include "utils.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "timer.h"
#include "pmm.h"
#include "kheap.h"
#include "ata.h"
#include "mbr.h"
#include "vfs.h"
#include "simplefs.h"
#include "elf.h"
#include "task.h"
#include "multiboot.h"
#include "gfx.h"
#include "mouse.h"
#include "gui.h"

void setup_filesystem(uint32_t part_lba, multiboot_info_t* mbd) {
    kprintf("[Kernel] Setting up SimpleFS environment...\n");
    simplefs_mount(part_lba);
    simplefs_format(part_lba, 10000);
    simplefs_create_file(1, "hello.txt", "This is the content of the very first file ever created on Simple OS!\n", 70);

    // 【修改】自動巡覽所有模組並寫入 HDD
    if (mbd->mods_count > 0) {
        multiboot_module_t* mod = (multiboot_module_t*)mbd->mods_addr;
        char* filenames[] = {
            "shell.elf",
            "echo.elf", "ping.elf", "pong.elf",
            // file/directory
            "touch.elf", "cat.elf", "ls.elf", "rm.elf", "mkdir.elf",
            // process
            "ps.elf", "top.elf", "kill.elf",
            // memory
            "free.elf",
            // window app
            "status.elf", "paint.elf"
        };
        uint32_t expected_mods = sizeof(filenames) / sizeof(filenames[0]);

        for(uint32_t i = 0; i < mbd->mods_count && i < expected_mods; i++) {
            uint32_t size = mod[i].mod_end - mod[i].mod_start;
            kprintf("[Kernel] Installing [%s] to HDD (Size: %d bytes)...\n", filenames[i], size);
            simplefs_create_file(1, filenames[i], (char*)mod[i].mod_start, size);
        }
    }

    // simplefs_list_files();

}

void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
    (void)magic; // 忽略未使用的警告

    terminal_initialize();
    kprintf("=== Simple OS Booting ===\n");

    // --- 系統底層基礎建設 ---
    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384);
    init_kheap();
    init_gfx(mbd);

    // ==========================================
    // 2. 解析 GRUB 傳遞的 Cmdline
    // ==========================================
    int is_gui = 1; // 預設為 GUI 模式

    // 檢查 mbd 的 bit 2，確認 cmdline 是否有效
    if (mbd->flags & (1 << 2)) {
        char* cmdline = (char*) mbd->cmdline;
        // 如果 GRUB 參數包含 mode=cli，就切換到 CLI 模式
        if (strstr(cmdline, "mode=cli") != 0) {
            is_gui = 0;
        }
    }

    // 3. 根據模式初始化系統介面
    enable_gui_mode(is_gui);
    terminal_set_mode(is_gui);

    if (is_gui) {
        init_gui();
        int term_win = create_window(50, 50, 368, 228, "Simple OS Terminal", 1); // 預設 PID(1) 建立
        terminal_bind_window(term_win);
        gui_render(400, 300);

    } else {
        // CLI 模式只綁定一個無效的視窗 ID，並強制重繪全螢幕
        terminal_bind_window(-1);
        gui_redraw();
    }

    init_mouse();

    __asm__ volatile ("sti");

    // 左上角的終端機文字會自然地印在藍綠色的桌面上
    kprintf("=== OS Subsystems Ready ===\n\n");
    kprintf("Welcome to Simple OS Desktop Environment!\n");

    // --- 儲存與檔案系統 ---
    uint32_t part_lba = parse_mbr();
    if (part_lba == 0) {
        kprintf("Disk Error: No partition found.\n");
        while(1) __asm__ volatile("hlt");
    }
    // 初始化 file system: mount, format, copy external applications
    setup_filesystem(part_lba, mbd);

    // ==========================================
    // 應用程式載入與排程 (Ring 0 -> Ring 3)
    // ==========================================
    kprintf("[Kernel] Fetching 'shell.elf' from Virtual File System...\n");
    fs_node_t* app_node = simplefs_find(1, "shell.elf");

    if (app_node != 0) {
        uint8_t* app_buffer = (uint8_t*) kmalloc(app_node->length);
        vfs_read(app_node, 0, app_node->length, app_buffer);
        uint32_t entry_point = elf_load((elf32_ehdr_t*)app_buffer);

        if (entry_point != 0) {
            kprintf("Creating Initial Process (Shell)...\n\n");

            // 啟動排程器 (Timer IRQ0 開始跳動)
            init_multitasking();

            // 為 Shell 分配專屬的 User Stack
            uint32_t ustack1_phys = (uint32_t) pmm_alloc_page();
            map_page(0x083FF000, ustack1_phys, 7);

            // 建立 Ring 3 主任務
            create_user_task(entry_point, 0x083FF000 + 4096);

            kprintf("[Kernel] Dropping to idle state. Enjoy your GUI!\n");

            // Kernel 功成身退，把自己宣告死亡！
            exit_task();
        }
    } else {
        kprintf("[Kernel] Error: Shell (shell.elf) not found on disk.\n");
    }

    while (1) { __asm__ volatile ("hlt"); }
}
```

---
syscall.c
```c
#include "syscall.h"
#include "tty.h"
#include "utils.h"
#include "simplefs.h"
#include "keyboard.h"
#include "task.h"
#include "gui.h"

#define MAX_MESSAGES 16 // IPC 訊息佇列 (Message Queue)
#define SYS_SET_DISPLAY_MODE 99 // 切換 desktop or terminal mode.

extern fs_node_t* simplefs_find(uint32_t dir_lba, char* filename);
extern int vfs_create_file(uint32_t dir_lba, char* filename, char* content);
extern int vfs_readdir(uint32_t dir_lba, int index, char* out_name, uint32_t* out_size, uint32_t* out_type);
extern int vfs_delete_file(uint32_t dir_lba, char* filename);
extern int vfs_mkdir(uint32_t dir_lba, char* dirname);
extern int vfs_getcwd(uint32_t dir_lba, char* buffer);
extern uint32_t simplefs_get_dir_lba(uint32_t current_dir_lba, char* dirname);
extern uint32_t mounted_part_lba;
extern int sys_kill(int pid);
extern void pmm_get_stats(uint32_t* total, uint32_t* used, uint32_t* free_frames);
extern uint32_t paging_get_active_universes(void);

typedef struct {
    char data[128]; // 每則訊息最大 128 bytes
} ipc_msg_t;

fs_node_t* fd_table[32] = {0};

// 環狀佇列 (Circular Buffer)
ipc_msg_t mailbox_queue[MAX_MESSAGES];
int mb_head = 0;  // 讀取頭
int mb_tail = 0;  // 寫入尾
int mb_count = 0; // 目前信箱裡的信件數量


// --- Internal API ----
// Mutex for Single Processor (SMP)
// 核心同步鎖：利用開關中斷來保護 Critical Section
void ipc_lock() {
    __asm__ volatile("cli"); // 關閉中斷：誰都別想搶走我的 CPU！
}

void ipc_unlock() {
    __asm__ volatile("sti"); // 開啟中斷：我用完了，大家可以繼續排隊了。
}


// --- Public API ----

void init_syscalls(void) {
    kprintf("System Calls initialized on Interrupt 0x80 (128).\n");
}

void syscall_handler(registers_t *regs) {
    // Accumulator Register: 函式回傳值或 Syscall 編號
    // 配合 asm/interrupt.S: IRS128
    uint32_t eax = regs->eax;

    if (eax == 2) {
        kprintf((char*)regs->ebx);
    }
    else if (eax == 3) {
        char* filename = (char*)regs->ebx;

        // 取得目前的目錄 (如果沒設定，1 代表相對根目錄)
        uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;
        fs_node_t* node = simplefs_find(current_dir, filename);

        if (node == 0) { regs->eax = -1; return; }
        for (int i = 3; i < 32; i++) {
            if (fd_table[i] == 0) {
                fd_table[i] = node;
                regs->eax = i;
                return;
            }
        }
        regs->eax = -1;
    }
    else if (eax == 4) {
        int fd = (int)regs->ebx;
        uint8_t* buffer = (uint8_t*)regs->ecx;
        uint32_t size = (uint32_t)regs->edx;
        if (fd >= 3 && fd < 32 && fd_table[fd] != 0) {
            regs->eax = vfs_read(fd_table[fd], 0, size, buffer);
        } else {
            regs->eax = -1;
        }
    }
    else if (eax == 5) {
        char c = keyboard_getchar();
        char str[2] = {c, '\0'};
        kprintf(str);
        regs->eax = (uint32_t)c;
    }
    else if (eax == 6) {
        schedule();
    }
    else if (eax == 7) {
        exit_task();
    }
    else if (eax == 8) {
        regs->eax = sys_fork(regs);
    }
    else if (eax == 9) {
        regs->eax = sys_exec(regs);
    }
    else if (eax == 10) {
        regs->eax = sys_wait(regs->ebx);
    }

    // ==========================================
    // IPC 系統呼叫
    // ==========================================
    // Syscall 11: sys_send (傳送訊息)
    else if (eax == 11) {
        char* msg = (char*)regs->ebx;

        ipc_lock(); // LOCK: CRITICAL SECTION

        if (mb_count < MAX_MESSAGES) {
            strcpy(mailbox_queue[mb_tail].data, msg);
            mb_tail = (mb_tail + 1) % MAX_MESSAGES;
            mb_count++;
            regs->eax = 0;
        } else {
            regs->eax = -1; // Queue Full
        }

        ipc_unlock();   // UNLOCK
    }
    // Syscall 12: sys_recv (接收訊息)
    else if (eax == 12) {
        char* buffer = (char*)regs->ebx;

        ipc_lock();

        if (mb_count > 0) {
            strcpy(buffer, mailbox_queue[mb_head].data);
            mb_head = (mb_head + 1) % MAX_MESSAGES;
            mb_count--;
            regs->eax = 1;
        } else {
            regs->eax = 0;
        }

        ipc_unlock();
    }

    // Syscall 13: sbrk (動態記憶體擴充)
    else if (eax == 13) {
        int increment = (int)regs->ebx;
        uint32_t old_end = current_task->heap_end;

        // 把 Heap 的邊界往上推
        current_task->heap_end += increment;

        // 回傳舊的邊界，這就是新分配空間的起始位址！
        regs->eax = old_end;
    }

    // Syscall 14: sys_create (建立/寫入文字檔)
    else if (eax == 14) {
        char* filename = (char*)regs->ebx;
        char* content = (char*)regs->ecx;
        uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;

        ipc_lock();
        regs->eax = vfs_create_file(current_dir, filename, content);
        ipc_unlock();
    }

    // Syscall 15: sys_readdir (讀取目錄內容)
    else if (eax == 15) {
        int index = (int)regs->ebx;
        char* out_name = (char*)regs->ecx;
        uint32_t* out_size = (uint32_t*)regs->edx;
        uint32_t* out_type = (uint32_t*)regs->esi; //從 ESI 暫存器拿出 out_type 指標！

        // 取得目前的目錄
        uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;

        ipc_lock();
        regs->eax = simplefs_readdir(current_dir, index, out_name, out_size, out_type);
        ipc_unlock();
    }

    // Syscall 16: sys_remove (刪除檔案)
    else if (eax == 16) {
        char* filename = (char*)regs->ebx;
        uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;

        ipc_lock();
        regs->eax = vfs_delete_file(current_dir, filename);
        ipc_unlock();
    }

    // Syscall 17: sys_mkdir (建立目錄)
    else if (eax == 17) {
        char* dirname = (char*)regs->ebx;
        uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;

        ipc_lock();
        regs->eax = vfs_mkdir(current_dir, dirname);
        ipc_unlock();
    }

    // Syscall 18: sys_chdir (切換目錄)
    else if (eax == 18) {
        char* dirname = (char*)regs->ebx;
        // 如果還沒有 CWD，預設就是根目錄
        uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;

        ipc_lock();
        if (strcmp(dirname, "/") == 0) {
            current_task->cwd_lba = 1; // 回到根目錄
            regs->eax = 0;
        } else {
            uint32_t target_lba = simplefs_get_dir_lba(current_dir, dirname);
            if (target_lba != 0) {
                current_task->cwd_lba = target_lba; // 切換成功, 更新任務的 CWD！
                regs->eax = 0;
            } else {
                regs->eax = -1; // 找不到該目錄
            }
        }
        ipc_unlock();
    }
    // Syscall 19: sys_getcwd (取得當前路徑)
    else if (eax == 19) {
        char* buffer = (char*)regs->ebx;

        // 由 Syscall 層負責查出目前的目錄 LBA
        uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;

        ipc_lock();
        regs->eax = vfs_getcwd(current_dir, buffer);
        ipc_unlock();
    }

    // ==========================================
    // 【Day 63 新增】系統與行程資訊 Syscall
    // ==========================================
    // Syscall 20: sys_getpid
    else if (eax == 20) {
        ipc_lock();
        regs->eax = current_task->pid;
        ipc_unlock();
    }
    // Syscall 21: sys_get_process_list
    else if (eax == 21) {
        process_info_t* list = (process_info_t*)regs->ebx;
        int max_count = (int)regs->ecx;

        ipc_lock();
        regs->eax = task_get_process_list(list, max_count);
        ipc_unlock();
    }
    // ==========================================
    // 【Day 65 優化】Syscall 22: 清空終端機畫面
    // ==========================================
    else if (eax == 22) {
        ipc_lock();
        terminal_initialize(); // 清空 text_buffer 並把游標歸零
        extern void gui_redraw(void);
        gui_redraw();          // 強制觸發畫面更新
        regs->eax = 0;
        ipc_unlock();
    }
    // ==========================================
    // 【Day 66 新增】Syscall 24: sys_kill
    // ==========================================
    else if (eax == 24) {
        int target_pid = (int)regs->ebx;
        regs->eax = sys_kill(target_pid);
    }

    // Syscall 25: sys_get_mem_info
    else if (eax == 25) {
        mem_info_t* info = (mem_info_t*)regs->ebx;
        ipc_lock();
        pmm_get_stats(&info->total_frames, &info->used_frames, &info->free_frames);
        info->active_universes = paging_get_active_universes();
        regs->eax = 0;
        ipc_unlock();
    }

    // ==========================================
    // 【Day 68 新增】Syscall 26: sys_create_window
    // ==========================================
    else if (eax == 26) {
        char* title = (char*)regs->ebx;
        int width = (int)regs->ecx;
        int height = (int)regs->edx;

        // 簡單的視窗錯開邏輯，避免視窗全部疊在一起
        static int win_offset = 0;
        int x = 200 + win_offset;
        int y = 150 + win_offset;
        win_offset = (win_offset + 30) % 200;

        ipc_lock();
        // 綁定當前呼叫它的行程 PID！
        regs->eax = create_window(x, y, width, height, title, current_task->pid);
        extern void gui_redraw(void);
        gui_redraw(); // 立刻把視窗畫出來
        ipc_unlock();
    }
    // ==========================================
    // 【Day 69 新增】Syscall 27: sys_update_window
    // 將 User Space 的畫布資料推送到 Kernel 進行合成
    // ==========================================
    else if (eax == 27) {
        int win_id = (int)regs->ebx;
        uint32_t* user_buffer = (uint32_t*)regs->ecx;

        ipc_lock();
        extern window_t* get_window(int id);
        window_t* win = get_window(win_id);

        // 確保視窗存在，而且這個行程真的是視窗的主人！(安全性檢查)
        if (win != 0 && win->owner_pid == current_task->pid && win->canvas != 0) {
            int canvas_bytes = (win->width - 4) * (win->height - 24) * 4;
            // 把 User 的陣列拷貝到 Kernel 的視窗畫布裡
            memcpy(win->canvas, user_buffer, canvas_bytes);

            // 標記畫面為髒，請排程器在下個 Tick 重繪螢幕
            extern void gui_redraw(void);
            gui_redraw();
        }
        regs->eax = 0;
        ipc_unlock();
    }

    // Syscall: 99
    else if (eax == SYS_SET_DISPLAY_MODE) {
        // 假設 EBX 存放第一個參數 (is_gui)
        int is_gui = (int) regs->ebx;

        ipc_lock();
        switch_display_mode(is_gui);
        regs->eax = 0; // 回傳成功
        ipc_unlock();
    }
}
```
