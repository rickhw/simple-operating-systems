太美了，Rick！這張截圖真的有一種 1995 年微軟總部剛跑出第一版 Windows 95 雛形的感動！🎉

視窗的 Z-Order 正確、文字完美地被限制在 Terminal 的深色區域內，而且背景完全沒有被破壞。這代表你的 GUI 引擎（Compositor）已經非常成熟穩固了。

既然我們已經有了視窗和滑鼠，一個真正的桌面環境怎麼能少得了最經典的靈魂元素？
**Day 58** 的任務，我們要來打造**「工作列 (Taskbar)」**與能讀取硬體時間的**「即時時鐘 (RTC, Real-Time Clock)」**！

這會讓你的作業系統瞬間充滿「活著」的感覺。請跟著我進行這 3 個步驟：

---

### 步驟 1：實作 RTC 硬體時鐘驅動 (`lib/rtc.c`)

電腦主機板上有一顆獨立的電池在維持時間，這顆晶片叫做 CMOS。我們可以透過 I/O Port `0x70` 和 `0x71` 跟它要目前的時間。

請建立兩個新檔案：**`lib/include/rtc.h`** 與 **`lib/rtc.c`**。

**`lib/include/rtc.h`**
```c
#ifndef RTC_H
#define RTC_H

#include <stdint.h>

// 取得目前的小時與分鐘
void read_time(int* hour, int* minute);

#endif
```

**`lib/rtc.c`**
```c
#include "rtc.h"

// 宣告 I/O 函式
extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t data);

#define CMOS_ADDRESS 0x70
#define CMOS_DATA    0x71

static int get_update_in_progress_flag() {
    outb(CMOS_ADDRESS, 0x0A);
    return (inb(CMOS_DATA) & 0x80);
}

static uint8_t get_RTC_register(int reg) {
    outb(CMOS_ADDRESS, reg);
    return inb(CMOS_DATA);
}

// CMOS 預設使用 BCD (Binary-Coded Decimal) 格式儲存數字，必須轉回一般二進位
static uint8_t bcd_to_binary(uint8_t bcd) {
    return ((bcd & 0xF0) >> 1) + ((bcd & 0xF0) >> 3) + (bcd & 0x0F);
}

void read_time(int* hour, int* minute) {
    // 等待時鐘更新完畢
    while (get_update_in_progress_flag());
    
    uint8_t h = get_RTC_register(0x04);
    uint8_t m = get_RTC_register(0x02);
    uint8_t regB = get_RTC_register(0x0B);

    // 如果不是二進位模式，就從 BCD 轉換
    if (!(regB & 0x04)) {
        h = bcd_to_binary(h);
        m = bcd_to_binary(m);
    }
    
    // 處理 12 小時制
    if (!(regB & 0x02) && (h & 0x80)) {
        h = ((h & 0x7F) + 12) % 24;
    }

    // 將底層的 UTC 時間加上 8 小時 (轉換為本地時間)
    *hour = (h + 8) % 24;
    *minute = m;
}
```

---

### 步驟 2：繪製復古工作列 (`lib/gui.c`)

有了時間後，我們要用 3D 邊框的畫法，在螢幕最下方畫出一條包含 `[Start]` 按鈕與右下角時鐘的工作列。

請打開 **`lib/gui.c`**，在上方引入標頭檔，並新增 `draw_taskbar` 函數：

```c
// ... 最上方引入
#include "rtc.h"

// ... 在 gui_render 之上新增這個函數：
static void draw_taskbar(void) {
    int screen_w = 800;
    int screen_h = 600;
    int taskbar_h = 28;
    int taskbar_y = screen_h - taskbar_h;

    // 1. 主底板 (經典亮灰色)
    draw_rect(0, taskbar_y, screen_w, taskbar_h, 0xC0C0C0);
    // 上方的高光白線，營造凸起感
    draw_rect(0, taskbar_y, screen_w, 2, 0xFFFFFF);

    // 2. Start 按鈕 (凸起的 3D 按鈕)
    draw_rect(4, taskbar_y + 4, 60, 20, 0xC0C0C0);
    draw_rect(4, taskbar_y + 4, 60, 2, 0xFFFFFF); // 上亮邊
    draw_rect(4, taskbar_y + 4, 2, 20, 0xFFFFFF); // 左亮邊
    draw_rect(4, taskbar_y + 22, 60, 2, 0x808080); // 下陰影
    draw_rect(62, taskbar_y + 4, 2, 20, 0x808080); // 右陰影
    draw_string(14, taskbar_y + 10, "Start", 0x000000, 0xC0C0C0);

    // 3. 右下角時鐘系統匣 (凹陷的 3D 區域)
    int tray_w = 66;
    int tray_x = screen_w - tray_w - 4;
    draw_rect(tray_x, taskbar_y + 4, tray_w, 20, 0xC0C0C0);
    draw_rect(tray_x, taskbar_y + 4, tray_w, 2, 0x808080);  // 上陰影 (凹陷感)
    draw_rect(tray_x, taskbar_y + 4, 2, 20, 0x808080);  // 左陰影
    draw_rect(tray_x, taskbar_y + 22, tray_w, 2, 0xFFFFFF); // 下亮邊
    draw_rect(tray_x + tray_w - 2, taskbar_y + 4, 2, 20, 0xFFFFFF); // 右亮邊

    // 4. 取得硬體時間並轉成字串
    int h, m;
    read_time(&h, &m);
    
    char time_str[6] = "00:00";
    time_str[0] = (h / 10) + '0';
    time_str[1] = (h % 10) + '0';
    time_str[3] = (m / 10) + '0';
    time_str[4] = (m % 10) + '0';
    
    // 畫出時間
    draw_string(tray_x + 10, taskbar_y + 10, time_str, 0x000000, 0xC0C0C0);
}
```

