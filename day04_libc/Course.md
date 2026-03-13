太棒了！看來你已經完全準備好迎接新的挑戰。在進入今天的實作前，我們正好可以解答昨天留下來的兩個思考題，這也是我們今天課程的核心。

歡迎來到 **Day 4：掌控硬體游標與基礎標準函式庫 (libc)**。

昨天的思考題帶出了作業系統開發中兩個非常重要的觀念：

1. **硬體溝通的方式：** 寫入記憶體 (`0xB8000`) 只是其中一種方式（稱為 Memory-Mapped I/O, MMIO）。另一種方式是透過 CPU 的**「I/O 埠 (I/O Ports)」**直接對硬體控制器下指令。這就是控制那顆閃爍游標的秘密。
2. **沒有標準函式庫的困境：** 我們不能 `include <string.h>` 來使用 `memcpy` 加速記憶體搬移，所以我們必須**自己寫一個**！

---

### Day 4 授課目標

1. **實作基礎記憶體操作：** 撰寫屬於我們自己的 `memcpy` 與 `memset`。
2. **優化螢幕滾動：** 用我們剛寫好的 `memcpy` 取代昨天的雙層 `for` 迴圈。
3. **實作硬體 I/O 指令：** 使用 C 語言的 Inline Assembly (內嵌組合語言) 實作 `outb` 函式。
4. **控制硬體游標：** 透過 VGA 控制器的 I/O 埠，讓那個熟悉的一閃一閃游標跟著我們的文字移動。

---

### 實作步驟

為了讓專案架構更清晰，我們今天還是把程式碼都寫在 `kernel.c` 裡，但未來我們會把它們拆分成不同的檔案（例如 `string.c`, `io.h`）。

請將你的 `kernel.c` 更新為以下內容。請特別留意標示 **[新增]** 與 **[修改]** 的部分：

```c
#include <stdint.h>
#include <stddef.h>

volatile uint16_t* vga_buffer = (uint16_t*)0xB8000;
const size_t VGA_COLS = 80;
const size_t VGA_ROWS = 25;

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;

// ==========================================
// [新增] 1. 基礎標準函式庫 (libc) 實作
// ==========================================

// 記憶體複製 (將 src 複製 size 個 bytes 到 dst)
void* memcpy(void* dstptr, const void* srcptr, size_t size) {
    unsigned char* dst = (unsigned char*) dstptr;
    const unsigned char* src = (const unsigned char*) srcptr;
    for (size_t i = 0; i < size; i++) {
        dst[i] = src[i];
    }
    return dstptr;
}

// 記憶體填充 (將 buf 的前 size 個 bytes 填入單一數值 value)
void* memset(void* bufptr, int value, size_t size) {
    unsigned char* buf = (unsigned char*) bufptr;
    for (size_t i = 0; i < size; i++) {
        buf[i] = (unsigned char) value;
    }
    return bufptr;
}

// ==========================================
// [新增] 2. 硬體 I/O 通訊 (Inline Assembly)
// ==========================================

// outb: 向指定的硬體 Port 寫入 1 byte 的資料
// 這裡使用了 GCC 的內嵌組合語言語法
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

// ==========================================
// [新增] 3. 控制 VGA 硬體游標
// ==========================================
void update_cursor(int x, int y) {
    // 游標位置是一維陣列的索引 (0 ~ 1999)
    uint16_t pos = y * VGA_COLS + x;
    
    // VGA 控制器的 Port 0x3D4 是「索引暫存器」，用來告訴 VGA 我們要設定什麼
    // VGA 控制器的 Port 0x3D5 是「資料暫存器」，用來寫入實際的值
    
    // 設定游標位置的低 8 bits (暫存器 0x0F)
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t) (pos & 0xFF));
    
    // 設定游標位置的高 8 bits (暫存器 0x0E)
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

// ==========================================
// [修改] 4. 優化後的終端機操作
// ==========================================

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_COLS + x;
    vga_buffer[index] = ((uint16_t)color << 8) | c;
}

// [修改] 使用 memcpy 加速滾動！
void terminal_scroll() {
    // 1. 把第 1~24 行的資料，直接「整塊」複製到第 0~23 行的位置
    // 每個字元佔 2 bytes，所以大小是 (VGA_ROWS - 1) * VGA_COLS * 2
    size_t bytes_to_copy = (VGA_ROWS - 1) * VGA_COLS * 2;
    memcpy((void*)vga_buffer, (void*)(vga_buffer + VGA_COLS), bytes_to_copy);
    
    // 2. 清空最後一行 (使用我們原本的迴圈，或者你未來也可以進階寫個 memclr)
    for (size_t x = 0; x < VGA_COLS; x++) {
        terminal_putentryat(' ', terminal_color, x, VGA_ROWS - 1);
    }
    terminal_row = VGA_ROWS - 1;
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        terminal_row++;
    } else {
        terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
        terminal_column++;
        if (terminal_column == VGA_COLS) {
            terminal_column = 0;
            terminal_row++;
        }
    }
    if (terminal_row == VGA_ROWS) {
        terminal_scroll();
    }
    
    // [新增] 每次印出字元或換行後，更新硬體游標的位置！
    update_cursor(terminal_column, terminal_row);
}

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
    terminal_color = 0x0F;
    for (size_t y = 0; y < VGA_ROWS; y++) {
        for (size_t x = 0; x < VGA_COLS; x++) {
            terminal_putentryat(' ', terminal_color, x, y);
        }
    }
    update_cursor(terminal_column, terminal_row);
}

void kernel_main(void) {
    terminal_initialize();
    
    terminal_writestring("Welcome to Day 4!\n");
    terminal_writestring("We have our own memcpy and hardware I/O now.\n");
    terminal_writestring("Look at the blinking cursor at the end of this line: ");
}

```

