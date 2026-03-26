太棒了！你的進度非常順利。

身為有豐富 SRE 與架構經驗的工程師，你一定非常清楚「可觀測性 (Observability)」與「日誌 (Logging)」在系統底層有多麼重要。

在我們真正進入「中斷 (Interrupts)」與「硬體驅動 (如鍵盤)」這片深水區之前，我們必須先為自己打造一把除錯利器。如果系統崩潰了，或者我們想觀察記憶體位址，總不能一直用肉眼數游標位置，或只能印出寫死的字串吧？

這就是我們 **Day 5：打造核心除錯神器 — 專屬的 `kprintf**` 的目標！這也是解答昨天第一個思考題的最佳實作。

---

### Day 5 授課目標

1. **實作 `itoa` (Integer to ASCII)：** 將整數轉換成字元陣列，支援十進位與十六進位。
2. **掌握不定長度引數 (`stdarg.h`)：** 學習 C 語言如何處理數量不固定的函式參數。
3. **實作核心版的格式化輸出 (`kprintf`)：** 支援 `%d` (整數)、`%x` (十六進位)、`%s` (字串) 與 `%c` (字元)。

---

### 理論補充：如何把整數變成字串？

要把數字 `123` 變成字串 `"123"`，我們需要反向操作：

1. `123 % 10 = 3` (轉成字元 `'3'`)
2. `123 / 10 = 12`
3. `12 % 10 = 2` (轉成字元 `'2'`)
4. `12 / 10 = 1`
5. `1 % 10 = 1` (轉成字元 `'1'`)
6. `1 / 10 = 0` (結束)

取出來的順序是 `'3'`, `'2'`, `'1'`，所以最後我們還需要把這個字串「反轉 (Reverse)」過來。十六進位也是一樣的邏輯，只是除數和餘數變成 16。

---

### 實作步驟：擴充 `kernel.c`

因為我們的環境是 `-ffreestanding`，我們不能用標準函式庫的 `printf`，但編譯器有提供 `<stdarg.h>` 標頭檔讓我們處理不定參數。

請打開 `kernel.c`，在檔案最上方加入 `<stdarg.h>` 和 `<stdbool.h>`，並加入以下的實作：

```c
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>   // [新增] 用於處理不定長度引數
#include <stdbool.h>  // [新增] 支援 bool 型別

// ... (保留先前的 vga_buffer, 狀態變數, memcpy, memset, outb, update_cursor 等) ...
// ... (保留先前的 terminal_putentryat, terminal_scroll, terminal_putchar, terminal_writestring, terminal_initialize) ...

// ==========================================
// [新增] 5. 字串與數字處理工具
// ==========================================

// 輔助函式：反轉字串
void reverse_string(char* str, int length) {
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

// 核心工具：整數轉字串 (itoa)
// value: 要轉換的數字, str: 存放結果的陣列, base: 進位制 (10或16)
void itoa(int value, char* str, int base) {
    int i = 0;
    bool is_negative = false;

    // 處理 0 的特例
    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    // 處理負數 (僅限十進位)
    if (value < 0 && base == 10) {
        is_negative = true;
        value = -value;
    }

    // 逐位取出餘數
    while (value != 0) {
        int rem = value % base;
        // 如果是大於 9 的餘數 (用於16進位)，轉換成 A~F
        str[i++] = (rem > 9) ? (rem - 10) + 'A' : rem + '0';
        value = value / base;
    }

    // 補上負號
    if (is_negative) {
        str[i++] = '-';
    }

    str[i] = '\0'; // 字串結尾符號

    // 反轉字串，因為我們是從個位數開始取出的
    reverse_string(str, i);
}

// ==========================================
// [新增] 6. 核心專屬格式化輸出 (kprintf)
// ==========================================

// 簡易版的 printf
void kprintf(const char* format, ...) {
    va_list args;
    va_start(args, format); // 初始化不定參數列表

    for (size_t i = 0; format[i] != '\0'; i++) {
        // 如果不是 '%'，就當作一般字元直接印出
        if (format[i] != '%') {
            terminal_putchar(format[i]);
            continue;
        }

        // 遇到 '%'，看下一個字元是什麼來決定格式
        i++; 
        switch (format[i]) {
            case 'd': { // 十進位整數
                int num = va_arg(args, int);
                char buffer[32];
                itoa(num, buffer, 10);
                terminal_writestring(buffer);
                break;
            }
            case 'x': { // 十六進位整數
                int num = va_arg(args, int);
                char buffer[32];
                terminal_writestring("0x"); // 加上 0x 前綴
                itoa(num, buffer, 16);
                terminal_writestring(buffer);
                break;
            }
            case 's': { // 字串
                char* str = va_arg(args, char*);
                terminal_writestring(str);
                break;
            }
            case 'c': { // 單一字元
                // char 在變數傳遞時會被提升為 int
                char c = (char) va_arg(args, int);
                terminal_putchar(c);
                break;
            }
            case '%': { // 印出 '%' 本身
                terminal_putchar('%');
                break;
            }
            default: // 未知的格式，原樣印出
                terminal_putchar('%');
                terminal_putchar(format[i]);
                break;
        }
    }

    va_end(args); // 清理不定參數列表
}

// ==========================================
// [修改] 主程式測試
// ==========================================

void kernel_main(void) {
    terminal_initialize();
    
    // 現在我們有強大的 kprintf 可以用了！
    kprintf("=== OS Kernel Booting ===\n");
    kprintf("Module: %s loaded successfully.\n", "VGA Driver");
    
    int ram_mb = 16;
    kprintf("System RAM detected: %d MB\n", ram_mb);
    
    // 測試十六進位與記憶體位址輸出
    int magic_number = 3735928559; // 猜猜這是什麼？
    kprintf("Magic Number in Hex: %x\n", magic_number);
    
    // 印出 VGA buffer 的記憶體位址
    kprintf("VGA Buffer starts at address: %x\n", (int)vga_buffer);
    
    kprintf("Initialization %d%% complete.\n", 100);
}

```

---

### 執行與驗證

依舊使用相同的指令：

```bash
make clean
make
qemu-system-i386 -cdrom myos.iso

```

**預期結果：**
畫面上會非常有模有樣地印出系統資訊！
特別注意 `Magic Number in Hex`，它應該會印出 `0xdeadbeef`（這是軟體工程界最經典的魔術數字之一）。並且你也會看到 VGA Buffer 被正確解析出 `0xb8000` 這個十六進位位址。

---

### 🤔 Day 5 思考題

有了這把除錯大劍，明天我們就要準備迎戰作業系統中最複雜的野獸：「中斷 (Interrupts) 與例外處理 (Exceptions)」。在開戰前，請思考：

1. **危險的除以零：** 如果我們在目前的 `kernel_main` 裡面，故意寫一行 `int a = 10 / 0;` 並執行它，你覺得 QEMU 會發生什麼事？CPU 遇到這種算術錯誤時，會怎麼通知系統？
2. **Buffer Overflow 的隱患：** 在我們的 `itoa` 函式中，我們宣告了 `char buffer[32];`。為什麼大小是 32？對於一個 32 位元的整數 (int) 來說，這足夠安全嗎？

**恭喜你完成了第一階段所有基礎建設！跑出畫面後，告訴我你的想法，或者我們就可以著手規劃 Day 6 的 GDT (Global Descriptor Table) 囉！**
