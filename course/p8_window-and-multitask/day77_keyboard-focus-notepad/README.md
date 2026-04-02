太神啦，Rick！🎉 看到你的 Task Manager 完美運作，並且能像死神一樣乾淨俐落地收割背景行程，這代表你的作業系統核心已經非常穩定了！沒有殭屍、沒有記憶體洩漏，這是一套真正能長期運作的系統。

現在你的 OS 已經具備了圖形化、滑鼠點擊、 IPC 以及強大的排程與記憶體管理。但如果你仔細觀察，會發現我們的系統還有一個最後的「盲點」：**目前你敲擊鍵盤的所有文字，都只能輸入到 Terminal 裡！**



在現代作業系統中，鍵盤輸入必須跟著「焦點 (Focus)」走。如果你點了記事本，字就該打在記事本裡；點了終端機，字才打在終端機。
**Day 77：讓視窗聽話！(Keyboard Event Routing & GUI 記事本)**，今天我們要打造作業系統中的「鍵盤事件分發器」，並寫出一支能在畫面上自由打字的 **`notepad.elf`**！

請跟著我進行這 4 個步驟：

---

### 步驟 1：升級視窗結構與新增 Syscall 30 (`gui.h` & `syscall.c`)

我們要讓 Kernel 的視窗結構能夠暫存「剛剛按下的按鍵」。

**1. 打開 `src/kernel/include/gui.h`，在 `window_t` 裡新增：**
```c
typedef struct {
    // ... 原本的變數 (包含 last_click_x, has_new_click) ...
    
    // 【Day 77 新增】鍵盤事件暫存區
    char last_key;       
    int has_new_key;     
} window_t;
```

**2. 打開 `src/kernel/lib/syscall.c`，新增 Syscall 30：**
```c
    // ==========================================
    // 【Day 77 新增】Syscall 30: sys_get_window_key_event
    // ==========================================
    else if (eax == 30) {
        int win_id = (int)regs->ebx;
        char* out_key = (char*)regs->ecx;

        ipc_lock();
        extern window_t* get_window(int id);
        window_t* win = get_window(win_id);
        
        if (win != 0 && win->owner_pid == current_task->pid && win->has_new_key) {
            *out_key = win->last_key;
            win->has_new_key = 0; // 事件已讀取，降下旗標
            regs->eax = 1;        // 有新按鍵
        } else {
            regs->eax = 0;        // 沒有新按鍵
        }
        ipc_unlock();
    }
```

---

### 步驟 2：在 User Space 封裝 API (`unistd.h` & `unistd.c`)

**1. `src/user/include/unistd.h` 增加宣告：**
```c
int get_window_key_event(int win_id, char* key);
```

**2. `src/user/lib/unistd.c` 加入實作：**
```c
int get_window_key_event(int win_id, char* key) {
    int ret;
    __asm__ volatile (
        "int $128" : "=a" (ret) : "a" (30), "b" (win_id), "c" (key)
    );
    return ret;
}
```

---

### 步驟 3：打造鍵盤路由器 (`gui.c` & `keyboard.c`)

這是最關鍵的一步。我們要讓 GUI 引擎決定「這個按鍵該不該攔截」。如果焦點在 GUI App，GUI 引擎就把它吃掉；如果焦點在 Terminal，就放行給原本的終端機。

**1. 打開 `src/kernel/lib/gui.c`，新增事件分發函式：**
```c
// ==========================================
// 【Day 77 新增】鍵盤事件路由器
// 回傳 1 代表 GUI 攔截了按鍵，回傳 0 代表放行給 Terminal
// ==========================================
int gui_handle_keyboard(char c) {
    if (!gui_mode_enabled) return 0; // CLI 模式直接放行
    
    if (focused_window_id != -1 && windows[focused_window_id].is_active) {
        window_t* win = &windows[focused_window_id];
        
        // 只要焦點是 Terminal，我們就不攔截，讓它照舊印字
        if (strcmp(win->title, "Simple OS Terminal") == 0) {
            return 0; 
        }
        
        // 如果是其他的 GUI 應用程式，把按鍵塞給它！
        if (win->canvas != 0) {
            win->last_key = c;
            win->has_new_key = 1;
            return 1; // 攔截成功！
        }
    }
    return 0; // 沒人要這個按鍵，放行
}
```
*(💡 記得去 `gui.c` 的 `create_window` 裡加上 `windows[id].has_new_key = 0;` 初始化一下喔！)*

