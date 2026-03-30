看起來可以了，我想增加一個讓兩者 (terminal, desktop) 可以相互切換的功能，例如在 terminal 下輸入 desktop 可以切換去 window, 在 window 則在 開始上新增一個功能，可以切換到 terminal.

對了，我重構了目前程式碼的結構如下，之後就用這個結構討論吧：

```bash
├── Dockerfile      // 用來 Compile 的 GCC container
├── Makefile
├── README.md
├── scripts         
│   ├── grub.cfg    // GRUB 配置
│   └── linker.ld
└── src
    ├── kernel      // Kernel 部分
    │   ├── asm         // 組合語言，主要是用來執行 CPU 指令集, 可以參閱 (x86_Instruction.md)
    │   │   └── *.S
    │   ├── include
    │   │   └── *.h
    │   ├── kernel.c    // 主要的 Kernel 流程
    │   └── lib
    │       └── *.c     // Kernel 相關 Lib，包含 ata, gdt, idc, mbr, paging, pmm, syscall, timer, tty ... etc.
    └── user        // User Space 部分
        ├── asm
        │   └── crt0.S  // C Runtime Zero
        ├── bin         // User Space 的程式, 包含 cat, echo, ls, mkdir, rm, shell, touch
        │   └── *.c
        ├── include
        │   └── *.h
        └── lib         // syscall, stdlib, stdio, unistd
            └── *.c
```

---

這個重構後的目錄結構實在太漂亮了！這是一個非常標準且專業的 UNIX-like 作業系統原始碼樹狀結構。將 `kernel` 與 `user` space 徹底分離，這對於我們接下來要引入 System Call 與實作真正的多工作業，絕對是完美的基礎！

要達成「動態切換 CLI 與 GUI」這個功能，我們遇到了一個跨越特權級別的挑戰：
* **GUI -> CLI**：開始選單是 Kernel 畫的，所以它可以直接呼叫底層函數來切換。
* **CLI -> GUI**：`shell` 是一個運行在 Ring 3 (User Space) 的應用程式，它**沒有權限**直接呼叫 Kernel 的圖形渲染函數。因此，我們必須開通一個全新的 **System Call (系統呼叫)** 來完成這件事！

讓我們依照你超讚的新目錄結構，分三個步驟來實作這個功能：

---

### 步驟 1：實作核心切換邏輯與修改開始選單 (`src/kernel/lib/gui.c`)

首先，我們在 Kernel 裡準備好負責「變身」的統籌函式，並把開始選單拉長，新增第四個選項。

打開 **`src/kernel/lib/gui.c`**：

```c
#include "gui.h"
#include "tty.h"

// ... 你的其他程式碼 ...

// 【新增】統籌切換模式的核心 API
void switch_display_mode(int is_gui) {
    enable_gui_mode(is_gui);
    terminal_set_mode(is_gui);

    if (is_gui) {
        // 切換回 GUI 時，確保有一個 Terminal 視窗可以接管輸入
        int found = 0;
        window_t* wins = get_windows();
        for (int i = 0; i < 10; i++) { // 假設 MAX_WINDOWS = 10
            if (wins[i].is_active && strcmp(wins[i].title, "Simple OS Terminal") == 0) {
                terminal_bind_window(i);
                set_focused_window(i);
                found = 1;
                break;
            }
        }
        if (!found) {
            int term_win = create_window(50, 50, 368, 228, "Simple OS Terminal");
            terminal_bind_window(term_win);
        }
    } else {
        // 切換到 CLI 時，解除視窗綁定，讓 TTY 全螢幕輸出
        terminal_bind_window(-1);
    }
    
    // 強制畫面大更新
    extern void gui_redraw(void);
    gui_redraw(); 
}

// 【修改】畫出更高的開始選單
static void draw_start_menu(void) {
    if (!start_menu_open) return;

    int menu_w = 150;
    int menu_h = 130; // 從 100 加高到 130
    int menu_x = 4;
    int menu_y = 600 - 28 - menu_h; // 自動貼齊工作列

    // 畫底板與邊框... (略，保持不變)
    draw_rect(menu_x, menu_y, menu_w, menu_h, 0xC0C0C0);
    // ... 

    // 畫選單項目
    draw_string(menu_x + 10, menu_y + 12, "1. Terminal", 0x000000, 0xC0C0C0);
    draw_string(menu_x + 10, menu_y + 42, "2. Sys Status", 0x000000, 0xC0C0C0);
    draw_string(menu_x + 10, menu_y + 72, "3. Shutdown", 0x000000, 0xC0C0C0);
    // 【新增】第四個選項
    draw_string(menu_x + 10, menu_y + 102, "4. Exit to CLI", 0x000000, 0xC0C0C0);
}

// 【修改】點擊攔截器
int gui_check_ui_click(int x, int y) {
    // ... 前面檢查 Start 按鈕保持不變 ...

    if (start_menu_open) {
        int menu_y = 600 - 28 - 130; 
        
        if (x >= 4 && x <= 154 && y >= menu_y && y <= menu_y + 130) {
            // ... 處理 1, 2, 3 的邏輯保持不變 ...
            
            // 【新增】點擊 "4. Exit to CLI"
            if (y >= menu_y + 100 && y <= menu_y + 120) {
                switch_display_mode(0); // 切換為 CLI 模式 (0)
            }
            
            start_menu_open = 0; 
            extern void gui_redraw(void);
            gui_redraw();
            return 1;
        }
        // ...
    }
    return 0;
}
```

