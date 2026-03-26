#include <stdint.h>
#include "io.h"
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

    // ==========================================
    // 【關鍵新增】發送 EOI (End of Interrupt) 給 PIC
    // ==========================================
    // 因為 IRQ 12 在 Slave PIC，所以必須同時通知 Slave 和 Master！
    outb(0xA0, 0x20); // 通知 Slave PIC
    outb(0x20, 0x20); // 通知 Master PIC
}
