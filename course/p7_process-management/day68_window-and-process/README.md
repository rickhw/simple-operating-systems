太酷了，Rick！`free` 指令的加入讓你的作業系統完整度又上了一個檔次！能即時監控 Paging 和 PMM，這絕對是架構師級別的品味。

現在，我們終於來到 **Phase 8** 的核心，也是所有圖形作業系統最迷人的轉捩點：
**Day 68：視窗伺服器啟動！(Window & Process 靈魂綁定)** 

目前的系統裡，視窗只是一個畫在螢幕上的「無生命方塊」，當你點擊右上角的 `[X]` 關閉它時，就只是把它的畫面藏起來而已。
真正的 Window Manager 必須具備**「生命共同體」**的機制：
1. **App 創建視窗**：User Space 的應用程式可以向 OS 申請視窗。
2. **視窗終結 App**：當使用者點擊 `[X]` 關閉視窗時，GUI 引擎會找出這個視窗的主人 (PID)，然後親手下達 `sys_kill` 把背後的 Process 殺掉！

跟著我實作這 4 個步驟，讓你的視窗正式獲得「靈魂」：

---

### 步驟 1：替視窗綁定 PID，實作致命的 `[X]` 按鈕 (`src/kernel/include/gui.h` & `lib/gui.c`)

我們要擴充 `window_t`，並把 `[X]` 按鈕的碰撞偵測拉出來，讓它具備殺人的能力！

**1. 打開 `src/kernel/include/gui.h`：**
在 `window_t` 結構中加入 `owner_pid`，並修改 `create_window` 的宣告：
```c
typedef struct {
    int id;
    int x;
    int y;
    int width;
    int height;
    char title[32];
    int is_active;
    int owner_pid; // 【Day 68 新增】視窗背後的靈魂 (PID)
} window_t;

// 修改宣告，加入 owner_pid
int create_window(int x, int y, int width, int height, const char* title, int owner_pid);
```

**2. 打開 `src/kernel/lib/gui.c`，修改 `create_window`：**
```c
int create_window(int x, int y, int width, int height, const char* title, int owner_pid) {
    if (window_count >= MAX_WINDOWS) return -1;
    int id = window_count++;
    // ... 原本的賦值 ...
    windows[id].owner_pid = owner_pid; // 【Day 68 新增】
    // ...
```
*(💡 注意：請全域搜尋 `create_window`，把你原本在 `kernel.c` 和 `gui.c` 裡創建 Terminal 與 Status 的地方，最後面都補上一個參數 `1`，代表它們預設是由 Kernel (PID 1) 創建的。例如：`create_window(..., "Simple OS Terminal", 1);`)*

**3. 在 `gui.c` 的 `gui_check_ui_click` 最前方，實作 `[X]` 殺手邏輯：**
```c
int gui_check_ui_click(int x, int y) {
    // ==========================================
    // 【Day 68 新增】0. 檢查是否點擊了焦點視窗的 [X] 關閉按鈕
    // ==========================================
    if (focused_window_id != -1 && windows[focused_window_id].is_active) {
        window_t* win = &windows[focused_window_id];
        int btn_x = win->x + win->width - 20;
        int btn_y = win->y + 4;
        
        // 如果滑鼠落在 [X] 的 14x14 方塊內
        if (x >= btn_x && x <= btn_x + 14 && y >= btn_y && y <= btn_y + 14) {
            int target_pid = win->owner_pid;
            close_window(win->id); // 關閉畫面
            
            // 呼叫死神！殺掉綁定這個視窗的 User Process！
            extern int sys_kill(int pid);
            if (target_pid > 1) { 
                sys_kill(target_pid); 
            }
            
            gui_dirty = 1;
            return 1; // 成功攔截點擊
        }
    }

    // ... 下面保留你原本的 (點擊桌面圖示、Start 選單等邏輯) ...
```

---

### 步驟 2：開通 `sys_create_window` 系統呼叫 (`src/kernel/lib/syscall.c`)

現在我們要允許 Ring 3 的平民百姓向 Kernel 申請視窗。

