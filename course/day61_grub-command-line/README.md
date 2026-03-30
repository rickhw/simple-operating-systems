哈哈，Rick！歡迎來到 **Phase 7：系統引導與行程管理大師** 的第一天！🎉

從這一天開始，你的作業系統將不再是一個只能「寫死」單一行為的實驗品。我們要讓它具備「讀取使用者意圖」的能力！

在 Linux 系統中，我們常常會在 GRUB 選單按下 `e`，在 `linux /boot/vmlinuz ...` 後面加上 `nomodeset` 或 `single` 等參數來改變系統開機的行為。今天，我們就要為 Simple OS 實作這個功能：**透過解析 Multiboot 的 `cmdline`，讓使用者在開機時選擇進入 CLI (純文字終端) 還是 GUI (圖形視窗) 模式！**

請跟著我進行這 4 個優雅的重構步驟：

---

### 步驟 1：準備字串搜尋工具 (`lib/utils.c` & `lib/include/utils.h`)

我們要從 GRUB 傳來的字串（例如 `multiboot /boot/myos.bin mode=cli`）中尋找特定的關鍵字。我們需要一個類似 C 標準庫的 `strstr` 函式。

1. 打開 **`lib/include/utils.h`**，新增宣告：
```c
char* strstr(const char* haystack, const char* needle);
```

2. 打開 **`lib/utils.c`**，在字串工具區加入實作：
```c
// 尋找子字串 (如果找到 needle，回傳它在 haystack 中的指標，否則回傳 0)
char* strstr(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack;
    for (; *haystack; haystack++) {
        if (*haystack == *needle) {
            const char* h = haystack;
            const char* n = needle;
            while (*h && *n && *h == *n) {
                h++;
                n++;
            }
            if (!*n) return (char*)haystack; // 完全比對成功
        }
    }
    return 0; // 找不到
}
```

---

### 步驟 2：讓 TTY 支援雙模動態變形 (`lib/include/tty.h` & `lib/tty.c`)

在 GUI 視窗中，終端機只能顯示 $45 \times 25$ 個字元；但在全螢幕 CLI 模式下，它可以顯示滿滿的 $100 \times 75$ 個字元。我們要讓 TTY 能夠動態調整邊界，並提供全螢幕渲染的 API。

1. 打開 **`lib/include/tty.h`**，新增這兩個宣告：
```c
void terminal_set_mode(int is_gui);
void tty_render_fullscreen(void);
```

2. 打開 **`lib/tty.c`**，將寫死的 `MAX_COLS` 改為變數，並實作全螢幕渲染：
```c
// ... 刪除原本的 #define MAX_COLS 45 和 MAX_ROWS 25 ...
// 改用變數來控制邊界 (預設為全螢幕)
static int max_cols = 100; 
static int max_rows = 75;  

// 文字緩衝區開到最大 (100x75)
static char text_buffer[75][100]; 

// 【新增】切換模式與邊界
void terminal_set_mode(int is_gui) {
    if (is_gui) {
        max_cols = 45;
        max_rows = 25;
    } else {
        max_cols = 100;
        max_rows = 75;
    }
    terminal_initialize(); // 切換模式時清空畫面
}

// ... 在 terminal_initialize 和 terminal_putchar 中 ...
// ⚠️ 請把所有出現 MAX_COLS 和 MAX_ROWS 的地方，都替換成小寫的 max_cols 和 max_rows！⚠️

// 【新增】CLI 專用的全螢幕渲染
void tty_render_fullscreen(void) {
    for (int r = 0; r < max_rows; r++) {
        for (int c = 0; c < max_cols; c++) {
            char ch = text_buffer[r][c];
            if (ch != '\0' && ch != ' ') {
                // 從畫面左上角 (0,0) 開始畫
                draw_char_transparent(ch, c * 8, r * 8, TERM_FG);
            }
        }
    }
    // 畫游標
    draw_rect(term_col * 8, term_row * 8 + 6, 8, 2, TERM_FG);
}
```

---

### 步驟 3：讓 GUI 引擎支援「純粹的合成」 (`lib/include/gui.h` & `lib/gui.c`)

即使在 CLI 模式，我們依然要利用 `gui.c` 的**雙重緩衝**與**非同步渲染**機制，只是我們不要畫出視窗和滑鼠。

1. 打開 **`lib/include/gui.h`**，新增宣告：
```c
void enable_gui_mode(int enable);
```