---

### 步驟 3：將工作列加入渲染迴圈 (`lib/gui.c`)

最後，我們只要讓畫面合成器在畫完所有視窗後，畫上工作列即可（讓工作列永遠疊在所有視窗之上）。

請在 **`lib/gui.c`** 找到你的 `gui_render`，並加入一行：

```c
void gui_render(int mouse_x, int mouse_y) {
    draw_rect(0, 0, 800, 600, TERM_BG);

    // 1. 畫底層視窗
    // ...
    // 2. 畫焦點視窗
    // ...
    // 3. 畫終端機文字
    tty_render();

    // ==========================================
    // 4. 畫工作列 (Taskbar)
    // ==========================================
    draw_taskbar();

    // 5. 畫滑鼠游標 (永遠在最最上層)
    draw_cursor(mouse_x, mouse_y);
    
    // 交換畫布
    gfx_swap_buffers();
}
```

*(💡 備註：記得修改 `Makefile`，確認 `lib/rtc.c` 會被編譯進去)*

---

一切就緒！存檔，敲下：
```bash
make clean && make run
```

現在你的螢幕下方會出現一條極具質感的立體工作列，而且右下角會準確顯示出你當前的實體時間！
因為我們的渲染機制綁定在滑鼠上，所以只要你**稍微晃動一下滑鼠**，時間如果跨越了一分鐘，數字就會立刻跟著跳動更新！

這是不是讓你的 Simple OS 瞬間有模有樣了？接下來，你想挑戰讓那個 `Start` 按鈕點下去能彈出選單，還是試著把鍵盤打字的輸入也接進這顆 GUI Terminal 裡呢？


---

哈哈，Rick！你這個想法太棒了，這正是我們打造 GUI 作業系統的終極目標：**讓黑屏時代的應用程式 (Ring 3 Apps)，完美地在圖形視窗裡重生！**

其實你的系統底層「完全沒有壞掉」，鍵盤中斷 (`IRQ 1`) 還是會收到訊號，只是目前有**兩個關鍵的開關**還沒打開：

1. **Kernel 還在沉睡**：在 Day 51 切換圖形模式時，我們把載入 `shell.elf` 並啟動多工作業 (`init_multitasking`) 的程式碼給註解掉了，所以系統現在只是一張「靜態畫布」，根本沒有在跑應用程式！
2. **畫面缺乏「自動刷新」**：在以前的 VGA 文字模式，寫入 `0xB8000` 螢幕就會瞬間改變。但在我們現在的「雙重緩衝 (Double Buffering)」架構下，如果我們打字，字元只會被默默存進 `text_buffer` 裡，必須呼叫 `gui_render()` 交換畫布，螢幕才會更新！（這就是為什麼你現在如果打字，必須「晃一下滑鼠」字才會跑出來XD）

我們只要進行以下 3 個步驟，就能讓你的 GUI Terminal 完全活過來！🚀

---

### 步驟 1：賦予 GUI「原地重繪」的能力 (`lib/include/gui.h` & `lib/gui.c`)

為了讓終端機在印出文字時可以通知螢幕更新，我們要新增一個 `gui_redraw()` 函式，它會記住最後一次的滑鼠座標來重繪畫面。

1. 打開 **`lib/include/gui.h`**，新增宣告：
```c
// ...
void gui_render(int mouse_x, int mouse_y);
void gui_redraw(void); // 【新增】不改變滑鼠座標的原地重繪
// ...
```

