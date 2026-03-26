很高興你順利掌握了記憶體與堆疊的底層運作！有了這個紮實的基礎，我們就可以放心大膽地在 C 語言的世界裡擴充系統功能了。

現在，我們正式進入 **Day 3：打造真正的終端機 (Terminal) 與螢幕滾動**。

昨天的 `terminal_writestring` 雖然能印出字，但它非常笨：它只會傻傻地從左上角開始覆蓋過去，如果遇到換行符號 `\n`，它會印出亂碼；如果字串超過螢幕長度（25 行 x 80 列），它會直接寫入未知的記憶體，導致系統崩潰。

今天的目標，就是讓我們的作業系統擁有一個「像樣」的文字終端機。

---

### Day 3 授課目標

1. **游標狀態管理：** 讓系統記住目前印到第幾行 (`row`)、第幾列 (`column`)。
2. **支援換行字元：** 當遇到 `\n` 時，將游標移動到下一行的開頭。
3. **實作螢幕滾動 (Scrolling)：** 當文字填滿最後一行時，將所有文字往上推一行，並清空最底層。

---

### 實作步驟：升級你的 `kernel.c`

我們需要大幅改寫昨天的 `kernel.c`。這次我們會加入全域變數來追蹤狀態，並拆分出專門處理「單一字元」的函式 `terminal_putchar`。

請將你的 `day01/kernel.c` (或 `day02/kernel.c`) 更新為以下完整內容：

```c
#include <stdint.h>
#include <stddef.h>

volatile uint16_t* vga_buffer = (uint16_t*)0xB8000;
const size_t VGA_COLS = 80;
const size_t VGA_ROWS = 25;

// 系統狀態變數：追蹤目前的游標位置與預設顏色
size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;

// 輔助函式：設定特定位置的字元與顏色
void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_COLS + x;
    vga_buffer[index] = ((uint16_t)color << 8) | c;
}

// 輔助函式：螢幕往上滾動一行
void terminal_scroll() {
    // 將第 1~24 行的內容，搬移到第 0~23 行
    for (size_t y = 1; y < VGA_ROWS; y++) {
        for (size_t x = 0; x < VGA_COLS; x++) {
            const size_t src_index = y * VGA_COLS + x;
            const size_t dest_index = (y - 1) * VGA_COLS + x;
            vga_buffer[dest_index] = vga_buffer[src_index];
        }
    }
    // 清空最後一行 (第 24 行)
    for (size_t x = 0; x < VGA_COLS; x++) {
        terminal_putentryat(' ', terminal_color, x, VGA_ROWS - 1);
    }
    // 滾動後，游標回到最後一行的開頭
    terminal_row = VGA_ROWS - 1;
}

// 核心函式：處理單一字元的邏輯 (包含換行與邊界偵測)
void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        terminal_row++;
    } else {
        terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
        terminal_column++;
        // 如果到了畫面最右邊，自動換行
        if (terminal_column == VGA_COLS) {
            terminal_column = 0;
            terminal_row++;
        }
    }

    // 如果游標超出了最底層，觸發滾動
    if (terminal_row == VGA_ROWS) {
        terminal_scroll();
    }
}

// 更新版的字串列印函式
void terminal_writestring(const char* data) {
    size_t index = 0;
    while (data[index] != '\0') {
        terminal_putchar(data[index]);
        index++;
    }
}

void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = 0x0F; // 黑底白字
    for (size_t y = 0; y < VGA_ROWS; y++) {
        for (size_t x = 0; x < VGA_COLS; x++) {
            terminal_putentryat(' ', terminal_color, x, y);
        }
    }
}

void kernel_main(void) {
    terminal_initialize();
    
    // 測試換行功能
    terminal_writestring("Hello, System Architect!\n");
    terminal_writestring("Welcome to Day 3.\n");
    
    // 測試螢幕滾動功能 (印出 30 行，迫使畫面捲動)
    for (int i = 0; i < 30; i++) {
        terminal_color = 0x0A + (i % 5); // 讓顏色稍微變化一下
        terminal_writestring("Scrolling line testing...\n");
    }
    
    terminal_color = 0x0E; // 黃色
    terminal_writestring("Scroll test complete. The system is stable.\n");
}

```

---

### 編譯與測試

這次的 `Makefile` 與 `boot.S` 完全不需要修改。請直接在終端機執行：

```bash
make clean
make
qemu-system-i386 -cdrom myos.iso

```

**預期結果：**
你會看到螢幕快速印出文字，並且畫面會順利往上「推」（滾動）。最後一行會停留在黃色的字樣 `"Scroll test complete. The system is stable."`。

---

### 🤔 今日思考題

為了培養你作為系統核心開發者的直覺，請在順利跑出畫面後，思考以下兩個問題：

1. **硬體游標去哪了？** 雖然我們的程式碼裡有 `terminal_row` 和 `terminal_column` 來記錄「軟體」游標的位置，但你應該有發現，畫面上並沒有那個熟悉的「一閃一閃的底線游標」。如果要讓 VGA 顯示卡把那個閃爍的游標移動到我們指定的位置，你覺得應該要怎麼做？（提示：不能只改記憶體，需要與硬體連接埠 I/O Port 溝通）。
2. **效能優化：** 在 `terminal_scroll` 函式中，我們使用了兩個巢狀的 `for` 迴圈來一個一個字元（2 bytes）搬移記憶體。如果這是一台古老的 386 電腦，這樣做會非常慢。在 C 語言中，有沒有什麼標準的做法或觀念，可以一次搬移一大塊記憶體？（提示：我們還沒有標準函式庫，但未來我們可以自己實作這個功能）。

**準備好更新程式碼並執行看看了嗎？如果有任何心得或是對思考題的想法，隨時跟我分享！**