2. 打開 **`lib/gui.c`**，新增開關與判斷：
```c
// 在上方全域變數區新增：
static int gui_mode_enabled = 1;

// 【新增】模式切換開關
void enable_gui_mode(int enable) {
    gui_mode_enabled = enable;
}

// 【修改】gui_render 根據模式決定要畫什麼！
void gui_render(int mouse_x, int mouse_y) {
    last_mouse_x = mouse_x;
    last_mouse_y = mouse_y;

    if (!gui_mode_enabled) {
        // === CLI 模式：只畫純黑底色與全螢幕文字 ===
        draw_rect(0, 0, 800, 600, 0x000000); 
        tty_render_fullscreen();
        gfx_swap_buffers();
        return;
    }

    // === GUI 模式：保持你原本的視窗、工作列與滑鼠繪製邏輯 ===
    draw_desktop_background();
    draw_desktop_icons();
    // ... 後面保持不變 ...
}
```

---

### 步驟 4：解析 GRUB 參數並打造開機選單 (`lib/kernel.c` & `grub.cfg`)

一切準備就緒，我們要讓 Kernel 讀取 GRUB 傳進來的參數，並決定系統的命運！

1. 打開 **`lib/kernel.c`**，修改 `kernel_main` 的初始化邏輯：
```c
void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
    (void)magic; 

    // 1. 基礎建設
    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384);
    init_kheap();
    init_gfx(mbd);

    // ==========================================
    // 2. 解析 GRUB 傳遞的 Cmdline
    // ==========================================
    int is_gui = 1; // 預設為 GUI 模式
    
    // 檢查 mbd 的 bit 2，確認 cmdline 是否有效
    if (mbd->flags & (1 << 2)) {
        char* cmdline = (char*) mbd->cmdline;
        // 如果 GRUB 參數包含 mode=cli，就切換到 CLI 模式
        if (strstr(cmdline, "mode=cli") != 0) {
            is_gui = 0;
        }
    }

    // 3. 根據模式初始化系統介面
    enable_gui_mode(is_gui);
    terminal_set_mode(is_gui);

    if (is_gui) {
        init_gui();
        int term_win = create_window(50, 50, 368, 228, "Simple OS Terminal");
        terminal_bind_window(term_win);
        gui_render(400, 300);
        init_mouse(); // CLI 模式不需要滑鼠，所以只在 GUI 啟動
    } else {
        // CLI 模式只綁定一個無效的視窗 ID，並強制重繪全螢幕
        terminal_bind_window(-1);
        gui_redraw(); 
    }

    __asm__ volatile ("sti");

    kprintf("=== OS Subsystems Ready ===\n");
    if (is_gui) {
        kprintf("Welcome to Simple OS Desktop Environment!\n");
    } else {
        kprintf("Welcome to Simple OS Command Line Interface!\n");
    }

    // ... 後面載入 shell.elf 的邏輯完全保持不變 ...
}
```

2. 最後，打開專案根目錄的 **`grub.cfg`**，我們給使用者兩個選項：
```text
# 設定預設選擇第一個選項，並且等待 5 秒讓使用者選
set default=0
set timeout=5

menuentry "Simple OS (Desktop GUI)" {
    set gfxpayload=800x600x32
    # 傳遞 mode=gui 參數
    multiboot /boot/myos.bin mode=gui
    module /boot/shell.elf
    module /boot/echo.elf
    module /boot/cat.elf
    module /boot/ping.elf
    module /boot/pong.elf
    module /boot/touch.elf
    module /boot/ls.elf
    module /boot/rm.elf
    module /boot/mkdir.elf
}

menuentry "Simple OS (Command Line)" {
    set gfxpayload=800x600x32
    # 傳遞 mode=cli 參數
    multiboot /boot/myos.bin mode=cli
    module /boot/shell.elf
    module /boot/echo.elf
    module /boot/cat.elf
    module /boot/ping.elf
    module /boot/pong.elf
    module /boot/touch.elf
    module /boot/ls.elf
    module /boot/rm.elf
    module /boot/mkdir.elf
}
```

---

存檔，執行：
```bash
make clean && make run
```

啟動後，**QEMU 畫面會停留在 GRUB 選單 5 秒鐘！**
你可以用鍵盤的上下鍵選擇 `Simple OS (Command Line)` 按下 Enter。系統就會進入純黑底色、無邊界、可以打滿整個螢幕的經典 Hacker 介面（但底層依舊享受著 60 FPS 雙重緩衝的頂級效能）！
重新開機選擇 `Desktop GUI`，你那華麗的 90 年代漸層視窗桌面又會回來陪伴你！

這就是現代作業系統 Bootloader 解析參數的精髓！