2. 打開 **`lib/gui.c`**，新增全域變數來記錄滑鼠，並實作 `gui_redraw`：
```c
// ... 在最上方的 static 變數區新增：
static int last_mouse_x = 400;
static int last_mouse_y = 300;

// 【修改】gui_render 每次被呼叫時，都記住最後的座標
void gui_render(int mouse_x, int mouse_y) {
    last_mouse_x = mouse_x;
    last_mouse_y = mouse_y;
    
    // ... 下面畫畫的邏輯完全保持不變 ...
    draw_rect(0, 0, 800, 600, TERM_BG);
    // ...
}

// 【新增】提供給打字機呼叫的重繪函式
void gui_redraw(void) {
    gui_render(last_mouse_x, last_mouse_y);
}
```

---

### 步驟 2：讓鍵盤打字能「瞬間觸發」螢幕更新 (`lib/tty.c`)

打開 **`lib/tty.c`**，在我們印出字元的最後一步，強制觸發 GUI 重繪。這樣不管你是按鍵盤，還是 `ls` 列出檔案，畫面都會立刻更新！

```c
void terminal_putchar(char c) {
    if (c == '\n') {
        term_col = 0;
        term_row++; 
        if (term_row >= MAX_ROWS) terminal_initialize(); 
        gui_redraw(); // 【新增】換行也要重繪
        return;
    }

    if (c == '\b') {
        if (term_col > 0) {
            term_col--;
            text_buffer[term_row][term_col] = '\0'; 
        }
        gui_redraw(); // 【新增】退格也要重繪
        return;
    }

    // 存入記憶矩陣
    text_buffer[term_row][term_col] = c;
    term_col++;

    // 自動換行
    if (term_col >= MAX_COLS) {
        term_col = 0;
        term_row++;
        if (term_row >= MAX_ROWS) terminal_initialize();
    }
    
    gui_redraw(); // 【新增】一般字元印出後重繪！
}
```

---

### 步驟 3：喚醒沉睡的 Shell (`lib/kernel.c`)

最後，我們要把註解掉的應用程式載入邏輯加回來，讓 Kernel 功成身退，把舞台交給 Ring 3 的 Shell！

打開 **`lib/kernel.c`**，滾到 `kernel_main` 的最底下，**刪除原本的 `while (1) { __asm__ volatile ("hlt"); }`**，並替換成以下程式碼：

```c
    // ... 前面建立視窗、綁定 TTY、啟動滑鼠的邏輯保持不變 ...
    
    kprintf("=== OS Subsystems Ready ===\n\n");
    kprintf("Welcome to Simple OS Desktop Environment!\n");

    // ==========================================
    // 【復活】應用程式載入與排程 (Ring 0 -> Ring 3)
    // ==========================================
    kprintf("[Kernel] Fetching 'shell.elf' from Virtual File System...\n");
    fs_node_t* app_node = simplefs_find(1, "shell.elf");

    if (app_node != 0) {
        uint8_t* app_buffer = (uint8_t*) kmalloc(app_node->length);
        vfs_read(app_node, 0, app_node->length, app_buffer);
        uint32_t entry_point = elf_load((elf32_ehdr_t*)app_buffer);

        if (entry_point != 0) {
            kprintf("Creating Initial Process (Shell)...\n\n");

            // 啟動排程器 (Timer IRQ0 開始跳動)
            init_multitasking();

            // 為 Shell 分配專屬的 User Stack
            uint32_t ustack1_phys = (uint32_t) pmm_alloc_page();
            map_page(0x083FF000, ustack1_phys, 7);

            // 建立 Ring 3 主任務
            create_user_task(entry_point, 0x083FF000 + 4096);

            kprintf("[Kernel] Dropping to idle state. Enjoy your GUI!\n");

            // Kernel 功成身退，把自己宣告死亡！
            exit_task();
        }
    } else {
        kprintf("[Kernel] Error: Shell (shell.elf) not found on disk.\n");
    }

    // 備用安全降落區
    while (1) { __asm__ volatile ("hlt"); }
}
```

---

存檔，執行：
```bash
make clean && make run
```

這一次，當畫面亮起後，你會看到終端機視窗裡面印出了熟悉的 `Simple OS Shell >` 提示字元！
你可以直接敲擊你的鍵盤，輸入 `ls`、`cat hello.txt`，甚至執行 `pong` 來玩你之前寫的 IPC 測試程式。你會發現所有的 Ring 3 應用程式，都在這個你親手刻出來的圖形化視窗裡面完美運作！

這絕對是一個里程碑，你的作業系統現在是個名副其實的 **GUI 作業系統**了！趕快下個 `ls` 指令，截張圖來感受一下這份成就感吧！😎
