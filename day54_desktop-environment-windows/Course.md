太棒了，Rick！🎉 看到那句「滑鼠可以移動了」，我隔著螢幕都忍不住為你歡呼！

這是一個極具歷史意義的時刻！你的 Simple OS 正式從「純鍵盤輸入的批次處理時代」，跨越到了「人類直覺互動的圖形指標時代」。你現在已經具備了現代作業系統最核心的互動靈魂！

既然我們有了畫布，也有了能在上面自由穿梭的滑鼠，接下來 **Day 54** 的任務呼之欲出：**打造桌面環境與視窗 (Desktop Environment & Windows)！** 🖼️



一個真正的 GUI 系統不能只有黑漆漆的背景和終端機文字。我們要來畫出充滿復古 Windows 95 風格的「桌面底色」以及帶有「標題列」的視窗！

請跟著我進行這 3 個步驟，讓你的 OS 擁有真正的「顏值」：

---

### 步驟 1：升級圖形引擎，學會「寫字」與「畫視窗」

我們現在只有 `draw_char`（畫單一字母），但畫視窗標題時，我們需要能直接在特定座標畫出「一整串字」。

請打開 **`lib/include/gfx.h`**，新增這兩個宣告：
```c
// 在特定座標畫出一串字
void draw_string(int x, int y, const char* str, uint32_t fg_color, uint32_t bg_color);

// 畫出一個標準的 GUI 視窗
void draw_window(int x, int y, int width, int height, const char* title);
```

接著，打開 **`lib/gfx.c`**，在檔案下方（游標系統之上）加入這兩個函式的實作：

```c
// 【新增】在指定座標畫出連續字串
void draw_string(int x, int y, const char* str, uint32_t fg_color, uint32_t bg_color) {
    int curr_x = x;
    for (int i = 0; str[i] != '\0'; i++) {
        draw_char(str[i], curr_x, y, fg_color, bg_color);
        curr_x += 8; // 每個字元寬 8 像素
    }
}

// 【新增】畫出帶有標題列的復古視窗！
void draw_window(int x, int y, int width, int height, const char* title) {
    // 1. 視窗主體 (經典的亮灰色 0xC0C0C0)
    draw_rect(x, y, width, height, 0xC0C0C0);
    
    // 2. 視窗邊框 (簡單的立體陰影)
    // 上邊與左邊 (白色反光)
    draw_rect(x, y, width, 2, 0xFFFFFF); 
    draw_rect(x, y, 2, height, 0xFFFFFF);
    // 下邊與右邊 (深灰色陰影)
    draw_rect(x, y + height - 2, width, 2, 0x808080);
    draw_rect(x + width - 2, y, 2, height, 0x808080);

    // 3. 標題列 (經典的深藍色 0x000080)
    // 留出邊框的空間 (x+2, y+2)，高度設為 18 像素
    draw_rect(x + 2, y + 2, width - 4, 18, 0x000080);

    // 4. 標題文字 (白色字，深藍色底)
    // 微調座標讓字體置中於標題列
    draw_string(x + 6, y + 7, title, 0xFFFFFF, 0x000080);
}
```

---

### 步驟 2：改造終端機底色 (`lib/tty.c`)

為了讓桌面有「桌面」的感覺，我們不再把整個螢幕塗黑了。我們要把它塗成經典的「Windows 95 藍綠色 (Teal)」。

請打開 **`lib/tty.c`**，找到 `terminal_initialize`，把底色換掉：

```c
// 把這行原本的黑色換成深青色 (Teal)
#define TERM_BG 0x008080 

void terminal_initialize(void) {
    term_x = 0;
    term_y = 0;
    // 這樣一來，背景就會變成漂亮的復古藍綠色桌面！
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, TERM_BG); 
}
```

---

### 步驟 3：在桌面上開啟你的第一個視窗 (`lib/kernel.c`)

現在，我們要指揮 Kernel 在開機時，幫我們把這個視窗畫出來！

**⚠️ 關鍵細節：** 繪製順序非常重要！我們必須在「啟動滑鼠 (`init_mouse`)」**之前**畫好視窗。因為滑鼠一啟動，就會擷取當時的「背景」當作游標底圖。如果先啟動滑鼠再畫視窗，滑鼠的底圖就會出錯。

請打開 **`lib/kernel.c`**，修改 `kernel_main`：

