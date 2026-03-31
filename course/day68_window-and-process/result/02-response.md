哈哈，Rick！你說得完全沒錯，這真的是個很經典的「硬體驅動未完成」問題！😂 

你剛剛在寫背景執行的 `status &` 時，一定發現不管怎麼按 `Shift + 7`，畫面上永遠只會出現 `7`，對吧？

去看看你的 `keyboard_handler`，我們當初為了偷懶，實作 Shift 邏輯的時候，**只寫了「小寫轉大寫 (`- 32`)」以及「單引號轉雙引號 (`0x28`)」**，根本沒有教 Kernel 怎麼處理鍵盤上方的那些符號鍵（`!@#$%^&*()`）！

在真實的作業系統中，最標準且乾淨的做法是**建立第二張「Shift 狀態專用的對應表 (Shift Keymap)」**，這樣就不用寫一堆醜醜的 `if-else` 了。

讓我們用 1 分鐘把它補齊，讓你的鍵盤正式升級為「全功能 US 鍵盤」！

### 🛠️ 終極修復：加入 Shift Keymap (`src/kernel/lib/keyboard.c`)

請打開 **`src/kernel/lib/keyboard.c`**，把原本的 `kbdus` 陣列下方補上 `kbdus_shift` 陣列，並大幅簡化你的 `keyboard_handler`：

```c
#include <stdint.h>
#include <stddef.h>
#include "keyboard.h"
#include "io.h"
#include "tty.h"
#include "utils.h"
#include "task.h"

#define KBD_BUFFER_SIZE 256
volatile char kbd_buffer[KBD_BUFFER_SIZE];
volatile int kbd_head = 0;
volatile int kbd_tail = 0;

// 一般狀態的鍵盤對應表
unsigned char kbdus[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,
  '*',  0,  ' ',  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0, '-',   0,   0,   0, '+',   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

// 【Day 68 課前補充】按住 Shift 時的鍵盤對應表！
unsigned char kbdus_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
  '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',   0,
  '*',  0,  ' ',  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0, '-',   0,   0,   0, '+',   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

static int shift_pressed = 0;

// --- 公開 API ---

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);

    // 1. 攔截 Shift 的狀態改變
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
    }
    else if (scancode == 0xAA || scancode == 0xB6) {
        shift_pressed = 0;
    }
    // 2. 一般的按鍵按下 (Make Code)
    else if (!(scancode & 0x80)) {
        char ascii = 0;

        // 【修改】查表邏輯變得超級乾淨！直接根據 Shift 狀態選表！
        if (shift_pressed) {
            ascii = kbdus_shift[scancode];
        } else {
            ascii = kbdus[scancode];
        }

        // 把最終決定的字元塞進 Buffer 交給 User App
        if (ascii != 0) {
            kbd_buffer[kbd_head] = ascii;
            kbd_head = (kbd_head + 1) % KBD_BUFFER_SIZE;
        }
    }

    // 通知 PIC 中斷處理完畢
    outb(0x20, 0x20);
}

char keyboard_getchar() {
    while (kbd_head == kbd_tail) {
        __asm__ volatile("sti");
        schedule();  // 讓出 CPU
        __asm__ volatile("hlt");
    }
    __asm__ volatile("cli");
    char c = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUFFER_SIZE;
    __asm__ volatile("sti");
    return c;
}
```

---

### 🚀 繼續享受背景執行的魔力！

存檔編譯 (`make clean && make run`) 後，你的鍵盤終於徹底「解印」了！
現在你可以順暢無阻地打出 `status &` 或是任何需要 `|`, `<`, `>` 的管線符號，為未來的進階 Shell 指令打好基礎。

現在視窗伺服器 (Day 68) 已經穩穩就位，如果有需要，隨時告訴我，我們立刻點燃 **Day 69：Shared Memory** 的戰火，讓你的視窗可以開始畫圖形！😎
