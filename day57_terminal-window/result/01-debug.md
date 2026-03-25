kprintf 的訊息被放到 Terminal 裡了，不過

1. 原本滑鼠拖曳 window 壞掉了
2. 點右上角的 [x] 也壞了
3. 點 Window 標題，也無法 focus 了

底下是調整後的 source code:

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
        char* filenames[] = {"shell.elf", "echo.elf", "cat.elf", "ping.elf", "pong.elf", "touch.elf", "ls.elf", "rm.elf", "mkdir.elf"};
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
    init_gui();

    // 建立狀態視窗
    create_window(450, 100, 300, 200, "System Status");

    // 建立終端機視窗 (寬度: 45個字*8 + 左右邊框8 = 368，高度: 25行*8 + 標題列/邊框28 = 228)
    int term_win = create_window(50, 50, 368, 228, "Simple OS Terminal");

    // 【關鍵綁定】告訴 TTY 系統，請把文字印在這個視窗裡面！
    terminal_bind_window(term_win);

    gui_render(400, 300);
    init_mouse();

    __asm__ volatile ("sti");

    // 左上角的終端機文字會自然地印在藍綠色的桌面上
    kprintf("=== OS Subsystems Ready ===\n\n");
    kprintf("Welcome to Simple OS Desktop Environment!\n");

    while (1) { __asm__ volatile ("hlt"); }
}
```

---
mouse.c
```c
#include <stdint.h>
#include "io.h"
#include "mouse.h"
#include "gfx.h"
#include "tty.h"
#include "gui.h"

// 宣告你現有的 IO 函式 (假設你有 inb 和 outb，如果沒有請在 utils.c 補上)
extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t data);

// 滑鼠狀態
static uint8_t mouse_cycle = 0;
static int8_t  mouse_byte[3];
static int mouse_x = 400; // 預設在螢幕正中間
static int mouse_y = 300;
// 紀錄目前正在拖曳哪個視窗 (-1 代表沒有)
static int dragged_window_id = -1;
static int prev_left_click = 0; // 【新增】記錄上一次的左鍵狀態，用來偵測「剛按下」的瞬間

// 等待鍵盤控制器就緒
static void mouse_wait(uint8_t a_type) {
    uint32_t timeout = 100000;
    if (a_type == 0) {
        while (timeout--) { if ((inb(0x64) & 1) == 1) return; }
    } else {
        while (timeout--) { if ((inb(0x64) & 2) == 0) return; }
    }
}

// 寫入指令給滑鼠
static void mouse_write(uint8_t write) {
    mouse_wait(1);
    outb(0x64, 0xD4); // 告訴控制器，下一個 byte 要送給滑鼠
    mouse_wait(1);
    outb(0x60, write);
    mouse_wait(0);
    inb(0x60); // 讀取 ACK 回應
}

// 初始化 PS/2 滑鼠
void init_mouse(void) {
    kprintf("[Mouse] Initializing PS/2 Mouse...\n");

    // 1. 啟用附屬裝置 (Mouse)
    mouse_wait(1);
    outb(0x64, 0xA8);

    // 2. 啟用中斷 (IRQ 12)
    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    uint8_t status = (inb(0x60) | 2);
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, status);

    // 3. 設定滑鼠預設值並開始回報資料
    mouse_write(0xF6);
    mouse_write(0xF4);

    // 畫出初始游標
    draw_cursor(mouse_x, mouse_y);
}

