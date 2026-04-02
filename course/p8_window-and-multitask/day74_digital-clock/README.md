太神啦，Rick！🎉 你的作業系統現在不只能多工、有記憶體保護、能用滑鼠拖曳視窗，甚至連 BMP 圖片解碼渲染都做出來了！這套系統的完整度已經超越了許多大學 OS 課程的期末專題了！

既然你提到了「時鐘」，**Day 74：時間的魔法 (The Digital Clock Widget)**，我們就來打造一支有著駭客風格的「LED 數位時鐘」App，同時也把「右下角工作列時鐘不會自己動」的問題一併解決！

我們要做兩件事：
1. **喚醒 Kernel 的定時器**：讓桌面每秒自動重繪一次，工作列的時鐘就會自己跳動了。
2. **打造 `clock.elf`**：這會是你第一支完全用「像素陣列 (Pixel Array)」自己刻出 8-bit 字型的動態小工具！

請跟著我完成這 4 個步驟：

---

### 步驟 1：喚醒沉睡的視窗管理員 (`src/kernel/lib/gui.c`)

目前的 GUI 只有在你「動滑鼠」或「開視窗」時才會重繪（透過設定 `gui_dirty = 1`）。我們要讓系統的硬體 Timer 每隔一小段時間就自動標記畫面為髒，這樣工作列的時間就會自己更新了！

打開 **`src/kernel/lib/gui.c`**，找到最下方的 `gui_handle_timer`，加上 `tick` 的判斷：

```c
// 在檔案最上方或函式上方宣告外部的系統滴答數
extern volatile uint32_t tick;

// 真正的渲染引擎，交給系統時鐘來呼叫！
void gui_handle_timer(void) {
    // ==========================================
    // 【Day 74 新增】每隔約 1 秒 (100 ticks)，自動刷新畫面！
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
```

---

### 步驟 2：開通時間系統呼叫 (`src/kernel/lib/syscall.c`)

我們要讓 User Space 程式也能知道現在幾點。
打開 **`src/kernel/lib/syscall.c`**，新增 Syscall 28：

```c
// 在檔案最上方加入宣告：
extern void read_time(int* h, int* m);

// ... 在 syscall_handler 裡面新增：
    // ==========================================
    // 【Day 74 新增】Syscall 28: sys_get_time
    // ==========================================
    else if (eax == 28) {
        int* h = (int*)regs->ebx;
        int* m = (int*)regs->ecx;
        
        ipc_lock();
        read_time(h, m);
        regs->eax = 0;
        ipc_unlock();
    }
```

---

### 步驟 3：在 User Space 封裝 API (`src/user/include/unistd.h` & `lib/unistd.c`)

**1. `unistd.h` 增加宣告：**
```c
void get_time(int* h, int* m);
```

**2. `unistd.c` 加入實作：**
```c
void get_time(int* h, int* m) {
    syscall(28, (int)h, (int)m, 0, 0, 0);
}
```

---

### 步驟 4：打造賽博龐克數位時鐘 (`src/user/bin/clock.c`)

因為 User Space 沒有 `draw_string`，我們要發揮創客精神，利用一個簡單的 `3x5` 二維陣列，自己把數字 "畫" 在 Canvas 上！

建立 **`src/user/bin/clock.c`**：

```c
#include "stdio.h"
#include "unistd.h"

#define CW 120
#define CH 60

// 畫實心方塊的輔助函式 (直接操作像素陣列)
void draw_rect(unsigned int* canvas, int x, int y, int w, int h, unsigned int color) {
    for(int cy = y; cy < y + h; cy++) {
        for(int cx = x; cx < x + w; cx++) {
            if (cx >= 0 && cx < CW && cy >= 0 && cy < CH) {
                canvas[cy * CW + cx] = color;
            }
        }
    }
}

// 超迷你 3x5 像素字型引擎！(1 代表有顏色，0 代表透明)
void draw_digit(unsigned int* canvas, int digit, int ox, int oy, unsigned int color) {
    unsigned int font[10][5] = {
        {7,5,5,5,7}, // 0 (111, 101, 101, 101, 111)
        {2,2,2,2,2}, // 1 (010, 010, 010, 010, 010)
        {7,1,7,4,7}, // 2
        {7,1,7,1,7}, // 3
        {5,5,7,1,1}, // 4
        {7,4,7,1,7}, // 5
        {7,4,7,5,7}, // 6
        {7,1,1,1,1}, // 7
        {7,5,7,5,7}, // 8
        {7,5,7,1,7}  // 9
    };
    
    for(int y = 0; y < 5; y++) {
        for(int x = 0; x < 3; x++) {
            // 用 bitwise 取出每一格是 1 還是 0
            if (font[digit][y] & (1 << (2 - x))) {
                // 每一個邏輯像素，我們用 5x5 的真實像素來畫，讓它看起來變大！
                draw_rect(canvas, ox + x * 5, oy + y * 5, 5, 5, color);
            }
        }
    }
}

int main() {
    int win_id = create_gui_window("Digital Clock", CW + 4, CH + 24);
    if (win_id < 0) return -1;

    unsigned int* my_canvas = (unsigned int*)sbrk(CW * CH * 4);
    int blink = 0;

    while (1) {
        int h, m;
        get_time(&h, &m);

        // 1. 清空畫布 (填滿純黑)
        draw_rect(my_canvas, 0, 0, CW, CH, 0x000000);

        // 2. 畫小時 (十位數與個位數，綠色)
        draw_digit(my_canvas, h / 10, 15, 18, 0x00FF00);
        draw_digit(my_canvas, h % 10, 35, 18, 0x00FF00);

        // 3. 畫中間的冒號 (每秒閃爍一次)
        if (blink % 2 == 0) {
            draw_rect(my_canvas, 56, 23, 4, 4, 0x00FF00);
            draw_rect(my_canvas, 56, 33, 4, 4, 0x00FF00);
        }

        // 4. 畫分鐘
        draw_digit(my_canvas, m / 10, 65, 18, 0x00FF00);
        draw_digit(my_canvas, m % 10, 85, 18, 0x00FF00);

        // 5. 推送給 Window Manager
        update_gui_window(win_id, my_canvas);

        blink++;
        
        // 粗略等待半秒鐘 (讓冒號閃爍)
        for(volatile int i = 0; i < 3000000; i++); 
    }

    return 0;
}
```

*(💡 最後一步：別忘了在 `kernel.c` 的 `filenames[]` 陣列以及 `grub.cfg` 中加入 `"clock.elf"` 喔！)*

---

### 🚀 驗收：喚醒時間！

存檔執行 `make clean && make run`！

進入 Desktop 之後，你會立刻發現：**右下角工作列的時間，竟然會自己每分鐘跳動更新了！** (因為我們修復了 `gui_handle_timer` 的定時重繪機制)。

接著，在 Terminal 輸入：
```bash
clock &
```

👉 一個有著駭客螢光綠色字體的「數位時鐘」視窗會彈出來！
👉 它的中間會有一個冒號 `::` 每秒鐘不斷地閃爍，時間完美與右下角的系統時間同步！
👉 就算你把它拖曳到畫面的任何角落，它依然會一邊移動一邊閃爍更新！

這真的是一個超有質感的桌面小工具。享受一下這個會跳動的生命力，如果有什麼想加的功能（例如滑鼠雙擊開啟檔案？），隨時跟我說，我們繼續把這個 OS 堆向極致！😎
