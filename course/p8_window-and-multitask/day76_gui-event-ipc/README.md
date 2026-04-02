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


---

太神啦，Rick！🎉 這張截圖真的是**史詩級的里程碑**！

畫面上同時跑著 Terminal、System Status、Dynamic Paint、Digital Clock，還有 File Explorer 與 Image Viewer！而且背景的 `ps` 清楚地展示了這些 Process 都在穩定地多工作業。這畫面完全就是一個現代化桌面作業系統的雛形，太感動了！

既然你看了大綱，想直接切入 **Task Manager (工作管理員)**，這個提議太完美了！
我們原本 Day 76 要做的「GUI 點擊事件 (IPC)」機制，剛好可以用來打造一個「**可以點擊按鈕來砍掉 (Kill) 視窗的圖形化工作管理員**」！

這將會是你 OS 裡最高階的系統管理工具。請跟著我進行這 3 個步驟：

---

### 步驟 1：開通「視窗點擊事件」系統呼叫

我們要讓 Kernel 把滑鼠點擊畫布的座標存起來，並提供 Syscall 讓 User App 讀取。
*(如果你還沒實作我們上個回合提到的 Syscall 29，請先補上這段)*

**1. 在 `src/kernel/lib/syscall.c` 新增 Syscall 29：**
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
        
        // 確保視窗存在，且確實是這個程式擁有的
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

**2. 在 `src/user/include/unistd.h` 新增宣告：**
```c
int get_window_event(int win_id, int* x, int* y);
```

**3. 在 `src/user/lib/unistd.c` 加入實作：**
```c
int get_window_event(int win_id, int* x, int* y) {
    int ret;
    __asm__ volatile (
        "int $128" : "=a" (ret) : "a" (29), "b" (win_id), "c" (x), "d" (y)
    );
    return ret;
}
```

---

### 步驟 2：打造圖形化工作管理員 (`src/user/bin/taskmgr.c`)

這支程式會呼叫 `get_process_list` 獲取系統所有行程，並在畫布上畫出列表。最酷的是，它會有一個事件迴圈 (Event Loop)，偵測你有沒有點擊紅色的 `[KILL]` 按鈕！

建立 **`src/user/bin/taskmgr.c`**：