// 【滑鼠中斷處理程式】當滑鼠移動或點擊時，IRQ 12 會呼叫這裡！
void mouse_handler(void) {
    uint8_t status = inb(0x64);
    while (status & 0x01) { // 檢查是否有資料可讀
        int8_t mouse_in = inb(0x60);

        // 滑鼠每次傳送 3 個 bytes (封包)
        switch (mouse_cycle) {
            case 0:
                if (mouse_in & 0x08) { // 檢查封包合法性 (bit 3 必須是 1)
                    mouse_byte[0] = mouse_in;
                    mouse_cycle++;
                }
                break;
            case 1:
                mouse_byte[1] = mouse_in;
                mouse_cycle++;
                break;
            case 2:
                mouse_byte[2] = mouse_in;
                mouse_cycle = 0;

                int dx = mouse_byte[1];
                int dy = mouse_byte[2];
                mouse_x += dx;
                mouse_y -= dy;

                if (mouse_x < 0) mouse_x = 0;
                if (mouse_x > 790) mouse_x = 790;
                if (mouse_y < 0) mouse_y = 0;
                if (mouse_y > 590) mouse_y = 590;

                // ==========================================
                // 拖曳與碰撞偵測邏輯
                // ==========================================
                int left_click = mouse_byte[0] & 0x01; // 檢查左鍵
                window_t* wins = get_window(dragged_window_id);

                // 【核心互動邏輯】
                if (left_click && !prev_left_click) {
                    // 滑鼠「剛按下的瞬間」
                    int clicked_id = -1;

                    // 為了符合 Z-Order，我們應該「倒過來」檢查，先檢查最上層的 (也就是 Focused 視窗)
                    int current_focus = get_focused_window();
                    if (current_focus != -1 && wins[current_focus].is_active) {
                        if (mouse_x >= wins[current_focus].x && mouse_x <= wins[current_focus].x + wins[current_focus].width &&
                            mouse_y >= wins[current_focus].y && mouse_y <= wins[current_focus].y + wins[current_focus].height) {
                            clicked_id = current_focus;
                        }
                    }

                    // 如果最上層沒點到，再檢查其他視窗
                    if (clicked_id == -1) {
                        for (int i = 0; i < 10; i++) {
                            if (wins[i].is_active && i != current_focus) {
                                if (mouse_x >= wins[i].x && mouse_x <= wins[i].x + wins[i].width &&
                                    mouse_y >= wins[i].y && mouse_y <= wins[i].y + wins[i].height) {
                                    clicked_id = i;
                                    break;
                                }
                            }
                        }
                    }

                    // 如果真的點到了某個視窗
                    if (clicked_id != -1) {
                        set_focused_window(clicked_id); // 將它拉到最上層！

                        // 判斷是否點到了右上角的 [X] 按鈕
                        if (mouse_x >= wins[clicked_id].x + wins[clicked_id].width - 20 &&
                            mouse_x <= wins[clicked_id].x + wins[clicked_id].width - 6 &&
                            mouse_y >= wins[clicked_id].y + 4 && mouse_y <= wins[clicked_id].y + 18) {
                            close_window(clicked_id); // 關閉視窗！
                        }
                        // 判斷是否點到了標題列，準備拖曳
                        else if (mouse_y >= wins[clicked_id].y && mouse_y <= wins[clicked_id].y + 20) {
                            dragged_window_id = clicked_id;
                        }
                    }
                }
                else if (left_click && prev_left_click) {
                    // 滑鼠「持續按住」中
                    if (dragged_window_id != -1) {
                        wins[dragged_window_id].x += mouse_byte[1];
                        wins[dragged_window_id].y -= mouse_byte[2];
                    }
                }
                else {
                    // 左鍵放開
                    dragged_window_id = -1;
                }

                prev_left_click = left_click; // 更新狀態紀錄
                gui_render(mouse_x, mouse_y);
                break;
        }
        status = inb(0x64); // 繼續讀取直到清空
    }

    // ==========================================
    // 【關鍵新增】發送 EOI (End of Interrupt) 給 PIC
    // ==========================================
    // 因為 IRQ 12 在 Slave PIC，所以必須同時通知 Slave 和 Master！
    outb(0xA0, 0x20); // 通知 Slave PIC
    outb(0x20, 0x20); // 通知 Master PIC
}
```
---
tty.c
```c
#include <stdint.h>
#include "io.h"
#include "mouse.h"
#include "gfx.h"
#include "tty.h"
#include "gui.h"

// 宣告你現有的 IO 函式 (假設你有 inb 和 outb，如果沒有請在 utils.c 補上)
extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t data);

// 滑鼠狀態
static uint8_t mouse_cycle = 0;
static int8_t  mouse_byte[3];
static int mouse_x = 400; // 預設在螢幕正中間
static int mouse_y = 300;
// 紀錄目前正在拖曳哪個視窗 (-1 代表沒有)
static int dragged_window_id = -1;
static int prev_left_click = 0; // 【新增】記錄上一次的左鍵狀態，用來偵測「剛按下」的瞬間

