哈哈，Rick！你的吐槽簡直一針見血！「FPS = 10 的遊戲，一個字一個字蹦出來」😂

恭喜你，你親身踩到了所有圖形作業系統與遊戲引擎開發者都會遇到的終極效能瓶頸：**「同步渲染 (Synchronous Rendering) 的慘案」**！


### 🔍 案發現場解密：為什麼會變成樹懶？

還記得我們在 `tty.c` 寫了什麼嗎？
```c
void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++) terminal_putchar(data[i]);
    gui_redraw(); // 兇手就是它！
}
```
當你的 Ring 3 應用程式 `ls` 透過標準函式庫 `printf` 印字時，它底層是呼叫 `sys_write(1, &c, 1)`。這代表它**每印「一個字元」，就會呼叫一次 `gui_redraw()`**！

一次 `gui_redraw()` 包含了什麼？
清空背景 -> 畫出所有視窗 -> 畫出所有文字 -> 畫上工作列 -> 畫上滑鼠 -> **最後把 1.92 MB 的畫布複製到顯示卡 (VRAM)**。
當 `ls` 印出 500 個字，你的系統在一瞬間搬移了 **將近 1 GB 的記憶體**！難怪它會像打字機一樣慢！

### 🛠️ 終極重構：非同步合成器 (Async Compositor)

真正的作業系統 (例如 Windows 的 DWM 或是 Linux 的 Wayland) 絕對不會在「印字」的時候去「畫螢幕」。它們會把這兩個動作**脫鉤 (Decouple)**！
1. **打字/滑鼠移動**：只負責把狀態寫進記憶體，並舉起一個「髒標記 (Dirty Flag)」，說：「嘿！畫面髒了，該更新囉！」
2. **系統時鐘 (Timer IRQ0)**：每秒跳動 100 次的系統心跳。它會負責檢查「畫面髒了嗎？」，如果髒了，它才統一進行**唯一一次**的螢幕大更新！

我們現在就把這個「遊戲級渲染迴圈」實作進 Simple OS：

---

#### 步驟 1：升級 GUI 標頭檔 (`lib/include/gui.h`)

打開 **`lib/include/gui.h`**，新增這兩個非同步 API：
```c
// ...
void gui_render(int mouse_x, int mouse_y);
void gui_redraw(void);

// 【新增】非同步渲染架構 API
void gui_handle_timer(void);
void gui_update_mouse(int x, int y);
// ...
```

---

#### 步驟 2：實作髒標記與互斥鎖 (`lib/gui.c`)

打開 **`lib/gui.c`**，我們要在最上方加入標記，並改寫重繪邏輯：

```c
// ... 在最上方的 static 變數區新增：
static int last_mouse_x = 400;
static int last_mouse_y = 300;

// 【新增】非同步渲染的靈魂：髒標記與渲染鎖！
static volatile int gui_dirty = 1;     // 1 代表畫面需要更新
static volatile int is_rendering = 0;  // 1 代表正在畫圖中，避免重複進入

// ... 中間的 init_gui, draw_window_internal, gui_render 保持完全不變 ...

// ==========================================
// 【大幅修改】現在它只做「標記」，0.0001 毫秒就執行完，絕對不卡 CPU！
// ==========================================
void gui_redraw(void) {
    gui_dirty = 1; 
}

// 【新增】給滑鼠專用的更新函式
void gui_update_mouse(int x, int y) {
    last_mouse_x = x;
    last_mouse_y = y;
    gui_dirty = 1; // 滑鼠動了，畫面也髒了
}

// 【新增】真正的渲染引擎，交給系統時鐘來呼叫！(約 60~100 FPS)
void gui_handle_timer(void) {
    // 如果畫面是髒的，而且目前「沒有人正在渲染」
    if (gui_dirty && !is_rendering) {
        is_rendering = 1; // 上鎖 (防止 Timer 中斷重疊引發 Kernel Panic)
        
        gui_render(last_mouse_x, last_mouse_y);
        
        gui_dirty = 0;    // 清除髒標記
        is_rendering = 0; // 解鎖
    }
}
```

---

#### 步驟 3：解除滑鼠的同步封印 (`lib/mouse.c`)

打開 **`lib/mouse.c`**，在 `case 2:` 最下方，把原本呼叫 `gui_render` 的地方換成新的 API。這不僅解決了效能問題，還解決了潛在的**中斷競爭危害 (Race Condition)**！

```c
                // ... 前面的拖曳邏輯保持不變 ...
                else {
                    dragged_window_id = -1;
                }

                prev_left_click = left_click; 
                
                // 【修改】不要在這裡直接 Render 了！只更新座標並標記 Dirty！
                gui_update_mouse(mouse_x, mouse_y);
                break;
        }
        status = inb(0x64); 
    }
    // ... 發送 EOI ...
```

---

#### 步驟 4：將渲染引擎掛載到系統心跳上 (`lib/timer.c`)

最後一步，我們要讓系統時鐘來接管畫布！
請打開你久違的 **`lib/timer.c`** (管理 PIT IRQ0 的檔案)，在它的中斷處理程式裡加上這行：

```c
#include "timer.h"
#include "isr.h"
#include "gui.h" // 【新增】引入 GUI

// 你的 timer_handler 名稱可能略有不同，找那個會被 IRQ0 呼叫的函式
void timer_handler(registers_t *regs) {
    // ... 原本的 tick++ ...

    // 【核心新增】每一滴答 (Tick)，檢查並刷新畫面！
    gui_handle_timer();

    // ... 原本呼叫 schedule() 的地方 ...
}
```

*(💡 備註：你不需要改 `tty.c` 或 `utils.c` 的 `gui_redraw()`，因為它們現在呼叫的 `gui_redraw()` 已經變成只會瞬間切換標籤的輕量級函式了！)*

---

存檔，執行！
```bash
make clean && make run
```

這一次再輸入 `ls`，你會體驗到什麼叫做**「瞬間彈出」的 60 FPS 終極流暢感**！印 1000 個字元的效能消耗，從原本的「畫 1000 次螢幕」，瞬間降維打擊變成「畫 1 次螢幕」！

這就是現代作業系統 Window Manager 最核心的架構。你已經完全掌握了 GUI 的任督二脈！享受一下這個絲滑的終端機吧，接下來你想讓桌面增加什麼新功能呢？