```c
void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
    (void)magic; 

    // terminal_initialize 會幫我們把背景塗成藍綠色桌面
    terminal_initialize(); 
    
    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384);
    init_kheap();

    // 1. 初始化圖形引擎
    init_gfx(mbd);

    // ==========================================
    // 2. 🎨 繪製 GUI 元素 (必須在啟動滑鼠之前！)
    // ==========================================
    // 在畫面右半邊，畫一個系統狀態視窗！
    draw_window(450, 100, 300, 200, "System Status");
    // 在視窗裡面寫點字 (底色要配重視窗的灰色 0xC0C0C0)
    draw_string(460, 130, "CPU: x86 32-bit", 0x000000, 0xC0C0C0);
    draw_string(460, 150, "Memory: 16 MB", 0x000000, 0xC0C0C0);
    draw_string(460, 170, "GUI: Active", 0x000000, 0xC0C0C0);

    // 3. 啟動滑鼠驅動 (它會擷取畫好視窗後的背景)
    init_mouse();

    __asm__ volatile ("sti");

    // 左上角的終端機文字會自然地印在藍綠色的桌面上
    kprintf("=== OS Subsystems Ready ===\n\n");
    kprintf("Welcome to Simple OS Desktop Environment!\n");

    while (1) { __asm__ volatile ("hlt"); }
}
```

---

### 🚀 迎接你的視窗時代

存檔，執行你的光榮指令：
```bash
make clean && make run
```

當 QEMU 亮起時，你將會看到一個極具年代感的藍綠色桌面、流暢的黑色打字終端機，以及右邊那個帶有立體邊框和深藍色標題列的「System Status」視窗！
最棒的是，你的滑鼠可以完美地在這個視窗上面滑動！

這代表你的系統已經正式升級為「桌面作業系統 (Desktop OS)」了！成功畫出視窗後記得截圖給我看，接下來我們要思考怎麼讓這些視窗「動起來」了！😎


---


哈哈，Rick！看到這張完美的截圖，真的讓人熱血沸騰！那個經典的藍色標題列加上亮灰色的視窗主體，簡直完美復刻了 90 年代 GUI 的黃金歲月！🎉

你的直覺非常敏銳！從現在開始，你確實可以把作業系統的 GUI 開發，當成是在寫一個底層的 **2D 遊戲引擎**。
視窗（Window）就像是場景裡的 Sprite（精靈圖），滑鼠游標是玩家的操作實體，判斷滑鼠有沒有點到按鈕，本質上就是 2D 遊戲裡最經典的 **AABB 碰撞偵測 (Bounding Box Collision)**。而處理多個視窗的重疊，也就是遊戲中 Z-Index 的排序（作業系統稱之為 Painter's Algorithm 畫家演算法）！

既然我們已經有了 2D 開發的思維，**Day 54** 的任務就順理成章了：**打造視窗管理員 (Window Manager)，讓視窗可以被滑鼠拖曳！** 🖱️🖼️

目前我們的視窗只是「印」在背景上的死像素。我們要把它變成一個真正的「資料結構」，並且加入碰撞偵測，讓它「活」過來。

請跟著我進行這 3 個步驟：

---

### 步驟 1：定義視窗的資料結構 (`lib/include/gui.h` & `lib/gui.c`)

我們要建立一個專門管理 GUI 的模組，把視窗抽象化成類似物件的概念。

1. 新增 **`lib/include/gui.h`**：
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

// 畫面合成器 (Compositor)：重繪整個桌面與所有視窗
void gui_render(void);

// 取得全域視窗陣列的指標 (給滑鼠偵測用)
window_t* get_windows(void);

#endif
```

2. 新增 **`lib/gui.c`**：
```c
#include "gui.h"
#include "gfx.h"
#include "utils.h"
#include "tty.h"

#define MAX_WINDOWS 10
#define TERM_BG 0x008080 // 桌面底色

static window_t windows[MAX_WINDOWS];
static int window_count = 0;

void init_gui(void) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i].is_active = 0;
    }
}

int create_window(int x, int y, int width, int height, const char* title) {
    if (window_count >= MAX_WINDOWS) return -1;
    
    int id = window_count;
    windows[id].id = id;
    windows[id].x = x;
    windows[id].y = y;
    windows[id].width = width;
    windows[id].height = height;
    strcpy(windows[id].title, title);
    windows[id].is_active = 1;
    
    window_count++;
    return id;
}

window_t* get_windows(void) {
    return windows;
}

