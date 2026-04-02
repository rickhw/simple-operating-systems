太神啦，Rick！🎉 看到這張截圖我真的超感動！

左邊是受到完美保護的 Terminal，印出那句霸氣的 `[Kernel] Segmentation Fault at 0x0`；右邊是正在背景獨立運作、畫著 XOR 迷幻漸層的 `paint.elf`；中間還有我們親手刻出來的 System Status。
這畫面證明了你的 OS 不僅具備了**多工 (Multitasking)**、**圖形化 (GUI)**，現在更擁有了現代作業系統最重要的**記憶體保護機制 (Memory Protection)**！

這絕對是你 OS 開發旅程中值得截圖裱框的一刻！

既然視窗都能乖乖在背景畫圖了，但它們現在像被釘死在螢幕上一樣，不能移動，這怎麼能稱得上是真正的 Desktop OS 呢？
**Day 71：讓視窗飛起來！(Window Dragging 視窗拖曳)** 我們今天要把「滑鼠狀態機」加入 GUI 引擎，讓你只要按住視窗的「標題列」，就能自由拖曳視窗！請跟著我實作這 3 個步驟：

---

### 步驟 1：在 GUI 引擎加入「拖曳狀態」 (`src/kernel/lib/gui.c`)

我們要讓 Window Manager 記住「現在滑鼠抓著哪個視窗」以及「抓著的相對座標（偏移量）」。

打開 **`src/kernel/lib/gui.c`**，在最上方的全域變數區加入這三個變數：

```c
// ... 原本的變數 ...
static int focused_window_id = -1;  
static int start_menu_open = 0;     
static int gui_mode_enabled = 1;    

// ==========================================
// 【Day 71 新增】視窗拖曳狀態追蹤
// ==========================================
static int dragged_window_id = -1; // 目前正在被拖曳的視窗 (-1 代表沒有)
static int drag_offset_x = 0;      // 滑鼠點擊位置與視窗左上角的 X 距離
static int drag_offset_y = 0;      // 滑鼠點擊位置與視窗左上角的 Y 距離
```

---

### 步驟 2：升級滑鼠處理邏輯 (`src/kernel/lib/gui.c`)

我們要修改原本處理滑鼠的 API。當滑鼠「按下」時判斷是否點到標題列；當滑鼠「移動」時更新視窗座標；當滑鼠「放開」時解除拖曳。

在 **`gui.c`** 中，修改 `gui_update_mouse` 並新增兩個新函數 `gui_mouse_down` 與 `gui_mouse_up`：

```c
// ==========================================
// 【Day 71 修改】滑鼠移動時，如果有拖曳視窗，就跟著更新座標！
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
// 【Day 71 新增】滑鼠「左鍵按下」事件
// ==========================================
void gui_mouse_down(int x, int y) {
    // 先執行原本的點擊檢查 (關閉按鈕、Start 選單、切換焦點等)
    int handled = gui_check_ui_click(x, y);
    
    if (handled) return; // 如果點到 [X] 或是按鈕，就不處理拖曳了

    // 檢查是否點擊了某個視窗的「標題列」來啟動拖曳
    // (我們從最上層的 focused_window_id 開始檢查會比較符合直覺，這裡簡單化處理)
    if (focused_window_id != -1 && windows[focused_window_id].is_active) {
        window_t* win = &windows[focused_window_id];
        
        // 標題列的範圍：Y 座標在視窗頂端往下 20px 內
        if (x >= win->x && x <= win->x + win->width &&
            y >= win->y && y <= win->y + 20) {
            
            dragged_window_id = win->id;
            drag_offset_x = x - win->x;
            drag_offset_y = y - win->y;
            return;
        }
    }
}

// ==========================================
// 【Day 71 新增】滑鼠「左鍵放開」事件
// ==========================================
void gui_mouse_up(void) {
    // 放開滑鼠，解除拖曳狀態
    dragged_window_id = -1;
}
```
*(💡 記得去 `src/kernel/include/gui.h` 把 `gui_mouse_down` 和 `gui_mouse_up` 加上宣告喔！)*

---

### 步驟 3：連接硬體滑鼠驅動 (`src/kernel/lib/mouse.c`)

最後，我們必須讓底層的 PS/2 滑鼠驅動知道「使用者什麼時候按下了左鍵」。在 PS/2 協定中，滑鼠送來的第一個 Byte（Packet[0]）的 **Bit 0** 就是左鍵狀態（1 代表按下，0 代表放開）。

打開你的 **`src/kernel/lib/mouse.c`**（或是你處理滑鼠中斷的地方 `mouse_handler`），加入狀態判斷：

```c
#include "gui.h"

// 紀錄上一次的左鍵狀態，用來偵測「剛按下」與「剛放開」的瞬間
static int last_left_btn = 0; 

// 在你的 mouse_handler 裡面 (通常是收集滿 3 個 bytes 後處理座標的地方)：
void mouse_handler(void) {
    // ... (前面從 inb(0x60) 讀取封包的邏輯保持不變) ...

    if (mouse_cycle == 3) {
        mouse_cycle = 0;
        
        // 解析座標 (你原本的邏輯)
        // ... 計算出 new_mouse_x, new_mouse_y ...
        
        // 解析左鍵狀態 (Bit 0)
        int current_left_btn = mouse_byte[0] & 0x01;

        // 1. 處理滑鼠移動
        gui_update_mouse(new_mouse_x, new_mouse_y);

        // 2. 處理按鍵狀態改變 (Edge Triggering)
        if (current_left_btn == 1 && last_left_btn == 0) {
            // 剛按下左鍵！
            gui_mouse_down(new_mouse_x, new_mouse_y);
        } 
        else if (current_left_btn == 0 && last_left_btn == 1) {
            // 剛放開左鍵！
            gui_mouse_up();
        }

        last_left_btn = current_left_btn; // 更新歷史狀態
    }
    
    // ... (最後發送 EOI 給 PIC 的邏輯保持不變) ...
}
```
*(💡 註：請根據你實際 `mouse.c` 的變數名稱微調，核心概念就是利用 `packet[0] & 0x01` 判斷左鍵狀態，並在狀態改變時呼叫對應的 GUI 函數！)*

---

### 🚀 驗收：見證視窗的自由！

存檔、編譯並執行 `make clean && make run`！

進入 Desktop，點開好幾個視窗（例如 `status &`, `paint &`）。
現在，把你的滑鼠游標移動到任何一個視窗的深藍色標題列上。
**按住滑鼠左鍵，然後移動滑鼠！**

你將會看到視窗無比絲滑地跟著你的游標在螢幕上飛舞！更棒的是，因為你的 `paint.c` 是在另一個 Process 裡獨立運作，你會發現**在拖曳視窗的同時，裡面的彩色漸層動畫依然在順暢播放，完全不會卡頓！** 這就是多工搭配 Double Buffering 合成器所帶來的極致視覺享受！享受完拖曳的快感後，告訴我你想不想在 Day 72 挑戰「視窗重疊時的背景文字穿透修復」，或是「桌面時鐘的動態更新」？😎
