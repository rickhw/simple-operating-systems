哈哈，Rick！你這句話簡直是神級吐槽，也是所有 GUI 開發者都會經歷的「頓悟時刻」！😂

你說得完全正確：**「這就跟 2D Game 開發一樣！」**
現代的作業系統桌面（Compositor），本質上就是一個超級大型的 2D 遊戲引擎。視窗是 Sprite，滑鼠是 Player，點擊是 Raycasting/Collision Detection。

既然是遊戲引擎，你馬上就遇見了遊戲開發最經典的兩個宿敵：
1. **閃爍 (Flickering/Tearing)**：因為我們直接把像素畫在螢幕 (VRAM) 上，畫到一半時螢幕剛好更新，玩家就會看到「半成品」的畫面。
2. **覆蓋遺忘 (Z-Order/Redraw Issue)**：`kprintf` 把字畫在畫布上，結果 `gui_render` 一抹上藍綠色背景，字就灰飛煙滅了，因為我們的作業系統「沒有記憶」！


為了徹底解決這兩個問題，我們要在 **Day 55** 引入遊戲開發的終極法寶：**「雙重緩衝 (Double Buffering)」與「全畫面渲染迴圈 (Render Loop)」！**

這會讓我們的架構變得無比乾淨，而且我們甚至可以**把原本醜陋的滑鼠背景還原程式碼 (`cursor_bg`) 直接刪掉**！請跟著我進行這場超爽快的重構：

---

### 步驟 1：實作雙重緩衝區 (`lib/gfx.c` & `lib/include/gfx.h`)

我們要在記憶體裡開一塊跟螢幕一樣大的隱形畫布（Back Buffer）。所有的畫圖都畫在這塊隱形畫布上，畫完之後，一次性瞬間複製到螢幕上（Swap）。

1. 打開 **`lib/include/gfx.h`**，新增這三個函數：
```c
void gfx_swap_buffers(void);
void draw_char_transparent(char c, int x, int y, uint32_t fg_color);
// draw_cursor 依然保留
```

2. 打開 **`lib/gfx.c`**，替換為以下引入了 Back Buffer 的實作：
```c
#include "gfx.h"
#include "paging.h"
#include "tty.h"
#include "font8x8.h"
#include "utils.h"

static uint8_t* fb_addr = 0;
static uint32_t fb_pitch = 0;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint8_t  fb_bpp = 0;

// 【核心魔法】隱形畫布 (Back Buffer)
// 800x600 * 4 Bytes = 約 1.92 MB。我們把它放在 BSS 區段，既安全又快速！
static uint32_t back_buffer[800 * 600]; 

// (保留原有的 cursor_bmp 陣列，但 cursor_bg 和 cursor_x, cursor_y 可以刪除了)
static const uint8_t cursor_bmp[10][10] = {
    {1,1,0,0,0,0,0,0,0,0}, {1,2,1,0,0,0,0,0,0,0}, {1,2,2,1,0,0,0,0,0,0},
    {1,2,2,2,1,0,0,0,0,0}, {1,2,2,2,2,1,0,0,0,0}, {1,2,2,2,2,2,1,0,0,0},
    {1,2,2,2,2,2,2,1,0,0}, {1,2,2,1,1,1,1,1,1,0}, {1,1,1,0,0,0,0,0,0,0},
    {1,0,0,0,0,0,0,0,0,0}
};

void init_gfx(multiboot_info_t* mbd) {
    if (mbd->flags & (1 << 12)) {
        fb_addr = (uint8_t*) (uint32_t) mbd->framebuffer_addr;
        fb_pitch = mbd->framebuffer_pitch;
        fb_width = mbd->framebuffer_width;
        fb_height = mbd->framebuffer_height;
        fb_bpp = mbd->framebuffer_bpp;

        uint32_t fb_size = fb_pitch * fb_height;
        for (uint32_t i = 0; i < fb_size; i += 4096) {
            map_vram((uint32_t)fb_addr + i, (uint32_t)fb_addr + i);
        }

        uint32_t cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
        __asm__ volatile("mov %0, %%cr3" : : "r"(cr3));
    }
}

// 【修改】現在所有的畫筆，都畫在隱形的 back_buffer 上！
void put_pixel(int x, int y, uint32_t color) {
    if (x < 0 || (uint32_t)x >= fb_width || y < 0 || (uint32_t)y >= fb_height) return;
    back_buffer[y * fb_width + x] = color;
}

uint32_t get_pixel(int x, int y) {
    if (x < 0 || (uint32_t)x >= fb_width || y < 0 || (uint32_t)y >= fb_height) return 0;
    return back_buffer[y * fb_width + x];
}

// 【新增】瞬間交換畫布！(消除閃爍的終極武器)
void gfx_swap_buffers() {
    if (fb_addr == 0) return;
    // 逐行將 back_buffer 複製到實體的 Framebuffer
    for (uint32_t y = 0; y < fb_height; y++) {
        memcpy(fb_addr + (y * fb_pitch), &back_buffer[y * fb_width], fb_width * 4);
    }
}

// (保留原有的 draw_rect, draw_char, draw_string, draw_window) ...
// (此處省略以節省篇幅，保留你原本 Day 53/54 寫好的那四個函數)

// 【新增】畫透明底色的字元 (給終端機疊加在背景上用)
void draw_char_transparent(char c, int x, int y, uint32_t fg_color) {
    if (c < 32 || c > 126) return;
    const uint8_t* glyph = font8x8[c - 32];
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (glyph[row] & (0x80 >> col)) {
                put_pixel(x + col, y + row, fg_color);
            }
        }
    }
}

// 【修改】滑鼠游標現在不需要備份背景了！直接畫上去！
void draw_cursor(int new_x, int new_y) {
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            if (cursor_bmp[i][j] == 1) put_pixel(new_x + j, new_y + i, 0xFFFFFF);
            else if (cursor_bmp[i][j] == 2) put_pixel(new_x + j, new_y + i, 0x000000);
        }
    }
}
```