// 【畫面合成器 Compositor】這就是我們的 2D Render Loop！
void gui_render(void) {
    // 1. 清空背景 (畫上桌面底色)
    draw_rect(0, 0, 800, 600, TERM_BG);

    // 2. 畫出所有活躍的視窗 (Painter's Algorithm：先畫的會在底層)
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_active) {
            draw_window(windows[i].x, windows[i].y, windows[i].width, windows[i].height, windows[i].title);
            
            // 為了示範，我們幫第一個視窗寫死一些內容
            if (i == 0) {
                draw_string(windows[i].x + 10, windows[i].y + 30, "CPU: x86 32-bit", 0x000000, 0xC0C0C0);
                draw_string(windows[i].x + 10, windows[i].y + 50, "Memory: 16 MB", 0x000000, 0xC0C0C0);
                draw_string(windows[i].x + 10, windows[i].y + 70, "GUI: Active", 0x000000, 0xC0C0C0);
            }
        }
    }
}
```

---

### 步驟 2：加入滑鼠點擊與拖曳邏輯 (`lib/mouse.c`)

現在是最精彩的碰撞偵測！PS/2 滑鼠傳來的封包中，`mouse_byte[0]` 的最低位元 (Bit 0) 代表**左鍵是否被按下**。

打開 **`lib/mouse.c`**，在最上方引入 `gui.h`，然後修改 `mouse_handler` 裡面的 `case 2:`：

```c
#include "gui.h"

// 紀錄目前正在拖曳哪個視窗 (-1 代表沒有)
static int dragged_window_id = -1; 

void mouse_handler(void) {
    uint8_t status = inb(0x64);
    while (status & 0x01) { 
        int8_t mouse_in = inb(0x60);
        
        switch (mouse_cycle) {
            case 0:
                if (mouse_in & 0x08) { 
                    mouse_byte[0] = mouse_in;
                    mouse_cycle++;
                }
                break;
            case 1:
                // ... 保持不變
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
                window_t* wins = get_windows();

                if (left_click) {
                    if (dragged_window_id == -1) {
                        // 剛按下去，偵測有沒有點到某個視窗的「標題列」
                        for (int i = 0; i < 10; i++) {
                            if (wins[i].is_active) {
                                // AABB 碰撞偵測：滑鼠座標是否在標題列的範圍內？
                                // 標題列區域大約是 (x, y) 到 (x + width, y + 20)
                                if (mouse_x >= wins[i].x && mouse_x <= wins[i].x + wins[i].width &&
                                    mouse_y >= wins[i].y && mouse_y <= wins[i].y + 20) {
                                    dragged_window_id = i; // 抓到了！開始拖曳
                                    break;
                                }
                            }
                        }
                    } else {
                        // 已經在拖曳中，更新視窗座標
                        // (因為 Y 軸顛倒，所以這裡是 -= dy)
                        wins[dragged_window_id].x += dx;
                        wins[dragged_window_id].y -= dy;
                        
                        // 視窗位置改變了，通知合成器重繪整個畫面！
                        gui_render(); 
                    }
                } else {
                    // 左鍵放開，停止拖曳
                    dragged_window_id = -1;
                }

                // 畫布重繪後，最後再把游標補上去 (蓋在最上層)
                draw_cursor(mouse_x, mouse_y);
                break;
        }
        status = inb(0x64); 
    }
    outb(0xA0, 0x20); 
    outb(0x20, 0x20); 
}
```

---

### 步驟 3：在開機時註冊視窗 (`lib/kernel.c`)

最後，我們把之前寫死的畫圖邏輯移除，改用我們優雅的 GUI 系統來管理。

打開 **`lib/kernel.c`**：
```c
#include "gui.h" // 記得引入

void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
    // ...
    init_kheap();

    init_gfx(mbd);
    
    // 【新增】初始化 GUI 系統並註冊視窗
    init_gui();
    create_window(450, 100, 300, 200, "System Status");
    create_window(50, 50, 350, 250, "Simple OS Terminal"); // 多開一個視窗試試！
    
    // 首次渲染全畫面
    gui_render();

    init_mouse();

    __asm__ volatile ("sti");
    
    // ...
    while (1) { __asm__ volatile ("hlt"); }
}
```

存檔，敲下：
```bash
make clean && make run
```

當系統啟動後，把你的滑鼠移動到「System Status」深藍色的標題列上，**按住左鍵不放並拖曳**。
見證奇蹟的時刻：視窗會完美地跟著你的滑鼠移動，而且原本被遮住的藍綠色桌面以及其他視窗，都會完好無缺地顯露出來！

你剛剛手搓出了一個作業系統級別的 Window Manager！快去玩玩看，拖曳成功的話請務必告訴我感覺如何！🚀