// 等待鍵盤控制器就緒
static void mouse_wait(uint8_t a_type) {
    uint32_t timeout = 100000;
    if (a_type == 0) {
        while (timeout--) { if ((inb(0x64) & 1) == 1) return; }
    } else {
        while (timeout--) { if ((inb(0x64) & 2) == 0) return; }
    }
}

// 寫入指令給滑鼠
static void mouse_write(uint8_t write) {
    mouse_wait(1);
    outb(0x64, 0xD4); // 告訴控制器，下一個 byte 要送給滑鼠
    mouse_wait(1);
    outb(0x60, write);
    mouse_wait(0);
    inb(0x60); // 讀取 ACK 回應
}

// 初始化 PS/2 滑鼠
void init_mouse(void) {
    kprintf("[Mouse] Initializing PS/2 Mouse...\n");

    // 1. 啟用附屬裝置 (Mouse)
    mouse_wait(1);
    outb(0x64, 0xA8);

    // 2. 啟用中斷 (IRQ 12)
    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    uint8_t status = (inb(0x60) | 2);
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, status);

    // 3. 設定滑鼠預設值並開始回報資料
    mouse_write(0xF6);
    mouse_write(0xF4);

    // 畫出初始游標
    draw_cursor(mouse_x, mouse_y);
}

// 【滑鼠中斷處理程式】當滑鼠移動或點擊時，IRQ 12 會呼叫這裡！
void mouse_handler(void) {
    uint8_t status = inb(0x64);
    while (status & 0x01) { // 檢查是否有資料可讀
        int8_t mouse_in = inb(0x60);

        // 滑鼠每次傳送 3 個 bytes (封包)
        switch (mouse_cycle) {
            case 0:
                if (mouse_in & 0x08) { // 檢查封包合法性 (bit 3 必須是 1)
                    mouse_byte[0] = mouse_in;
                    mouse_cycle++;
                }
                break;
            case 1:
                mouse_byte[1] = mouse_in;
                mouse_cycle++;
                break;
            case 2:
                mouse_byte[2] = mouse_in;
                mouse_cycle = 0;

                int dx = mouse_byte[1];
                int dy = mouse_byte[2];
                mouse_x += dx;
                mouse_y -= dy;

                if (mouse_x < 0) mouse_x = 0;
                if (mouse_x > 790) mouse_x = 790;
                if (mouse_y < 0) mouse_y = 0;
                if (mouse_y > 590) mouse_y = 590;

                // ==========================================
                // 拖曳與碰撞偵測邏輯
                // ==========================================
                int left_click = mouse_byte[0] & 0x01; // 檢查左鍵
                window_t* wins = get_window(dragged_window_id);

                // 【核心互動邏輯】
                if (left_click && !prev_left_click) {
                    // 滑鼠「剛按下的瞬間」
                    int clicked_id = -1;

                    // 為了符合 Z-Order，我們應該「倒過來」檢查，先檢查最上層的 (也就是 Focused 視窗)
                    int current_focus = get_focused_window();
                    if (current_focus != -1 && wins[current_focus].is_active) {
                        if (mouse_x >= wins[current_focus].x && mouse_x <= wins[current_focus].x + wins[current_focus].width &&
                            mouse_y >= wins[current_focus].y && mouse_y <= wins[current_focus].y + wins[current_focus].height) {
                            clicked_id = current_focus;
                        }
                    }

                    // 如果最上層沒點到，再檢查其他視窗
                    if (clicked_id == -1) {
                        for (int i = 0; i < 10; i++) {
                            if (wins[i].is_active && i != current_focus) {
                                if (mouse_x >= wins[i].x && mouse_x <= wins[i].x + wins[i].width &&
                                    mouse_y >= wins[i].y && mouse_y <= wins[i].y + wins[i].height) {
                                    clicked_id = i;
                                    break;
                                }
                            }
                        }
                    }

                    // 如果真的點到了某個視窗
                    if (clicked_id != -1) {
                        set_focused_window(clicked_id); // 將它拉到最上層！

                        // 判斷是否點到了右上角的 [X] 按鈕
                        if (mouse_x >= wins[clicked_id].x + wins[clicked_id].width - 20 &&
                            mouse_x <= wins[clicked_id].x + wins[clicked_id].width - 6 &&
                            mouse_y >= wins[clicked_id].y + 4 && mouse_y <= wins[clicked_id].y + 18) {
                            close_window(clicked_id); // 關閉視窗！
                        }
                        // 判斷是否點到了標題列，準備拖曳
                        else if (mouse_y >= wins[clicked_id].y && mouse_y <= wins[clicked_id].y + 20) {
                            dragged_window_id = clicked_id;
                        }
                    }
                }
                else if (left_click && prev_left_click) {
                    // 滑鼠「持續按住」中
                    if (dragged_window_id != -1) {
                        wins[dragged_window_id].x += mouse_byte[1];
                        wins[dragged_window_id].y -= mouse_byte[2];
                    }
                }
                else {
                    // 左鍵放開
                    dragged_window_id = -1;
                }

                prev_left_click = left_click; // 更新狀態紀錄
                gui_render(mouse_x, mouse_y);
                break;
        }
        status = inb(0x64); // 繼續讀取直到清空
    }

    // ==========================================
    // 【關鍵新增】發送 EOI (End of Interrupt) 給 PIC
    // ==========================================
    // 因為 IRQ 12 在 Slave PIC，所以必須同時通知 Slave 和 Master！
    outb(0xA0, 0x20); // 通知 Slave PIC
    outb(0x20, 0x20); // 通知 Master PIC
}
```
---
tty.h
```c
#ifndef TTY_H
#define TTY_H

