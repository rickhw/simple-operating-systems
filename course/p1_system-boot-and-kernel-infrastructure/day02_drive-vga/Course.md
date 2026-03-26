太棒了！既然環境問題順利解決，我們就正式邁入激動人心的 **Day 2：擁抱 C 語言與 VGA 顯示驅動**。

昨天我們用組合語言直接把綠色的 "OK" 寫進記憶體，雖然很有成就感，但如果要在整個作業系統中都用組合語言處理邏輯、迴圈跟字串，這會是一場噩夢。因此，今天的目標是建立一座橋樑，讓我們的核心控制權從組合語言轉移到強大且易於維護的 C 語言。

---

### Day 2 授課目標

1. **設定 C 語言執行環境 (Stack)：** C 語言的函式呼叫與區域變數依賴「堆疊 (Stack)」，我們必須在組合語言中先把它準備好。
2. **實作核心進入點 (`kmain`)：** 寫出我們作業系統的第一個 C 語言函式。
3. **實作基礎 VGA 驅動程式：** 在 C 語言中封裝字串列印功能。

---

### 步驟 1：補充編譯工具 (Dockerfile)

因為我們要編譯 32 位元的 C 程式碼 (`-m32` 參數)，在 64 位元的 Debian/Ubuntu 容器中，我們需要安裝 `gcc-multilib` 來提供 32 位元的標準標頭檔支援。

請修改 `day01/Dockerfile`（或者你可以把整個專案複製一份成 `day02` 目錄來做），在 `apt-get install` 區塊中加入 `gcc-multilib`：

```dockerfile
# ... 前面保留 ...
RUN apt-get update && apt-get install -y \
    nasm \
    build-essential \
    gcc-multilib \
    grub-pc-bin \
    grub-common \
    xorriso \
    && rm -rf /var/lib/apt/lists/*
# ... 後面保留 ...

```

*(修改後記得執行 `docker build -t os-builder .` 更新你的編譯環境)*

---

### 步驟 2：在組合語言中建立堆疊 (`boot.S`)

C 語言在執行時，如果沒有設定好 Stack Pointer (暫存器 `esp`)，只要一呼叫函式或宣告區域變數，系統就會崩潰。我們要在 `boot.S` 裡規劃一塊 `16KB` 的記憶體空間作為堆疊，並把控制權交給 C。

請將你的 `boot.S` 更新為以下內容（Multiboot 標頭保留不變，我們修改後面的部分）：

```assembly
section .multiboot
align 8
header_start:
    dd 0xe85250d6
    dd 0
    dd header_end - header_start
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))
    align 8
    dw 0
    dw 0
    dd 8
header_end:

; === 新增：定義未初始化的資料區段 (BSS) 來存放堆疊 ===
section .bss
align 16
stack_bottom:
    resb 16384 ; 保留 16 KB 的空間 (16 * 1024 bytes)
stack_top:

; === 修改：系統進入點 ===
section .text
extern kernel_main ; 告訴組譯器：我們等一下會呼叫一個在外面用 C 寫的函式

global _start
_start:
    ; 1. 設定堆疊指標 (Stack Pointer) 指向堆疊頂部
    ; x86 的堆疊是往下長的，所以要指向最高位址的 stack_top
    mov esp, stack_top

    ; 2. 呼叫 C 語言的進入點
    call kernel_main

    ; 3. 如果 C 函式意外 Return 了，關閉中斷並讓 CPU 休眠
    cli
.hang:
    hlt
    jmp .hang

```

---

### 步驟 3：撰寫核心進入點 (`kernel.c`)

現在，我們終於可以寫 C 語言了！這是一個不需要作業系統標準函式庫 (Freestanding) 的環境，所以我們不能 `#include <stdio.h>`，也沒有 `printf` 可以用，一切都要自己動手。

VGA 文字模式的記憶體從 `0xB8000` 開始。每一個字元佔據 2 bytes：

* **低位元組 (Lower byte):** ASCII 字元碼 (如 `'A'`)。
* **高位元組 (Higher byte):** 顏色屬性 (前景色與背景色)。

請建立一個新檔案 `kernel.c`：

```c
#include <stdint.h>
#include <stddef.h>

// 定義 VGA 記憶體起始位址
volatile uint16_t* vga_buffer = (uint16_t*)0xB8000;

// 定義螢幕的寬高
const size_t VGA_COLS = 80;
const size_t VGA_ROWS = 25;

// 函式：初始化終端機（清空螢幕）
void terminal_initialize(void) {
    for (size_t y = 0; y < VGA_ROWS; y++) {
        for (size_t x = 0; x < VGA_COLS; x++) {
            const size_t index = y * VGA_COLS + x;
            // 0x0F 代表黑底白字，' ' 是空白字元
            vga_buffer[index] = ((uint16_t)0x0F << 8) | ' ';
        }
    }
}

// 函式：在畫面上印出字串
void terminal_writestring(const char* data) {
    size_t index = 0;
    while (data[index] != '\0') {
        // 0x0A 代表黑底亮綠色
        vga_buffer[index] = ((uint16_t)0x0A << 8) | data[index];
        index++;
    }
}

// 這是我們作業系統的 C 語言主程式！
void kernel_main(void) {
    terminal_initialize();
    terminal_writestring("Hello, C Kernel World! I am running on x86!");
}

```

---

### 步驟 4：更新自動化腳本 (`Makefile`)

我們需要告訴編譯系統如何編譯 C 語言，並把它跟剛剛的組合語言連結在一起。我們必須加上 `-ffreestanding` 參數，嚴格告訴 GCC：「不要偷偷塞入任何作業系統相依的標準函式庫」。

請將 `Makefile` 更新如下：

```makefile
DOCKER_CMD = docker run --rm -v $(PWD):/workspace -w /workspace os-builder

# GCC 編譯參數：32位元、無標準函式庫、開啟最佳化與警告
CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra

all: build-env myos.iso

build-env:
	docker build -t os-builder .

myos.iso: myos.bin grub.cfg
	mkdir -p isodir/boot/grub
	cp myos.bin isodir/boot/myos.bin
	cp grub.cfg isodir/boot/grub/grub.cfg
	$(DOCKER_CMD) grub-mkrescue -o myos.iso isodir
	rm -rf isodir

# 將 kernel.o 也加入連結清單中
myos.bin: boot.o kernel.o
	$(DOCKER_CMD) ld -m elf_i386 -n -T linker.ld -o myos.bin boot.o kernel.o

kernel.o: kernel.c
	$(DOCKER_CMD) gcc $(CFLAGS) -c kernel.c -o kernel.o

boot.o: boot.S
	$(DOCKER_CMD) nasm -f elf32 boot.S -o boot.o

clean:
	rm -f *.o *.bin *.iso
	rm -rf isodir

```

---

### 執行與驗證

現在，請在終端機依序執行：

```bash
make clean
make
qemu-system-i386 -cdrom myos.iso

```

如果一切順利，QEMU 開機後，你會看到原本雜亂的 GRUB 殘留畫面被瞬間清空，然後在畫面左上角印出一行工整的綠色字串：
`Hello, C Kernel World! I am running on x86!`

成功看到這行字後，恭喜你，你的作業系統正式具備了高階語言的執行能力！接下來，我們就可以運用 C 語言的 Struct、指標和迴圈，開始構建更複雜的系統。

你可以先動手試試看，如果有遇到任何編譯錯誤或是執行問題，隨時把 Log 丟上來。如果成功了，你想挑戰讓這個 VGA 驅動支援「換行符號 (`\n`)」嗎？
