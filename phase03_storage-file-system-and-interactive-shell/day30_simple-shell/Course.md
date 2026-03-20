這 30 天的旅程，從第一天我們面對著 QEMU 黑漆漆的螢幕，一路點亮了 VGA、建立了 GDT/IDT、開啟了分頁機制、手刻了記憶體管理與檔案系統，到昨天我們終於戰勝了「EIP 死亡滑行」，成功讓 Kernel 從硬碟動態載入 ELF 執行檔。

這一切的努力，都是為了今天這最後一塊拼圖。

真正的作業系統，不該只是開機後默默跑完腳本就結束。它應該要有一個靈魂，能夠傾聽使用者的聲音，並為使用者服務。

歡迎來到這趟壯遊的最終章——**Day 30：賦予系統靈魂 — 實作互動式命令列 (Simple Shell)**！

---

### Day 30 授課目標

1. **攔截鍵盤輸入 (`keyboard.c`)：** 之前我們的鍵盤驅動只是把按鍵直接印在螢幕上。現在，我們要把它存進一個「環狀緩衝區 (Ring Buffer)」，留給應用程式去讀取。
2. **新增輸入系統呼叫 (`syscall.c`)：** 開設 5 號服務櫃檯 `sys_getchar`，讓 Ring 3 的平民可以安全地向 Kernel 討按鍵資料。
3. **打造 Simple Shell (`app.c`)：** 這是我們 OS 的第一支互動式應用程式！它會印出 `SimpleOS> `，解析你敲打的指令，並執行如 `help` 或 `cat` (讀取檔案) 等功能！

---

### 實作步驟：從單向輸出到雙向互動

#### 1. 打造核心鍵盤緩衝區 (`lib/keyboard.c` & `lib/keyboard.h`)

請打開 **`lib/keyboard.h`**，新增一個讀取字元的 API：

```c
#ifndef KEYBOARD_H
#define KEYBOARD_H
#include <stdint.h>

void init_keyboard();
char keyboard_getchar(); // [新增] 讓 Syscall 可以來拿字元

#endif

```

接著打開 **`lib/keyboard.c`**。我們要加入一個簡單的環狀緩衝區。請將原本的 `keyboard_handler` 修改為把字元塞進 Buffer 的模式：

```c
#include "keyboard.h"
#include "tty.h"
#include "io.h"

// 建立一個 256 bytes 的環狀緩衝區
#define KBD_BUFFER_SIZE 256
volatile char kbd_buffer[KBD_BUFFER_SIZE];
volatile int kbd_head = 0; // 寫入指標
volatile int kbd_tail = 0; // 讀取指標

// (這裡保留你原本 Day 9 寫的鍵盤掃描碼 scancode 轉 ASCII 的陣列 kbdus)
extern unsigned char kbdus[128]; 

// [修改] 中斷處理程式：不直接印出，而是存進 Buffer
void keyboard_handler() {
    uint8_t scancode = inb(0x60);
    
    if (!(scancode & 0x80)) { // 只有按下 (Press) 才處理，忽略放開 (Release)
        char ascii = kbdus[scancode];
        if (ascii != 0) {
            // 把字元存進緩衝區，並移動寫入指標
            kbd_buffer[kbd_head] = ascii;
            kbd_head = (kbd_head + 1) % KBD_BUFFER_SIZE;
        }
    }
}

// [新增] 讓 Kernel 或 Syscall 讀取字元
char keyboard_getchar() {
    // 如果緩衝區是空的，就讓 CPU 進入休眠 (hlt)，直到下一次中斷 (按鍵) 喚醒它！
    // 這是作業系統省電與等待 I/O 的核心魔法！
    while (kbd_head == kbd_tail) {
        __asm__ volatile("hlt"); 
    }
    
    char c = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUFFER_SIZE;
    return c;
}

```

#### 2. 新增 Syscall 5：取得輸入 (`lib/syscall.c`)

打開 **`lib/syscall.c`**，引入 `keyboard.h`，並在 `syscall_handler` 中加入 5 號服務：

```c
#include "tty.h"
#include "vfs.h"
#include "simplefs.h"
#include "keyboard.h" // [新增] 為了使用 keyboard_getchar()

// ... 前面的 fd_table 保留 ...

void syscall_handler(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp,
                     uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax) {
    if (eax == 2) {
        kprintf((char*)ebx);
    } 
    else if (eax == 3) {
        // ... sys_open 邏輯保留 ...
    }
    else if (eax == 4) {
        // ... sys_read 邏輯保留 ...
    }
    else if (eax == 5) {
        // [新增] Syscall 5: 讀取一個鍵盤字元
        char c = keyboard_getchar();
        
        // 為了讓 Shell 有互動感，使用者打什麼，Kernel 就幫忙印在螢幕上 (Echo)
        char str[2] = {c, '\0'};
        kprintf(str); 
        
        __asm__ volatile("mov %0, %%eax" : : "r"((uint32_t)c));
    }
}

```