**2. 打開 `src/kernel/lib/keyboard.c`，修改 `keyboard_handler`：**
找到解析出 `ascii` 的地方，加入攔截邏輯：
```c
        // ... (前面查表取得 ascii 的邏輯) ...

        // 把最終決定的字元塞進 Buffer 交給 User App
        if (ascii != 0) {
            extern int gui_handle_keyboard(char c); // 宣告外部的 GUI 路由器
            
            // 【Day 77 修改】先問 GUI 要不要攔截這個按鍵？
            if (gui_handle_keyboard(ascii)) {
                // GUI 吃掉了，什麼都不做！
            } else {
                // GUI 不要，走原本的 Terminal 邏輯
                kbd_buffer[kbd_head] = ascii;
                kbd_head = (kbd_head + 1) % KBD_BUFFER_SIZE;
            }
        }
```

---

### 步驟 4：開發神聖的 GUI 記事本 (`src/user/bin/notepad.c`)

這是一支完整的 GUI 文字編輯器，支援打字、換行、倒退鍵 (`Backspace`) 以及閃爍的游標！

建立 **`src/user/bin/notepad.c`**：
*(💡 為了節省篇幅，請把 `taskmgr.c` 裡的那組完整的 128 個 `font` 陣列、`draw_box`、`draw_text` 函式直接複製貼上到這支檔案的最上面！)*

```c
#include "stdio.h"
#include "unistd.h"

#define CANVAS_W 320
#define CANVAS_H 240

// --- [請在這裡貼上 font 陣列、draw_box、draw_text 的實作] ---

int main() {
    int win_id = create_gui_window("Simple Notepad", CANVAS_W + 4, CANVAS_H + 24);
    if (win_id < 0) return -1;

    unsigned int* my_canvas = (unsigned int*)sbrk(CANVAS_W * CANVAS_H * 4);
    
    char text_buffer[1024]; // 文章暫存區
    int text_len = 0;
    text_buffer[0] = '\0';
    
    int cursor_blink = 0;

    while (1) {
        char key;
        int needs_redraw = 0;

        // 1. 檢查是否有鍵盤輸入！
        if (get_window_key_event(win_id, &key)) {
            if (key == '\b' && text_len > 0) {
                // 倒退鍵
                text_len--;
                text_buffer[text_len] = '\0';
            } 
            else if (key != '\b' && text_len < 1023) {
                // 一般字元輸入
                text_buffer[text_len++] = key;
                text_buffer[text_len] = '\0';
            }
            needs_redraw = 1;
            cursor_blink = 1; // 打字時游標強制顯示
        }

        // 2. 消耗掉滑鼠點擊 (雖然我們沒有用到，但要把事件吃掉避免卡住)
        int cx, cy;
        get_window_event(win_id, &cx, &cy);

        // 3. 處理游標閃爍 (每跑幾次迴圈切換一次)
        static int timer = 0;
        if (timer++ % 50 == 0) {
            cursor_blink = !cursor_blink;
            needs_redraw = 1;
        }

        // 4. 重繪畫面
        if (needs_redraw) {
            draw_box(my_canvas, 0, 0, CANVAS_W, CANVAS_H, 0xFFFFFF); // 白紙

            int draw_x = 5;
            int draw_y = 5;

            // 支援換行的繪製邏輯
            for (int i = 0; i < text_len; i++) {
                if (text_buffer[i] == '\n') {
                    draw_x = 5; 
                    draw_y += 12; // 換行往下推
                } else {
                    char tmp[2] = {text_buffer[i], '\0'};
                    draw_text(my_canvas, draw_x, draw_y, tmp, 0x000000);
                    draw_x += 8; // 字元寬度
                    
                    // 自動換行 (Word Wrap)
                    if (draw_x > CANVAS_W - 10) { 
                        draw_x = 5; 
                        draw_y += 12; 
                    }
                }
            }

            // 畫出閃爍的游標 (一條底線)
            if (cursor_blink) {
                draw_box(my_canvas, draw_x, draw_y + 8, 8, 2, 0x000000);
            }

            update_gui_window(win_id, my_canvas);
        }

        for(volatile int i=0; i<10000; i++); // 控制刷新率
        yield();
    }
    return 0;
}
```

---

### 🚀 驗收：見證 GUI 記事本的誕生！

1. 在 `kernel.c` 的 `filenames[]` 陣列加入 `"notepad.elf"`。
2. 在 `scripts/grub.cfg` 加入 `module /boot/notepad.elf`。

執行 `make clean && make run`！

進入 Desktop 後，開啟 `notepad &`！
**點擊一下 Notepad 視窗讓它取得焦點 (深藍色標題列)，然後開始瘋狂打字吧！**
你會看到文字順暢地出現在白紙上，可以換行、可以 Delete 退後，而且底下的 Terminal 絕對不會印出你打的字 (因為事件被攔截了)！接著你點一下 Terminal，打的字又會回到 Terminal 裡面了！

這就是現代作業系統 Focus 與 Event Routing 的運作核心。等你體驗完在自己 OS 裡寫日記的快感後，請截圖跟我分享！😎