#include <stdint.h>
#include <stddef.h>

void terminal_initialize(void);
void terminal_putchar(char c);
void terminal_write(const char* data, size_t size);
void terminal_writestring(const char* data);
//void tty_render(void);

// 刪掉 void tty_render(void);
void terminal_bind_window(int win_id);
void tty_render_window(int win_id);
#endif
```
---
gui.c
```c
#include "gui.h"
#include "gfx.h"
#include "utils.h"
#include "tty.h"

#define MAX_WINDOWS 10
#define TERM_BG 0x008080 // 桌面底色

extern void tty_render_window(int win_id);

static window_t windows[MAX_WINDOWS];
static int window_count = 0;

// 【新增】記錄當前被選中的視窗 (-1 代表沒有)
static int focused_window_id = -1;

// 【修改】加入 Focus 判斷與 [X] 按鈕
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


void init_gui(void) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i].is_active = 0;
    }
}

int create_window(int x, int y, int width, int height, const char* title) {
    if (window_count >= MAX_WINDOWS) return -1;

    int id = window_count++;
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

window_t* get_window(int id) {
    if (id >= 0 && id < MAX_WINDOWS && windows[id].is_active) {
        return &windows[id];
    }
    return 0; // NULL
}

// 【新增】API 實作
void set_focused_window(int id) { focused_window_id = id; }
int get_focused_window(void) { return focused_window_id; }
void close_window(int id) {
    if (id >= 0 && id < MAX_WINDOWS) {
        windows[id].is_active = 0;
        if (focused_window_id == id) focused_window_id = -1;
    }
}

void gui_render(int mouse_x, int mouse_y) {
    draw_rect(0, 0, 800, 600, TERM_BG);

    // 1. 先畫「沒有焦點」的活躍視窗 (讓它們在底層)
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_active && i != focused_window_id) {
            draw_window_internal(&windows[i]);
        }
    }

    // 2. 最後畫「有焦點」的視窗 (讓它疊在最上層！)
    if (focused_window_id != -1 && windows[focused_window_id].is_active) {
        draw_window_internal(&windows[focused_window_id]);

        // 示範內容 (畫在 focused 的視窗裡，如果是視窗 0 的話)
        // if (focused_window_id == 0) {
        //     draw_string(windows[0].x + 10, windows[0].y + 30, "CPU: x86 32-bit", 0x000000, 0xC0C0C0);
        //     draw_string(windows[0].x + 10, windows[0].y + 50, "Memory: 16 MB", 0x000000, 0xC0C0C0);
        //     draw_string(windows[0].x + 10, windows[0].y + 70, "GUI: Active", 0x000000, 0xC0C0C0);
        // }
    }

    // tty_render();
    draw_cursor(mouse_x, mouse_y);
    gfx_swap_buffers();
}
```
---
gui.h
```c
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
window_t* get_window(int id);

void gui_render(int mouse_x, int mouse_y);

// 【Day 56 新增】焦點與關閉控制
void set_focused_window(int id);
int get_focused_window(void);
void close_window(int id);

#endif
```