---

### 步驟 2：讓 TTY 擁有記憶能力 (`lib/tty.c` & `lib/include/tty.h`)

為了不讓文字被背景蓋掉，我們的 `tty` 必須用一個陣列把「印過的字」記下來，並開放一個 `tty_render()` 讓 GUI 引擎在最後一刻把字疊上去。

1. 在 **`lib/include/tty.h`** 新增宣告：
```c
void tty_render(void);
```

2. 打開 **`lib/tty.c`**，把文字記錄下來：
```c
// ... 前面保留 ...
#define TERM_FG 0xFFFFFF
#define MAX_ROWS (600 / 8) // 75 行
#define MAX_COLS (800 / 8) // 100 列

static int term_x = 0;
static int term_y = 0;

// 【新增】文字記憶緩衝區
static char text_buffer[MAX_ROWS][MAX_COLS];

void terminal_initialize(void) {
    term_x = 0;
    term_y = 0;
    // 清空文字緩衝區
    for (int r = 0; r < MAX_ROWS; r++) {
        for (int c = 0; c < MAX_COLS; c++) {
            text_buffer[r][c] = '\0';
        }
    }
    // 不再這裡畫背景了，交給 GUI！
}

void terminal_putchar(char c) {
    if (c == '\n') {
        term_x = 0;
        term_y += 8; 
        if (term_y >= 600) terminal_initialize(); 
        return;
    }
    if (c == '\b') {
        if (term_x >= 8) {
            term_x -= 8;
            text_buffer[term_y / 8][term_x / 8] = '\0'; // 抹除記憶
        }
        return;
    }

    // 記在陣列裡，不再直接畫！
    text_buffer[term_y / 8][term_x / 8] = c;
    term_x += 8;

    if (term_x >= 800) {
        term_x = 0;
        term_y += 8;
        if (term_y >= 600) terminal_initialize();
    }
}

// 【新增】把記憶體裡的字，畫在目前的畫布上
void tty_render(void) {
    for (int r = 0; r < MAX_ROWS; r++) {
        for (int c = 0; c < MAX_COLS; c++) {
            char ch = text_buffer[r][c];
            if (ch != '\0' && ch != ' ') {
                draw_char_transparent(ch, c * 8, r * 8, TERM_FG);
            }
        }
    }
}

// ... 下面的 terminal_write, kprintf 保持不變 ...
```

---

### 步驟 3：完美的遊戲渲染迴圈 (`lib/gui.c` & `lib/include/gui.h`)

現在，我們要把所有東西組裝成一個完美的 Render Loop！

1. 在 **`lib/include/gui.h`** 修改 `gui_render` 的宣告，讓它接收滑鼠座標：
```c
void gui_render(int mouse_x, int mouse_y);
```

2. 打開 **`lib/gui.c`**，打造終極渲染管道：
```c
// ... 前面保持不變 ...

// 【終極遊戲渲染迴圈】
void gui_render(int mouse_x, int mouse_y) {
    // 1. 畫背景 (Clear Screen)
    draw_rect(0, 0, 800, 600, TERM_BG);

    // 2. 畫視窗 (Draw Sprites)
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_active) {
            draw_window(windows[i].x, windows[i].y, windows[i].width, windows[i].height, windows[i].title);
            
            if (i == 0) {
                draw_string(windows[i].x + 10, windows[i].y + 30, "CPU: x86 32-bit", 0x000000, 0xC0C0C0);
                draw_string(windows[i].x + 10, windows[i].y + 50, "Memory: 16 MB", 0x000000, 0xC0C0C0);
                draw_string(windows[i].x + 10, windows[i].y + 70, "GUI: Active", 0x000000, 0xC0C0C0);
            }
        }
    }

    // 3. 畫終端機文字 (Draw UI Layer)
    tty_render();

    // 4. 畫滑鼠游標 (Draw Cursor On Top)
    draw_cursor(mouse_x, mouse_y);

    // 5. 瞬間交換畫布！(Swap Buffers)
    gfx_swap_buffers();
}
```

---

### 步驟 4：由滑鼠驅動世界 (`lib/mouse.c`)

最後，我們只要讓 `mouse_handler` 在每次滑鼠移動時，呼叫 `gui_render` 即可！

打開 **`lib/mouse.c`**，把 `case 2:` 最下面的邏輯改成這樣：
```c
                // 拖曳邏輯... (保留你昨天寫好的碰撞偵測與 dragged_window_id 更新)
                if (left_click) {
                    // ... 
                } else {
                    dragged_window_id = -1;
                }

                // 【修改】不管有沒有拖曳，只要滑鼠動了，就觸發整個世界的重繪！
                gui_render(mouse_x, mouse_y);
                break;
```

*(💡 註：記得在 `kernel.c` 裡面，開機最後的 `gui_render()` 也要補上初始參數，例如 `gui_render(400, 300);`)*

---

存檔，敲下：
```bash
make clean && make run
```

這一次，拿起你的滑鼠拖曳視窗吧！
**完全沒有閃爍、極致絲滑的拖曳體驗，而且終端機文字就像貼紙一樣死死地貼在桌面上，再也不會消失了！**

歡迎來到 Double Buffering 的奇妙世界！成功後請務必告訴我這個滑鼠手感有沒有讓你感動！😎