---

### 步驟 2：開通 System Call (`src/kernel/lib/syscall.c`)

為了讓 User Space 可以呼叫剛剛寫好的 `switch_display_mode`，我們必須註冊一個系統呼叫。假設你的 Syscall 是用中斷 128 (`int 0x80` 或 `int 0x82` 之類的) 實作的。

打開 **`src/kernel/lib/syscall.c`** (或對應處理 Syscall 的檔案)：

```c
#include "gui.h" // 為了呼叫 switch_display_mode

// 假設你定義了一個系統呼叫號碼，例如 10
#define SYS_SET_DISPLAY_MODE 10

// 你的 syscall handler
void syscall_handler(registers_t *regs) {
    // 假設 EAX 存放 syscall number
    switch (regs->eax) {
        // ... 其他系統呼叫如 SYS_WRITE, SYS_READ ...
        
        case SYS_SET_DISPLAY_MODE: {
            // 假設 EBX 存放第一個參數 (is_gui)
            int is_gui = (int) regs->ebx;
            switch_display_mode(is_gui);
            regs->eax = 0; // 回傳成功
            break;
        }
        default:
            kprintf("Unknown syscall: %d\n", regs->eax);
            break;
    }
}
```

---

### 步驟 3：在 User Space 實作並接入 Shell

最後，來到你的 User Space 目錄，我們要包裝這個系統呼叫，並把它變成 `shell` 的一個內建指令。

1. 打開 **`src/user/lib/unistd.c`** (實作 User API)：
```c
// 透過組合語言觸發軟體中斷
int set_display_mode(int is_gui) {
    int ret;
    // 假設 128 是你的 syscall 中斷號碼，EAX=10 代表切換模式，EBX=參數
    __asm__ volatile (
        "int $128" 
        : "=a" (ret) 
        : "a" (10), "b" (is_gui)
    );
    return ret;
}
```

2. 並且在 **`src/user/include/unistd.h`** 中宣告它：
```c
int set_display_mode(int is_gui);
```

3. 打開 **`src/user/bin/shell.c`**，把指令接上去：
```c
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main() {
    char input[256];
    
    while (1) {
        printf("SimpleOS> ");
        // 假設你有 gets 或自己寫的 readline
        gets(input); 

        // 【新增】檢查是不是 desktop 指令
        if (strcmp(input, "desktop") == 0) {
            set_display_mode(1); // 呼叫 syscall，要求 Kernel 切換到 GUI
            continue; // 直接迴圈，不用執行其他程式
        }
        
        // ... 原本的 ls, cat 等外部指令執行邏輯 ...
    }
    return 0;
}
```

編譯並執行 (`make clean && make run`) 後。
如果你在 GRUB 選擇 CLI 模式開機，當你在全黑的終端機打下 `desktop` 時，User Space 的 Shell 會觸發中斷，一瞬間，華麗的漸層視窗桌面就會將你包圍；反之，在 GUI 點擊 Start Menu 的 `Exit to CLI`，視窗會瞬間瓦解，回歸最純粹的 Hacker 黑屏。