打開 **`src/kernel/lib/syscall.c`**，加入第 26 號系統呼叫：
```c
    // ==========================================
    // 【Day 68 新增】Syscall 26: sys_create_window
    // ==========================================
    else if (eax == 26) {
        char* title = (char*)regs->ebx;
        int width = (int)regs->ecx;
        int height = (int)regs->edx;
        
        // 簡單的視窗錯開邏輯，避免視窗全部疊在一起
        static int win_offset = 0;
        int x = 200 + win_offset;
        int y = 150 + win_offset;
        win_offset = (win_offset + 30) % 200;

        ipc_lock();
        // 綁定當前呼叫它的行程 PID！
        regs->eax = create_window(x, y, width, height, title, current_task->pid);
        extern void gui_redraw(void);
        gui_redraw(); // 立刻把視窗畫出來
        ipc_unlock();
    }
```

---

### 步驟 3：在 User Space 封裝 API (`src/user/include/unistd.h` & `lib/unistd.c`)

**1. `unistd.h` 增加宣告：**
```c
int create_gui_window(const char* title, int width, int height);
```

**2. `unistd.c` 加入實作：**
```c
int create_gui_window(const char* title, int width, int height) {
    int ret;
    __asm__ volatile (
        "int $128" 
        : "=a" (ret) 
        : "a" (26), "b" (title), "c" (width), "d" (height)
    );
    return ret;
}
```

---

### 步驟 4：開發第一支圖形化應用程式 (`src/user/bin/status.c`)

過去，我們的 `"System Status"` 是寫死在 GUI 裡面彈出來的。現在，我們要在 User Space 寫一支名為 `status.elf` 的程式，讓**它自己去申請這個視窗**！

建立 **`src/user/bin/status.c`**：

```c
#include "stdio.h"
#include "unistd.h"

int main() {
    // 1. 向 Window Manager 申請一個名為 "System Status" 的視窗
    int win_id = create_gui_window("System Status", 300, 200);
    
    if (win_id < 0) {
        printf("Failed to create GUI Window.\n");
        return -1;
    }

    // 2. 進入守護迴圈 (Daemon Loop)
    // 只要這個 while(1) 活著，視窗就會一直存在。
    // 當使用者點擊視窗的 [X]，Kernel 會直接從底層 sys_kill 這個程式，迴圈就會被強制終結！
    while (1) {
        yield(); // 讓出 CPU，避免佔用 100% 資源
    }

    return 0;
}
```

*(💡 記得在 `kernel.c` 的 setup_filesystem 陣列與 `grub.cfg` 加入 `"status.elf"`！)*

---

### 🚀 驗收：見證靈魂的綁定！

這是一個極具里程碑意義的測試，請依照以下步驟體驗：

1. 編譯執行 `make clean && make run`，進入桌面。
2. 在 Terminal 裡，先輸入 `ps`，看看目前的行程數量。
3. 在 Terminal 裡，輸入 **`status`** (執行我們剛寫好的 `status.elf`)。
   👉 你會看到畫面中央瞬間彈出一個 "System Status" 視窗！(因為我們 `gui.c` 裡剛好寫了只要標題是這個，就會畫出 CPU 狀態字樣)。
4. 再輸入一次 `ps`。
   👉 你會看到 `status.elf` 安靜地活在行程列表裡，狀態是 RUNNING/WAITING！
5. **最神奇的時刻來了**：用滑鼠點擊 System Status 視窗右上角的 **`[X]`**！
   👉 視窗瞬間消失！
6. 最後再敲一次 `ps` 或 `top`。
   👉 你會發現 `status.elf` **從行程列表中徹底消失了！**

Rick，你剛剛完成了現代作業系統中最精華的 **「視窗與行程的生命週期連動」**！這代表你的 GUI 不再只是一層皮，而是一個真正的 Window Manager 伺服器了！

體會一下這種從底層掌控一切的快感，準備好我們就邁向 **Day 69**，準備實作真・多工 GUI 所需的「Shared Memory (共享畫布)」吧！
