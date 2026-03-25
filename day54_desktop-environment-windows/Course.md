太棒了，Rick！🎉 看到那句「滑鼠可以移動了」，我隔著螢幕都忍不住為你歡呼！

這是一個極具歷史意義的時刻！你的 Simple OS 正式從「純鍵盤輸入的批次處理時代」，跨越到了「人類直覺互動的圖形指標時代」。你現在已經具備了現代作業系統最核心的互動靈魂！

既然我們有了畫布，也有了能在上面自由穿梭的滑鼠，接下來 **Day 54** 的任務呼之欲出：**打造桌面環境與視窗 (Desktop Environment & Windows)！** 🖼️



一個真正的 GUI 系統不能只有黑漆漆的背景和終端機文字。我們要來畫出充滿復古 Windows 95 風格的「桌面底色」以及帶有「標題列」的視窗！

請跟著我進行這 3 個步驟，讓你的 OS 擁有真正的「顏值」：

---

### 步驟 1：升級圖形引擎，學會「寫字」與「畫視窗」

我們現在只有 `draw_char`（畫單一字母），但畫視窗標題時，我們需要能直接在特定座標畫出「一整串字」。

請打開 **`lib/include/gfx.h`**，新增這兩個宣告：
```c
// 在特定座標畫出一串字
void draw_string(int x, int y, const char* str, uint32_t fg_color, uint32_t bg_color);

// 畫出一個標準的 GUI 視窗
void draw_window(int x, int y, int width, int height, const char* title);
```

接著，打開 **`lib/gfx.c`**，在檔案下方（游標系統之上）加入這兩個函式的實作：

```c
// 【新增】在指定座標畫出連續字串
void draw_string(int x, int y, const char* str, uint32_t fg_color, uint32_t bg_color) {
    int curr_x = x;
    for (int i = 0; str[i] != '\0'; i++) {
        draw_char(str[i], curr_x, y, fg_color, bg_color);
        curr_x += 8; // 每個字元寬 8 像素
    }
}

// 【新增】畫出帶有標題列的復古視窗！
void draw_window(int x, int y, int width, int height, const char* title) {
    // 1. 視窗主體 (經典的亮灰色 0xC0C0C0)
    draw_rect(x, y, width, height, 0xC0C0C0);
    
    // 2. 視窗邊框 (簡單的立體陰影)
    // 上邊與左邊 (白色反光)
    draw_rect(x, y, width, 2, 0xFFFFFF); 
    draw_rect(x, y, 2, height, 0xFFFFFF);
    // 下邊與右邊 (深灰色陰影)
    draw_rect(x, y + height - 2, width, 2, 0x808080);
    draw_rect(x + width - 2, y, 2, height, 0x808080);

    // 3. 標題列 (經典的深藍色 0x000080)
    // 留出邊框的空間 (x+2, y+2)，高度設為 18 像素
    draw_rect(x + 2, y + 2, width - 4, 18, 0x000080);

    // 4. 標題文字 (白色字，深藍色底)
    // 微調座標讓字體置中於標題列
    draw_string(x + 6, y + 7, title, 0xFFFFFF, 0x000080);
}
```

---

### 步驟 2：改造終端機底色 (`lib/tty.c`)

為了讓桌面有「桌面」的感覺，我們不再把整個螢幕塗黑了。我們要把它塗成經典的「Windows 95 藍綠色 (Teal)」。

請打開 **`lib/tty.c`**，找到 `terminal_initialize`，把底色換掉：

```c
// 把這行原本的黑色換成深青色 (Teal)
#define TERM_BG 0x008080 

void terminal_initialize(void) {
    term_x = 0;
    term_y = 0;
    // 這樣一來，背景就會變成漂亮的復古藍綠色桌面！
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, TERM_BG); 
}
```

---

### 步驟 3：在桌面上開啟你的第一個視窗 (`lib/kernel.c`)

現在，我們要指揮 Kernel 在開機時，幫我們把這個視窗畫出來！

**⚠️ 關鍵細節：** 繪製順序非常重要！我們必須在「啟動滑鼠 (`init_mouse`)」**之前**畫好視窗。因為滑鼠一啟動，就會擷取當時的「背景」當作游標底圖。如果先啟動滑鼠再畫視窗，滑鼠的底圖就會出錯。

請打開 **`lib/kernel.c`**，修改 `kernel_main`：

```c
void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
    (void)magic; 

    // terminal_initialize 會幫我們把背景塗成藍綠色桌面
    terminal_initialize(); 
    
    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384);
    init_kheap();

    // 1. 初始化圖形引擎
    init_gfx(mbd);

    // ==========================================
    // 2. 🎨 繪製 GUI 元素 (必須在啟動滑鼠之前！)
    // ==========================================
    // 在畫面右半邊，畫一個系統狀態視窗！
    draw_window(450, 100, 300, 200, "System Status");
    // 在視窗裡面寫點字 (底色要配重視窗的灰色 0xC0C0C0)
    draw_string(460, 130, "CPU: x86 32-bit", 0x000000, 0xC0C0C0);
    draw_string(460, 150, "Memory: 16 MB", 0x000000, 0xC0C0C0);
    draw_string(460, 170, "GUI: Active", 0x000000, 0xC0C0C0);

    // 3. 啟動滑鼠驅動 (它會擷取畫好視窗後的背景)
    init_mouse();

    __asm__ volatile ("sti");

    // 左上角的終端機文字會自然地印在藍綠色的桌面上
    kprintf("=== OS Subsystems Ready ===\n\n");
    kprintf("Welcome to Simple OS Desktop Environment!\n");

    while (1) { __asm__ volatile ("hlt"); }
}
```

---

### 🚀 迎接你的視窗時代

存檔，執行你的光榮指令：
```bash
make clean && make run
```

當 QEMU 亮起時，你將會看到一個極具年代感的藍綠色桌面、流暢的黑色打字終端機，以及右邊那個帶有立體邊框和深藍色標題列的「System Status」視窗！
最棒的是，你的滑鼠可以完美地在這個視窗上面滑動！

這代表你的系統已經正式升級為「桌面作業系統 (Desktop OS)」了！成功畫出視窗後記得截圖給我看，接下來我們要思考怎麼讓這些視窗「動起來」了！😎