```c
#include "stdio.h"
#include "unistd.h"

#define CANVAS_W 340
#define CANVAS_H 280

// 簡單的字串處理工具
int strlen(const char* s) { int i=0; while(s[i]) i++; return i; }

// 整數轉字串 (為了在畫布上印出 PID 和 Memory)
void itoa(int n, char* buf) {
    if (n == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    int i = 0;
    while (n > 0) { buf[i++] = (n % 10) + '0'; n /= 10; }
    buf[i] = '\0';
    for (int j = 0; j < i / 2; j++) {
        char t = buf[j]; buf[j] = buf[i - 1 - j]; buf[i - 1 - j] = t;
    }
}

// 借用 explorer 的微型 8x8 字型庫
static const unsigned char font[128][8] = {
    ['a']={0x00,0x3c,0x06,0x3e,0x66,0x3e,0x00}, ['b']={0x60,0x7c,0x66,0x66,0x66,0x7c,0x00},
    ['c']={0x00,0x3c,0x60,0x60,0x60,0x3c,0x00}, ['d']={0x06,0x3e,0x66,0x66,0x66,0x3e,0x00},
    ['e']={0x00,0x3c,0x7e,0x60,0x60,0x3c,0x00}, ['f']={0x1c,0x30,0x7c,0x30,0x30,0x30,0x00},
    ['g']={0x00,0x3e,0x66,0x66,0x3e,0x06,0x3c}, ['h']={0x60,0x7c,0x66,0x66,0x66,0x66,0x00},
    ['i']={0x18,0x00,0x38,0x18,0x18,0x3c,0x00}, ['j']={0x0c,0x00,0x1c,0x0c,0x0c,0x6c,0x38},
    ['k']={0x60,0x66,0x6c,0x78,0x6c,0x66,0x00}, ['l']={0x38,0x18,0x18,0x18,0x18,0x3c,0x00},
    ['m']={0x00,0xec,0xfe,0xd6,0xd6,0xd6,0x00}, ['n']={0x00,0x7c,0x66,0x66,0x66,0x66,0x00},
    ['o']={0x00,0x3c,0x66,0x66,0x66,0x3c,0x00}, ['p']={0x00,0x7c,0x66,0x66,0x7c,0x60,0x60},
    ['q']={0x00,0x3e,0x66,0x66,0x3e,0x06,0x06}, ['r']={0x00,0x5c,0x66,0x60,0x60,0x60,0x00},
    ['s']={0x00,0x3e,0x60,0x3c,0x06,0x7c,0x00}, ['t']={0x30,0x7c,0x30,0x30,0x30,0x1c,0x00},
    ['u']={0x00,0x66,0x66,0x66,0x66,0x3e,0x00}, ['v']={0x00,0x66,0x66,0x66,0x3c,0x18,0x00},
    ['w']={0x00,0xd6,0xd6,0xd6,0xfe,0x6c,0x00}, ['x']={0x00,0x66,0x3c,0x18,0x3c,0x66,0x00},
    ['y']={0x00,0x66,0x66,0x66,0x3e,0x06,0x3c}, ['z']={0x00,0x7e,0x0c,0x18,0x30,0x7e,0x00},
    ['0']={0x3c,0x66,0x6e,0x76,0x66,0x3c,0x00}, ['1']={0x18,0x38,0x18,0x18,0x18,0x3c,0x00},
    ['2']={0x3c,0x66,0x0c,0x18,0x30,0x7e,0x00}, ['3']={0x3c,0x66,0x1c,0x06,0x66,0x3c,0x00},
    ['4']={0x0c,0x1c,0x3c,0x6c,0x7e,0x0c,0x00}, ['5']={0x7e,0x60,0x7c,0x06,0x66,0x3c,0x00},
    ['6']={0x3c,0x60,0x7c,0x66,0x66,0x3c,0x00}, ['7']={0x7e,0x06,0x0c,0x18,0x30,0x30,0x00},
    ['8']={0x3c,0x66,0x3c,0x66,0x66,0x3c,0x00}, ['9']={0x3c,0x66,0x66,0x3e,0x06,0x3c,0x00},
    ['.']={0x00,0x00,0x00,0x00,0x18,0x18,0x00}, ['_']={0x00,0x00,0x00,0x00,0x00,0x7e,0x00},
    ['-']={0x00,0x00,0x00,0x3c,0x00,0x00,0x00}, [' ']={0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['A']={0x3c,0x66,0x66,0x7e,0x66,0x66,0x00}, ['C']={0x3c,0x66,0x60,0x60,0x66,0x3c,0x00},
    ['E']={0x7e,0x60,0x7c,0x60,0x60,0x7e,0x00}, ['I']={0x3c,0x18,0x18,0x18,0x18,0x3c,0x00},
    ['K']={0x66,0x6c,0x78,0x78,0x6c,0x66,0x00}, ['L']={0x60,0x60,0x60,0x60,0x60,0x7e,0x00},
    ['N']={0x66,0x76,0x7e,0x7e,0x6e,0x66,0x00}, ['P']={0x7c,0x66,0x66,0x7c,0x60,0x60,0x00},
    ['R']={0x7c,0x66,0x66,0x7c,0x6c,0x66,0x00}, ['S']={0x3c,0x60,0x3c,0x06,0x66,0x3c,0x00},
    ['T']={0x7e,0x18,0x18,0x18,0x18,0x18,0x00}, ['Z']={0x7e,0x0c,0x18,0x30,0x60,0x7e,0x00},
    ['[']={0x3c,0x30,0x30,0x30,0x30,0x3c,0x00}, [']']={0x3c,0x0c,0x0c,0x0c,0x0c,0x3c,0x00}
};

void draw_box(unsigned int* canvas, int x, int y, int w, int h, unsigned int color) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            if (x + j < CANVAS_W && y + i < CANVAS_H) {
                canvas[(y + i) * CANVAS_W + (x + j)] = color;
            }
        }
    }
}

void draw_text(unsigned int* canvas, int x, int y, const char* str, unsigned int color) {
    while (*str) {
        unsigned char c = *str++;
        if (c >= 'A' && c <= 'Z' && font[c][0] == 0 && font[c][1] == 0) c += 32;
        for (int r = 0; r < 8; r++) {
            for (int c_bit = 0; c_bit < 8; c_bit++) {
                if (font[c][r] & (1 << (7 - c_bit))) {
                    int px = x + c_bit, py = y + r;
                    if (px < CANVAS_W && py < CANVAS_H) canvas[py * CANVAS_W + px] = color;
                }
            }
        }
        x += 8; 
    }
}

int main() {
    int win_id = create_gui_window("Task Manager", CANVAS_W + 4, CANVAS_H + 24);
    if (win_id < 0) return -1;

    unsigned int* my_canvas = (unsigned int*)sbrk(CANVAS_W * CANVAS_H * 4);
    process_info_t p_list[32]; // 裝載行程資訊的陣列
    int refresh_timer = 0;

    while(1) {
        // 1. 每隔一段時間刷新一次名單
        if (refresh_timer % 100 == 0) {
            int p_count = get_process_list(p_list, 32);

            // 清空畫布
            draw_box(my_canvas, 0, 0, CANVAS_W, CANVAS_H, 0xFFFFFF);
            
            // 畫出標題列
            draw_box(my_canvas, 0, 0, CANVAS_W, 20, 0x4169E1); // 皇家藍
            draw_text(my_canvas, 10, 6, "PID   NAME       MEM (KB)   ACTION", 0xFFFFFF);

            // 畫出每一個 Process
            int y_offset = 25;
            for (int i = 0; i < p_count; i++) {
                char buf[16];
                
                // 畫 PID
                itoa(p_list[i].pid, buf);
                draw_text(my_canvas, 10, y_offset, buf, 0x000000);

                // 畫 Name
                draw_text(my_canvas, 50, y_offset, p_list[i].name, 0x000000);

                // 畫 Memory (KB)
                itoa(p_list[i].memory_used / 1024, buf);
                draw_text(my_canvas, 140, y_offset, buf, 0x000000);

                // 畫出 [KILL] 按鈕 (防呆：不准殺 PID 0, 1 和自己)
                if (p_list[i].pid > 1 && strcmp(p_list[i].name, "taskmgr.elf") != 0) {
                    draw_box(my_canvas, 250, y_offset - 2, 48, 12, 0xFF0000); // 紅色按鈕
                    draw_text(my_canvas, 254, y_offset, "KILL", 0xFFFFFF);
                }

                y_offset += 20;
                if (y_offset > CANVAS_H - 20) break;
            }
            update_gui_window(win_id, my_canvas);
        }
        refresh_timer++;

        // ==========================================
        // 2. 處理點擊事件 (IPC Event Loop)
        // ==========================================
        int cx, cy;
        if (get_window_event(win_id, &cx, &cy)) {
            // 檢查點擊 Y 座標是不是在列表區
            if (cy >= 25) {
                int clicked_row = (cy - 25) / 20;
                
                // 檢查點擊 X 座標是不是落在 [KILL] 按鈕的範圍 (250 ~ 298)
                if (cx >= 250 && cx <= 298) {
                    int p_count = get_process_list(p_list, 32);
                    if (clicked_row >= 0 && clicked_row < p_count) {
                        int target_pid = p_list[clicked_row].pid;
                        
                        // 防禦機制
                        if (target_pid > 1 && strcmp(p_list[clicked_row].name, "taskmgr.elf") != 0) {
                            printf("[TaskMgr] Killing PID %d (%s)...\n", target_pid, p_list[clicked_row].name);
                            kill(target_pid);
                            refresh_timer = 0; // 立刻強制刷新畫面！
                        }
                    }
                }
            }
        }

        yield(); // 讓出 CPU
    }
    return 0;
}
```

---

### 步驟 3：註冊並體驗死神的權力！

1. 打開 **`src/kernel/kernel.c`**，在 `filenames[]` 陣列加入 `"taskmgr.elf"`。
2. 打開 **`scripts/grub.cfg`**，在最下方加入 `module /boot/taskmgr.elf`。

執行 `make clean && make run`！

進入 Desktop 後，先開幾個 `paint &` 或是 `clock &` 讓畫面熱鬧一點。
接著輸入 **`taskmgr &`**。

👉 你會看到一個超專業的「工作管理員」視窗，清楚列出所有 PID、記憶體用量。
👉 **最精彩的來了：把滑鼠移到其中一個 `paint` 旁邊的紅色 `[KILL]` 按鈕，點下去！**
👉 那個 `paint` 視窗會瞬間被死神收割，並從列表中消失！

這就是整合了 Process Management 與 GUI IPC 的終極成果！去扮演系統的死神吧，截圖跟我分享你的戰果！😎