#### 3. 實作平民的浪漫：Simple Shell (`app.c`)

請完全覆蓋你的 **`app.c`**。我們要為 Ring 3 的平民加入字串比對 (`strcmp`) 的能力，並寫一個無窮迴圈來接收指令！

```c
// 系統呼叫封裝
void sys_print(char* msg) {
    __asm__ volatile ("int $0x80" : : "a"(2), "b"(msg) : "memory");
}

int sys_open(char* filename) {
    int fd;
    __asm__ volatile ("int $0x80" : "=a"(fd) : "a"(3), "b"(filename) : "memory");
    return fd;
}

int sys_read(int fd, char* buffer, int size) {
    int bytes;
    __asm__ volatile ("int $0x80" : "=a"(bytes) : "a"(4), "b"(fd), "c"(buffer), "d"(size) : "memory");
    return bytes;
}

char sys_getchar() {
    int c;
    __asm__ volatile ("int $0x80" : "=a"(c) : "a"(5) : "memory");
    return (char)c;
}

// User Space 專用的字串比對工具
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// 讀取一整行指令
void read_line(char* buffer, int max_len) {
    int i = 0;
    while (i < max_len - 1) {
        char c = sys_getchar();
        if (c == '\n') {
            break; // 使用者按下了 Enter
        } else if (c == '\b') {
            if (i > 0) i--; // 處理倒退鍵 (Backspace)
        } else {
            buffer[i++] = c;
        }
    }
    buffer[i] = '\0'; // 字串結尾
}

void _start() {
    sys_print("\n======================================\n");
    sys_print("      Welcome to Simple OS Shell!     \n");
    sys_print("======================================\n");
    sys_print("Type 'help' to see available commands.\n\n");

    char cmd_buffer[128];

    while (1) {
        sys_print("SimpleOS> ");
        
        // 讀取使用者輸入的指令
        read_line(cmd_buffer, 128);

        // 執行指令邏輯
        if (strcmp(cmd_buffer, "") == 0) {
            continue;
        } 
        else if (strcmp(cmd_buffer, "help") == 0) {
            sys_print("Available commands:\n");
            sys_print("  help    - Show this message\n");
            sys_print("  cat     - Read 'hello.txt' from disk\n");
            sys_print("  about   - OS information\n");
        } 
        else if (strcmp(cmd_buffer, "about") == 0) {
            sys_print("Simple OS v1.0\nBuilt from scratch in 30 days!\n");
        }
        else if (strcmp(cmd_buffer, "cat") == 0) {
            int fd = sys_open("hello.txt");
            if (fd == -1) {
                sys_print("Error: hello.txt not found.\n");
            } else {
                char file_buf[128];
                for(int i=0; i<128; i++) file_buf[i] = 0;
                sys_read(fd, file_buf, 100);
                sys_print("--- File Content ---\n");
                sys_print(file_buf);
                sys_print("\n--------------------\n");
            }
        } 
        else {
            sys_print("Command not found: ");
            sys_print(cmd_buffer);
            sys_print("\n");
        }
    }
}

```

#### 4. 讓 Kernel 把舞台交給 Shell (`lib/kernel.c`)

最後，請打開 **`lib/kernel.c`**。
由於我們的 `my_app.elf` 現在是一支會無限循環等待輸入的 Shell，Kernel 本身就不需要再硬編碼去印出 `hello.txt` 的內容了，把這份樂趣留給使用者吧！

請把 `kernel_main` 裡面，在跳轉 (`enter_user_mode`) 之前的測試讀取邏輯（`simplefs_find("my_app.elf")` 之後的 `vfs_read` 與印出字串等）保留，但**移除掉在 Kernel 模式下印出 hello.txt 的程式碼**。只要順利把權限降級交給 `my_app.elf`，Kernel 的使命就達成了。

---

### 最終的見證 🏆

一切就緒，敲下你這 30 天來最神聖的一次指令：

```bash
make clean && make run

```

當 QEMU 畫面亮起，跑完模組載入與分頁映射後，你會看到令人感動的畫面停在：
`SimpleOS> `

這時候，**請你親自把手放在鍵盤上**，試著打出 `help`，然後按下 Enter。
接著，試著打出 `cat`，按下 Enter。

看著硬碟裡的 `hello.txt` 被你親自透過指令呼叫出來的那一刻，感受一下這種**「我創造了一個擁有自己生命週期的世界」**的震撼感。

恭喜你，你完成了從零打造作業系統的壯舉！三十天的堅持，從 Bootloader 到 VFS，從分頁錯誤到 EIP 滑行，這趟旅程你覺得如何？有沒有什麼特別想加入 Shell 的新指令呢？
