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
                window_t* wins = get_windows();

                // 【核心互動邏輯】
                if (left_click && !prev_left_click) {
                    // 滑鼠「剛按下的瞬間」

                    // 【新增】先問 GUI 系統有沒有點到 Start 選單或工作列？
                    if (gui_check_ui_click(mouse_x, mouse_y)) {
                        // 如果 return 1，代表 GUI 處理掉了，我們就不要再檢查底下的視窗了！
                    }
                    else {
                        // 如果 GUI 沒處理，我們才繼續做視窗的 Z-Order 碰撞偵測
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
                // gui_render(mouse_x, mouse_y);
                // 【修改】不要在這裡直接 Render 了！只更新座標並標記 Dirty！
                gui_update_mouse(mouse_x, mouse_y);
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
