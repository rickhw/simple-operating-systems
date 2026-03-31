哈哈，Rick！準備好迎接歷史性的一刻了嗎？😎

在 **Day 69**，我們要為你的 GUI 系統帶來革命性的升級：**「獨立畫布 (Window Canvas)」**！


目前，所有視窗的內容（例如 "System Status" 或終端機文字）都是 Kernel 在 `gui_render` 裡面「硬核寫死」的。這就像是電視台直接把字幕燒在畫面上，觀眾沒辦法自己換節目。
真正的圖形化作業系統，應該是：
1. **User 程式擁有自己的記憶體畫布**（一個二維陣列）。
2. User 程式在畫布上盡情作畫（畫點、畫線、渲染圖片）。
3. 畫完後，呼叫 Syscall 把畫布 **「推 (Flush)」** 給 Window Manager (Kernel)。
4. Kernel 的排程器在重繪螢幕時，把這塊畫布貼到對應的視窗框框裡！

這其實就是現代顯示伺服器（如 Linux 的 Wayland 或 macOS 的 Quartz）底層使用的 **「Double Buffering 雙重緩衝 + Compositor 合成器」** 架構！

跟著我實作這 4 個步驟，讓你的作業系統可以跑圖形化 App：

---

### 步驟 1：擴充 `window_t` 加入私有畫布 (`src/kernel/include/gui.h` & `lib/gui.c`)

我們要讓 Kernel 在建立視窗時，順便在 Heap 裡幫它準備一塊記憶體來存放像素資料。

**1. 打開 `src/kernel/include/gui.h`：**
```c
typedef struct {
    int id;
    int x;
    int y;
    int width;
    int height;
    char title[32];
    int is_active;
    int owner_pid;
    uint32_t* canvas; // 【Day 69 新增】視窗專屬的像素緩衝區
} window_t;
```

**2. 打開 `src/kernel/lib/gui.c`，修改 `create_window`：**
我們要分配記憶體給這塊畫布。為了扣掉邊框 (左右各 2px) 和標題列 (上 2px + 18px 標題 + 下 2px)，畫布的實際大小會比視窗小一點。
```c
// 在 create_window 裡面加入：
int create_window(int x, int y, int width, int height, const char* title, int owner_pid) {
    if (window_count >= MAX_WINDOWS) return -1;
    int id = window_count++;

    // ... 原本的賦值 ...
    windows[id].owner_pid = owner_pid;
    
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
    
    // ...
```

**3. 繼續在 `gui.c`，修改 `draw_window_internal`，把畫布印出來：**
在函數的最下方（`tty_render_window` 之前），加入畫布的渲染邏輯：
```c
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
```

---

### 步驟 2：開通 `sys_update_window` 系統呼叫 (`src/kernel/lib/syscall.c`)

當 User App 畫好圖後，它會呼叫這個 Syscall 把陣列傳給 Kernel。

打開 **`src/kernel/lib/syscall.c`**，新增 Syscall 27：
```c
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
```

---

### 步驟 3：在 User Space 封裝 API (`src/user/include/unistd.h` & `lib/unistd.c`)

**1. `unistd.h` 增加宣告：**
```c
void update_gui_window(int win_id, unsigned int* buffer);
```

**2. `unistd.c` 加入實作：**
```c
void update_gui_window(int win_id, unsigned int* buffer) {
    int ret;
    __asm__ volatile (
        "int $128" 
        : "=a" (ret) 
        : "a" (27), "b" (win_id), "c" (buffer)
    );
}
```

---

### 步驟 4：開發第一支「真・圖形應用程式」 (`src/user/bin/paint.c`)

我們來寫一支名為 `paint.elf` 的程式，它會即時渲染出一個會動的彩色漸層圖案！

建立 **`src/user/bin/paint.c`**：

```c
#include "stdio.h"
#include "unistd.h"

// 定義我們想要的畫布大小
#define CANVAS_W 200
#define CANVAS_H 150

int main() {
    // 申請視窗：總寬度要加上邊框 (4)，總高度要加上標題列與邊框 (24)
    int win_id = create_gui_window("Dynamic Paint", CANVAS_W + 4, CANVAS_H + 24);
    if (win_id < 0) {
        printf("Failed to create window.\n");
        return -1;
    }

    // 這就是我們在 User Space 的專屬畫布！
    unsigned int my_canvas[CANVAS_W * CANVAS_H];
    int offset = 0;

    while (1) {
        // 1. 在畫布上作畫 (產生會動的彩色漸層 / XOR 紋理)
        for (int y = 0; y < CANVAS_H; y++) {
            for (int x = 0; x < CANVAS_W; x++) {
                // RGB 顏色計算
                unsigned int r = (x + offset) % 255;
                unsigned int g = (y + offset) % 255;
                unsigned int b = (x ^ y) % 255; // 經典的 XOR 紋理
                
                // 將 RGB 組合合成一個 32-bit 的像素值 (0x00RRGGBB)
                my_canvas[y * CANVAS_W + x] = (r << 16) | (g << 8) | b;
            }
        }

        // 2. 把畫布交給 Kernel 去更新螢幕！
        update_gui_window(win_id, my_canvas);

        // 3. 推進動畫影格，並稍微讓出 CPU 休息一下
        offset += 3;
        for(volatile int i = 0; i < 500000; i++); 
    }

    return 0;
}
```

*(💡 最後，別忘了在 `kernel.c` 的 `setup_filesystem` 陣列與 `grub.cfg` 加入 `"paint.elf"` 喔！)*

---

### 🚀 驗收：見證圖形化多工的奇蹟！

存檔、編譯、執行 `make clean && make run`！

進入 Terminal 後，大膽地輸入：
```bash
paint &
```

👉 你會看到畫面上彈出一個名為 "Dynamic Paint" 的視窗。
👉 **視窗裡面會出現令人驚豔的彩色動畫，而且正不斷地即時運算流動！**
👉 更棒的是，因為它是背景執行，你可以繼續在 Terminal 裡面打字，或者再輸入一次 `paint &`，畫面上就會出現**第二個獨立運算的圖形視窗**！

Rick，這就是作業系統 GUI 的終極魅力！從字元終端機，一路走到擁有獨立事件迴圈、獨立緩衝區的動態視窗系統。享受一下這份成就感，如果效能有點卡，那是正常的（因為我們用 `draw_rect(1,1)` 軟體渲染），未來我們可以優化它。

欣賞完你的畫布後，告訴我你想在 **Day 70** 挑戰什麼！是要實作「時鐘與日期系統」還是開始打造「字體渲染引擎」呢？😎
