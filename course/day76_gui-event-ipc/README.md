太神啦，Rick！🎉 看到這張截圖我真的是驚呼出聲！

你的畫面上同時跑著 **Terminal (Shell)、System Status、Dynamic Paint、Digital Clock，還有 File Explorer 與 Image Viewer**！而且背景的 `ps` 清楚地展示了這些 Process 都在穩定地多工作業。這絕對是一個作業系統從「玩具」晉升為「現代化桌面 OS」最具代表性的里程碑，完全可以拿去當作開源專案的宣傳圖了！

既然我們的檔案總管已經能完美列出硬碟裡的檔案，現在就只差那「畫龍點睛」的一步了。
**Day 76：點擊開啟！(GUI Event IPC 機制)** 在現代的 UI 框架中，視窗不只是被動地被畫出來，它還要能接收使用者的操作。今天，我們要讓 Kernel 成為一個「事件分發器」，當你用滑鼠點擊視窗的畫布 (Canvas) 時，Kernel 會把座標轉換成相對位址，並推播給擁有該視窗的應用程式。

請跟著我進行這 4 個步驟，讓你的檔案總管真正「活」起來！

---

### 步驟 1：讓視窗記住滑鼠點擊 (`src/kernel/include/gui.h` & `lib/gui.c`)

我們要讓 `window_t` 結構多出幾個欄位，用來儲存最新一次的點擊事件。

**1. 打開 `src/kernel/include/gui.h`，擴充 `window_t`：**
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
    uint32_t* canvas; 
    
    // ==========================================
    // 【Day 76 新增】視窗內部事件暫存區
    // ==========================================
    int last_click_x;    // 點擊在畫布上的相對 X 座標
    int last_click_y;    // 點擊在畫布上的相對 Y 座標
    int has_new_click;   // 1 代表有新事件尚未被 App 讀取
} window_t;
```

**2. 打開 `src/kernel/lib/gui.c`：**
首先，在 `create_window` 的結尾處，加上初始化：
```c
    windows[id].has_new_click = 0; // 【Day 76 新增】初始化事件狀態
    focused_window_id = id; 
    return id;
}
```

接著，修改 `gui_check_ui_click` 裡面的「第 2 層：目前擁有焦點的視窗」邏輯。我們要攔截點擊「畫布本體」的事件：
```c
    // ------------------------------------------
    // 第 2 層：目前擁有焦點的視窗 (Top Window)
    // ------------------------------------------
    if (focused_window_id != -1 && windows[focused_window_id].is_active) {
        window_t* win = &windows[focused_window_id];
        
        if (x >= win->x && x <= win->x + win->width && y >= win->y && y <= win->y + win->height) {
            
            int btn_x = win->x + win->width - 20;
            int btn_y = win->y + 4;
            
            if (x >= btn_x && x <= btn_x + 14 && y >= btn_y && y <= btn_y + 14) {
                // ... (關閉視窗邏輯保持不變) ...
            } 
            else if (y >= win->y && y <= win->y + 20) {
                // ... (拖曳視窗邏輯保持不變) ...
            }
            // ==========================================
            // 【Day 76 新增】點擊了畫布內部 (Client Area)！
            // ==========================================
            else if (win->canvas != 0 && 
                     x >= win->x + 2 && x <= win->x + win->width - 2 &&
                     y >= win->y + 22 && y <= win->y + win->height - 2) {
                
                // 計算相對於畫布左上角的相對座標
                win->last_click_x = x - (win->x + 2);
                win->last_click_y = y - (win->y + 22);
                win->has_new_click = 1; // 舉起旗標，等待 App 來領取！
            }
            
            gui_dirty = 1;
            return 1; // 吞掉點擊！
        }
    }
```

---

### 步驟 2：開通 `sys_get_window_event` 系統呼叫 (`src/kernel/lib/syscall.c`)

User Space 的程式會不斷輪詢 (Poll) 這個系統呼叫，看 Kernel 有沒有發送新點擊事件給自己。

打開 **`src/kernel/lib/syscall.c`**，加入 Syscall 29：
```c
    // ==========================================
    // 【Day 76 新增】Syscall 29: sys_get_window_event
    // ==========================================
    else if (eax == 29) {
        int win_id = (int)regs->ebx;
        int* out_x = (int*)regs->ecx;
        int* out_y = (int*)regs->edx;

        ipc_lock();
        extern window_t* get_window(int id);
        window_t* win = get_window(win_id);
        
        // 安全性檢查：確認視窗存在，且確實是這個程式擁有的
        if (win != 0 && win->owner_pid == current_task->pid && win->has_new_click) {
            *out_x = win->last_click_x;
            *out_y = win->last_click_y;
            win->has_new_click = 0; // 事件已讀取，降下旗標
            regs->eax = 1; // 回傳 1 代表有新事件
        } else {
            regs->eax = 0; // 回傳 0 代表沒有事件
        }
        ipc_unlock();
    }
