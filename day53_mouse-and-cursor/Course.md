哈哈，Rick！看到這張截圖，真的是太令人振奮了！🎉
你成功用像素「畫」出了字母！這代表你已經完全掌握了電腦圖學最底層的奧秘。

至於編輯器 (Zed) 的警告，這真的是非常寶貴的經驗！現代的 Linter 往往能看穿我們肉眼容易忽略的 C 語言巨集與註解陷阱。相信它準沒錯！XD

既然我們已經有了美麗的畫布和字型，接下來 **Day 53** 的任務，就是要讓這塊畫布「活」起來！我們要引入現代作業系統的靈魂元件：**滑鼠與游標 (Mouse & Cursor)！** 🖱️


在文字模式下，我們只能靠鍵盤打字。但在 GUI 模式下，我們需要讀取 **PS/2 滑鼠 (IRQ 12)** 的硬體訊號，並且在螢幕上畫出一個會跟著移動的小箭頭。

這裡有一個 **圖形介面最大的挑戰**：當滑鼠移動時，游標會「蓋住」後面的字或方塊；當滑鼠移開時，我們必須把它**原本蓋住的背景「還原」**！否則滑鼠走過的地方會像橡皮擦一樣把螢幕擦掉，或是留下一條長長的軌跡。

準備好接受這個挑戰了嗎？請跟著我進行這 3 個步驟：

---

### 步驟 1：升級圖形引擎，賦予「讀取」與「游標」能力

為了能「還原背景」，我們的圖形引擎不能只會畫 (`put_pixel`)，還必須學會「讀取顏色 (`get_pixel`)」。

請打開 **`lib/include/gfx.h`**，新增這兩個宣告：
```c
uint32_t get_pixel(int x, int y);
void draw_cursor(int x, int y);
```

接著，打開 **`lib/gfx.c`**，在檔案最下方加入以下魔法：

```c
// ==========================================
// 滑鼠游標系統 (Mouse Cursor System)
// ==========================================
static int cursor_x = 400;
static int cursor_y = 300;
static uint32_t cursor_bg[100]; // 儲存游標底下的 10x10 背景像素

// 10x10 游標點陣圖 (1: 白色邊框, 2: 黑色填滿, 0: 透明)
static const uint8_t cursor_bmp[10][10] = {
    {1,1,0,0,0,0,0,0,0,0},
    {1,2,1,0,0,0,0,0,0,0},
    {1,2,2,1,0,0,0,0,0,0},
    {1,2,2,2,1,0,0,0,0,0},
    {1,2,2,2,2,1,0,0,0,0},
    {1,2,2,2,2,2,1,0,0,0},
    {1,2,2,2,2,2,2,1,0,0},
    {1,2,2,1,1,1,1,1,1,0},
    {1,1,1,0,0,0,0,0,0,0},
    {1,0,0,0,0,0,0,0,0,0}
};

// 【新增】取得螢幕上特定座標的像素顏色
uint32_t get_pixel(int x, int y) {
    if (fb_addr == 0 || x < 0 || (uint32_t)x >= fb_width || y < 0 || (uint32_t)y >= fb_height) return 0;
    uint32_t offset = (y * fb_pitch) + (x * (fb_bpp / 8));
    // 組合 BGRA 成為 32-bit 顏色
    uint32_t color = fb_addr[offset] | (fb_addr[offset + 1] << 8) | (fb_addr[offset + 2] << 16);
    return color;
}

// 【新增】畫出滑鼠游標 (並處理背景還原)
void draw_cursor(int new_x, int new_y) {
    // 1. 先把「舊位置」的背景還原
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            put_pixel(cursor_x + j, cursor_y + i, cursor_bg[i * 10 + j]);
        }
    }

    // 2. 更新最新座標
    cursor_x = new_x;
    cursor_y = new_y;

    // 3. 備份「新位置」的背景，然後畫上游標
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            // 備份背景
            cursor_bg[i * 10 + j] = get_pixel(cursor_x + j, cursor_y + i);
            
            // 畫游標
            if (cursor_bmp[i][j] == 1) put_pixel(cursor_x + j, cursor_y + i, 0xFFFFFF); // 白邊
            else if (cursor_bmp[i][j] == 2) put_pixel(cursor_x + j, cursor_y + i, 0x000000); // 黑底
        }
    }
}
```