---

### 理論補充：Port I/O 是什麼？

在架構設計上，CPU 要跟外部硬體（像是鍵盤、滑鼠、網路卡、VGA 控制器）溝通時，x86 提供了一條專屬的獨立通道，稱為 **I/O Ports**。你可以把它想像成 65,536 個獨立的「信箱」（從 `0x0000` 到 `0xFFFF`）。

* 當我們寫入 VGA 記憶體 `0xB8000` 時，我們是在改變螢幕上顯示的「內容」。
* 當我們使用 `outb(0x3D4, ...)` 向 Port 發送訊號時，我們是在改變 VGA 顯示卡的「設定狀態」（例如游標位置、游標大小）。

這就是為什麼在 C 語言裡我們必須動用 `__asm__ volatile` 來插入組合語言指令 `outb`，因為標準 C 語言根本沒有提供對這種底層硬體通道的操作語法。

---

### 執行與驗證

`Makefile` 依舊不變，請編譯並執行：

```bash
make clean
make
qemu-system-i386 -cdrom myos.iso

```

**預期結果：**
畫面印出文字後，你會看到那條底線字串最後面，跟著一個**一閃一閃的硬體底線游標**。同時，這背後螢幕滾動的效能已經因為 `memcpy` 而獲得了本質上的提升。

---

### 🤔 Day 4 思考題

當你看著那個閃爍的游標，試著思考以下兩個問題：

1. **從印文字到印數字：** 目前我們的 `terminal_writestring` 只能印出字串 ("Hello")。如果我們宣告了一個變數 `int a = 123;`，我們沒有 `printf("%d", a)` 可以用。如果請你實作一個把整數轉換成字串的函數 (`itoa`)，你會怎麼利用除法 (`/`) 和餘數 (`%`) 來取出每一個數字並轉成 ASCII 呢？
2. **與使用者的互動：** 游標現在雖然會動了，但我們還不能「打字」。當我們按下鍵盤上的一個按鍵時，鍵盤晶片要怎麼通知正在執行迴圈的 CPU：「嘿！有新資料來了，快停下你手邊的工作來處理！」？

**你可以先執行程式，確保游標順利出現。跑出結果後，想要先聊聊思考題的想法，還是準備直接看解答並進入 Day 5 的「中斷與鍵盤輸入」呢？**