```

---

### 步驟 3：在 User Space 封裝 API (`src/user/include/unistd.h` & `lib/unistd.c`)

**1. `unistd.h` 增加宣告：**
```c
int get_window_event(int win_id, int* x, int* y);
```

**2. `unistd.c` 加入實作：**
```c
int get_window_event(int win_id, int* x, int* y) {
    int ret;
    __asm__ volatile (
        "int $128" 
        : "=a" (ret) 
        : "a" (29), "b" (win_id), "c" (x), "d" (y)
    );
    return ret;
}
```

---

### 步驟 4：升級檔案總管，賦予靈魂！ (`src/user/bin/explorer.c`)

這是最精彩的部分！我們要讓 `explorer` 記住剛剛掃描到的檔案名稱，並且在事件迴圈中接收滑鼠點擊。一旦發現點擊座標落在某個檔案上，就呼叫 `fork()` + `exec()` 開啟它！

修改 **`src/user/bin/explorer.c`**，補上字串函式與事件迴圈：

```c
#include "stdio.h"
#include "unistd.h"

#define CANVAS_W 280
#define CANVAS_H 360

// 簡單的字串處理工具
int strlen(const char* s) { int i=0; while(s[i]) i++; return i; }
void strcpy(char* dest, const char* src) { while(*src) *dest++ = *src++; *dest = '\0'; }

// ... (中間的 font 陣列, draw_box, draw_text 保持不變) ...

int main() {
    int win_id = create_gui_window("File Explorer", CANVAS_W + 4, CANVAS_H + 24);
    if (win_id < 0) return -1;

    unsigned int* my_canvas = (unsigned int*)sbrk(CANVAS_W * CANVAS_H * 4);

    draw_box(my_canvas, 0, 0, CANVAS_W, CANVAS_H, 0xFFFFFF); 
    draw_box(my_canvas, 0, 0, CANVAS_W, 24, 0xE0E0E0);       
    draw_box(my_canvas, 0, 24, CANVAS_W, 2, 0x808080);       
    draw_text(my_canvas, 10, 8, "C:\\> SimpleFS", 0x000000);  

    // ==========================================
    // 【Day 76 新增】建立陣列，記住所有掃描到的檔案
    // ==========================================
    char file_names[64][32];
    int total_files = 0;

    int y_offset = 35;
    char name[32];
    int size, type;

    for (int index = 0; index < 64; index++) {
        name[0] = '\0'; 
        if (readdir(index, name, &size, &type) == -1) break; 
        if (name[0] == '\0') continue; 

        // 把檔名存進陣列，方便等一下點擊時查表！
        strcpy(file_names[total_files], name);
        total_files++;

        if (type == 2) {
            draw_box(my_canvas, 10, y_offset, 16, 12, 0xFFD700);
            draw_box(my_canvas, 10, y_offset - 2, 6, 2, 0xFFD700);
        } else {
            draw_box(my_canvas, 10, y_offset, 12, 16, 0x4169E1);
            draw_box(my_canvas, 12, y_offset + 2, 8, 12, 0xFFFFFF);
        }

        draw_text(my_canvas, 32, y_offset + 4, name, 0x000000);

        y_offset += 24;
        if (y_offset > CANVAS_H - 24) break; 
    }

    update_gui_window(win_id, my_canvas);

    // ==========================================
    // 【Day 76 新增】動態事件迴圈 (Event Loop)
    // ==========================================
    while(1) { 
        int click_x, click_y;
        
        // 檢查 Kernel 有沒有傳來滑鼠點擊事件
        if (get_window_event(win_id, &click_x, &click_y)) {
            
            // 檢查點擊的 Y 座標是否落在檔案列表區
            if (click_y >= 35 && click_y < y_offset) {
                // 反推是點到了陣列中的第幾個檔案 (每個檔案佔 24px)
                int clicked_idx = (click_y - 35) / 24;
                
                if (clicked_idx >= 0 && clicked_idx < total_files) {
                    char* target_file = file_names[clicked_idx];
                    
                    // 為了視覺回饋，在 Terminal 印出點擊了什麼
                    printf("[Explorer] Opening: %s\n", target_file);

                    // 判斷是不是執行檔 (.elf)
                    int len = strlen(target_file);
                    if (len > 4 && target_file[len-4]=='.' && target_file[len-3]=='e' && 
                        target_file[len-2]=='l' && target_file[len-1]=='f') {
                        
                        // 呼叫偉大的 fork + exec！在背景啟動它！
                        int pid = fork();
                        if (pid == 0) {
                            char* args[] = {target_file, 0};
                            exec(target_file, args);
                            exit();
                        }
                    }
                }
            }
        }

        yield(); // 避免占用 CPU
    }
    
    return 0;
}
```

---

### 🚀 驗收：見證點擊開啟的瞬間！

存檔、編譯執行 `make clean && make run`！

進入 Desktop 後：
1. 打開 `explorer &`。
2. 讓滑鼠在列表中尋找 **`clock.elf`** 或是 **`paint.elf`**。
3. 把滑鼠移到它上面，輕輕點擊一下左鍵！

👉 **啪！你的 `clock` 或是 `paint` 視窗將會直接彈出來！** 你再也不用回 Terminal 去打字了。你現在擁有了一個具備完整 Event Loop，且能夠透過 IPC 呼叫 OS 系統核心啟動新 Process 的真實圖形介面檔案總管！

Rick，這就是作業系統中最經典的「雙擊圖示開啟程式」底層的運作原理！享受一下這份成就感，把視窗點得滿畫面都是吧！😎