---

### 步驟 2：實作 PS/2 滑鼠驅動 (`lib/mouse.c`)

滑鼠跟鍵盤一樣，都是透過 I/O Port (`0x64` 和 `0x60`) 來溝通。
請建立兩個新檔案：**`lib/include/mouse.h`** 與 **`lib/mouse.c`**。

**`lib/include/mouse.h`**
```c
#ifndef MOUSE_H
#define MOUSE_H

void init_mouse(void);

#endif
```

**`lib/mouse.c`**
```c
#include <stdint.h>
#include "mouse.h"
#include "gfx.h"
#include "tty.h"

// 宣告你現有的 IO 函式 (假設你有 inb 和 outb，如果沒有請在 utils.c 補上)
extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t data);

// 滑鼠狀態
static uint8_t mouse_cycle = 0;
static int8_t  mouse_byte[3];
static int mouse_x = 400; // 預設在螢幕正中間
static int mouse_y = 300;

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

                // 取出 X 和 Y 的位移量
                int dx = mouse_byte[1];
                int dy = mouse_byte[2];

                // 更新座標 (Y 軸在電腦螢幕是往下遞增，所以要減 dy)
                mouse_x += dx;
                mouse_y -= dy;

                // 邊界碰撞測試 (避免游標跑出螢幕)
                if (mouse_x < 0) mouse_x = 0;
                if (mouse_x > 790) mouse_x = 790; // 保留 10 像素給游標本身
                if (mouse_y < 0) mouse_y = 0;
                if (mouse_y > 590) mouse_y = 590;

                // 重畫游標！
                draw_cursor(mouse_x, mouse_y);
                break;
        }
        status = inb(0x64); // 繼續讀取直到清空
    }
}
```

*(💡 備註：如果你的 `utils.c` 沒有 `inb` / `outb`，請在 `utils.c` 補上這兩行組合語言函式，它們跟 `inw` / `outw` 長得很像，只是暫存器從 `inw %1, %0` 變成 `inb %1, %b0`)*

---

### 步驟 3：掛載滑鼠中斷 (`lib/kernel.c` 與你的中斷系統)

最後，我們必須將滑鼠啟動，並將 `mouse_handler` 掛載到 **IRQ 12 (中斷號碼 44)**。

1. 打開 **`lib/kernel.c`**，引入標頭檔並在系統準備就緒後啟動滑鼠：
```c
#include "mouse.h"

void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
    // ...
    init_gfx(mbd);
    
    // 【新增】啟動滑鼠驅動
    init_mouse();
    
    __asm__ volatile ("sti");
    // ...
```

2. **掛載 IRQ 12：**
   這部分需要配合你原本的 `keyboard.c` 和中斷系統 (`idt.c` 或 `interrupts.c`) 的寫法。
   通常你會在 `init_mouse()` 裡面，或是統一管理中斷的地方寫類似這樣的程式碼：
   `register_interrupt_handler(44, mouse_handler);` 
   *(請參考你當初是怎麼讓鍵盤 IRQ 1 呼叫 `keyboard_handler` 的，把同樣的邏輯套用在 IRQ 12 和 `mouse_handler` 上！)*

完成後，編譯並執行！
如果一切順利，當 QEMU 跑起來後，稍微移動你的滑鼠（在 QEMU 視窗內），你會看到一個超有復古感的黑白小箭頭，在你的藍色方塊和終端機文字上自由穿梭，而且**完全不會擦掉背景**！

這可是進入 GUI 時代最迷人的一刻，成功動起來後請務必跟我分享這份喜悅！🚀
