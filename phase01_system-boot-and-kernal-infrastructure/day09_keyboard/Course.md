太棒了！看到截圖裡一整排順利觸發的 Scancode，這畫面真的超級療癒，這代表你的軟體已經成功跟硬體接上線了！

而且，**你想要重構的直覺非常精準！** 身為有豐富經驗的架構師與開發者，看到一個不斷膨脹、充滿各種底層操作的「God File (`kernel.c`)」肯定會全身不對勁。作業系統開發到這個階段，如果不及時重構，後面的記憶體管理跟檔案系統絕對會把我們逼瘋。

既然你提出了這個需求，我們 **Day 9 的主題就是：核心大重構與打字機誕生**！

---

### Day 9 授課目標

1. **升級 Makefile：** 讓它自動抓取所有 `.c` 和 `.S` 檔，以後新增檔案再也不用手動改 Makefile。
2. **模組化系統架構：** 將 `kernel.c` 拆分成終端機驅動、基礎函式庫、鍵盤驅動等獨立模組。
3. **實作 Scancode 轉換表：** 把冷冰冰的 Hex Code 轉換成我們看得懂的英文字母。

---

### 步驟 1：升級你的自動化武器 (Makefile)

當檔案變多時，手動維護 `.o` 檔案清單太痛苦了。我們可以利用 `make` 的 `wildcard` 功能自動尋找檔案。

請將你的 `Makefile` 的編譯規則部分更新為以下這樣：

```makefile
DOCKER_CMD = docker run --platform linux/amd64 --rm -v $(PWD):/workspace -w /workspace os-builder
CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra

# 自動抓取所有的 .c 與 .S 檔案
C_SOURCES = $(wildcard *.c)
C_OBJS = $(C_SOURCES:.c=.o)
ASM_SOURCES = $(wildcard *.S)
ASM_OBJS = $(ASM_SOURCES:.S=.o)

# 把所有 .o 檔統整起來 (注意 boot.o 必須在前面，雖然 linker.ld 裡有指定 ENTRY，但放前面更保險)
OBJS = boot.o $(filter-out boot.o, $(ASM_OBJS)) $(C_OBJS)

all: build-env myos.iso

build-env:
	docker build -t os-builder .

myos.iso: myos.bin grub.cfg
	mkdir -p isodir/boot/grub
	cp myos.bin isodir/boot/myos.bin
	cp grub.cfg isodir/boot/grub/grub.cfg
	$(DOCKER_CMD) grub-mkrescue -o myos.iso isodir
	rm -rf isodir

myos.bin: $(OBJS)
	$(DOCKER_CMD) ld -m elf_i386 -n -T linker.ld -o myos.bin $(OBJS)

%.o: %.c
	$(DOCKER_CMD) gcc $(CFLAGS) -c $< -o $@

%.o: %.S
	$(DOCKER_CMD) nasm -f elf32 $< -o $@

clean:
	rm -f *.o *.bin *.iso
	rm -rf isodir

```

*現在，你不管建立多少個 `.c` 檔，只要下 `make` 就會自動幫你全部編譯好！*

---

### 步驟 2：核心架構重構 Blueprint

我建議將原本擠在 `kernel.c` 裡的東西，拆分成以下幾個檔案：

#### 1. `tty.c` 與 `tty.h` (Teletypewriter / 終端機顯示驅動)

把 VGA 相關的東西全搬過來。

* **包含：** `vga_buffer`、`terminal_row/column`、`terminal_initialize`、`terminal_putchar`、`terminal_writestring`、`update_cursor`、`terminal_scroll`。

#### 2. `utils.c` 與 `utils.h` (基礎工具 / libc 雛形)

我們自己手刻的標準函式庫。

* **包含：** `memcpy`、`memset`、`itoa`、`reverse_string`、`kprintf`。
* *(記得在 `utils.c` 裡 `#include "tty.h"`，因為 `kprintf` 需要呼叫 `terminal_putchar`)*。

#### 3. `keyboard.c` 與 `keyboard.h` (鍵盤驅動)

這正是我們今天要豐富的主角！請建立這兩個檔案。

**`keyboard.h`**

```c
#ifndef KEYBOARD_H
#define KEYBOARD_H

void keyboard_handler(void);

#endif

```

**`keyboard.c`**
鍵盤送來的 Scancode 其實就是這個陣列的 Index。我們建立一個對應表（Lookup Table），直接查表就能得到 ASCII 字元！

```c
#include "keyboard.h"
#include "io.h"
#include "utils.h"

// 美式鍵盤的 Scancode 對應表 (無 Shift 狀態)
// 例如: 按下 'A' 的 Scancode 是 0x1E (十進位 30)，kbd_US[30] 就是 'a'
const unsigned char kbd_US[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',   
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',   
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,   
  '*',  0,  ' ',  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0, '-',   0,   0,   0, '+',   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);

    // 如果最高位元是 0 (代表按下按鍵)
    if (!(scancode & 0x80)) {
        // 從對應表中抓出字元
        char c = kbd_US[scancode];
        
        // 如果這個字元存在 (不是 0)，就把它印出來！
        if (c != 0) {
            // [注意] 這裡我們先借用 kprintf 來印單一字元
            kprintf("%c", c);
        }
    }

    // 發送 EOI 給 PIC
    outb(0x20, 0x20);
}

```

#### 4. 瘦身後的 `kernel.c`

重構完後，你的 `kernel.c` 將會變得非常乾淨，這才是作業系統進入點該有的樣子：

```c
#include "tty.h"
#include "utils.h"
#include "gdt.h"
#include "idt.h"

void kernel_main(void) {
    terminal_initialize();
    kprintf("=== OS Kernel Booting ===\n");
    
    init_gdt();
    init_idt();
    
    __asm__ volatile ("sti");

    kprintf("System is ready. Start typing!\n");
    kprintf("> ");

    while (1) {
        __asm__ volatile ("hlt");
    }
}

```

---

### 執行與驗證

重構是最容易出現小 Typo 或 `#include` 忘記加的時候，請發揮你強大的工程師本能，把這幾個檔案切好。

然後執行：

```bash
make clean
make
qemu-system-i386 -cdrom myos.iso

```

**預期結果：**
畫面依舊漂亮地開機。但這一次，當你在 QEMU 裡面敲打鍵盤時，**你打的英文字母跟數字會直接出現在畫面上**！你已經完成了一個擁有基礎輸入與輸出 (I/O) 功能的微型作業系統了！

**這個重構工程可能需要花一點時間搬移程式碼。完成後，請跟我說一聲，我們就可以邁入下一個大魔王：記憶體分頁管理 (Paging) 囉！** 如果在搬移過程中有遇到任何 Linker 找不到函數的錯誤，隨時把錯誤丟上來。
