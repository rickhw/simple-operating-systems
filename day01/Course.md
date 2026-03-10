太棒了！準備好 Git repo 和每日資料夾結構是個非常好的開始，這完全展現了你作為資深工程師的專業素養。

今天我們將進入 **Day 1：環境建置與破殼而出 (First Boot)**。

由於你使用的是 M1 Max (ARM64 架構)，而我們要開發的是 x86_64 的作業系統，處理工具鏈 (Toolchain) 的相容性會是第一道關卡。我們不會在你的 macOS 上直接亂裝套件，而是會利用 Docker 建立一個乾淨、可拋棄的 x86_64 交叉編譯環境，並透過 QEMU 來模擬測試。

---

### Day 1 授課目標

1. **建立建置環境**：設定 Docker 容器化編譯環境，確保能產出 x86 格式的二進位檔 (ELF)。
2. **理解開機流程**：了解 BIOS -> Bootloader -> Kernel 的轉交過程。
3. **撰寫 Multiboot2 標頭**：讓標準的 Bootloader (如 GRUB) 認得我們的作業系統。
4. **第一次開機**：寫一小段組合語言，在螢幕上印出字元，並讓 CPU 進入安全停止狀態。

---

### 理論補充：開機的最初幾秒鐘

當電腦通電時，CPU 會處於 16 位元的「真實模式」(Real Mode)。BIOS 會進行硬體自檢 (POST)，然後尋找啟動裝置，將該裝置的第一個磁區 (Boot Sector) 載入記憶體並執行。

在過去，開發者必須自己寫一段極為繁瑣的 16 位元組合語言，將 CPU 切換到 32 位元保護模式 (Protected Mode)，再載入核心。現在，我們站在巨人的肩膀上，使用 **Multiboot2 規範**。我們只需在程式碼開頭放上一段特定的「魔法數字 (Magic Number)」，像 GRUB 這樣的 Bootloader 就會幫我們處理好一切硬體初始化的髒活，直接把我們帶入 32 位元環境。

---

### 實作步驟

請在你的 Git repo 中建立 `day01` 資料夾，我們今天所有的檔案都會放在這裡。

#### 1. 建立編譯環境 (Dockerfile)

我們需要一個包含 `nasm` (組譯器)、`gcc` (編譯器) 和 `ld` (連結器) 的環境。
請建立 `day01/Dockerfile`：

```dockerfile
FROM debian:bullseye

RUN apt-get update && apt-get install -y \
    nasm \
    build-essential \
    grub-pc-bin \
    grub-common \
    xorriso \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

```

#### 2. 撰寫系統進入點 (boot.S)

這是我們 OS 的第一行程式碼，負責定義 Multiboot2 標頭，並在螢幕上印出兩個綠色的字母「OK」。
請建立 `day01/boot.S`：

```assembly
section .multiboot
align 8
header_start:
    dd 0xe85250d6                ; Multiboot2 魔法數字
    dd 0                         ; 架構 0 (代表 i386)
    dd header_end - header_start ; 標頭總長度
    ; 檢查碼 (Checksum) = -(魔法數字 + 架構 + 長度)
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start)) 
header_end:

section .text
global _start
_start:
    ; VGA 文字模式的記憶體起始位置是 0xb8000
    ; 每個字元佔用 2 bytes：前景色/背景色 (1 byte) + ASCII 碼 (1 byte)
    ; 0x2f 代表綠底白字 (或類似高對比顏色)，0x4f 是 'O'，0x4b 是 'K'
    mov dword [0xb8000], 0x2f4b2f4f 
    
    ; 關閉硬體中斷
    cli 

.hang:
    ; 讓 CPU 進入休眠狀態 (Halt)，如果被喚醒則跳回 .hang 繼續休眠
    hlt
    jmp .hang

```

#### 3. 記憶體佈局設定 (linker.ld)

編譯後的程式碼需要告訴系統該被放在記憶體的哪裡。作業系統核心通常會被載入到實體記憶體 `1MB` (0x100000) 的位置。
請建立 `day01/linker.ld`：

```ld
ENTRY(_start)

SECTIONS {
    /* 核心載入起始位址：1MB */
    . = 1M;

    /* 將 multiboot 標頭放在最前面，這非常重要，否則 Bootloader 找不到 */
    .text BLOCK(4K) : ALIGN(4K) {
        *(.multiboot)
        *(.text)
    }

    /* 唯讀資料區段 */
    .rodata BLOCK(4K) : ALIGN(4K) {
        *(.rodata)
    }

    /* 可讀寫資料區段 */
    .data BLOCK(4K) : ALIGN(4K) {
        *(.data)
    }

    /* 未初始化資料區段 */
    .bss BLOCK(4K) : ALIGN(4K) {
        *(COMMON)
        *(.bss)
    }
}

```

#### 4. 自動化建置腳本 (Makefile)

把編譯的繁瑣指令寫進 Makefile。它會使用 Docker 來執行編譯。
請建立 `day01/Makefile`：

```makefile
# 避免 M1 架構產生不相容的 object 檔，我們透過 docker 執行
DOCKER_CMD = docker run --rm -v $(PWD):/workspace -w /workspace os-builder

all: build-env myos.bin

build-env:
	docker build -t os-builder .

myos.bin: boot.o
	$(DOCKER_CMD) ld -n -T linker.ld -o myos.bin boot.o

boot.o: boot.S
	$(DOCKER_CMD) nasm -f elf32 boot.S -o boot.o

clean:
	rm -f *.o *.bin

```

---

### 執行與驗證

請打開終端機，確保你在 `day01` 目錄下。

1. **編譯你的核心**：
執行 `make`。這會先建置 Docker image，然後組譯 `boot.S`，最後連結成 `myos.bin`。
2. **安裝 QEMU** (如果你還沒裝)：
透過 Homebrew 安裝支援 x86 模擬的 QEMU：`brew install qemu`。
3. **啟動你的作業系統**：
執行以下指令，讓 QEMU 直接載入你的核心二進位檔：
```bash
qemu-system-i386 -kernel myos.bin

```



**預期結果：**
QEMU 視窗會彈出，螢幕左上角應該會出現亮色的 **"OK"** 兩個字，然後畫面停止不動（因為我們執行了 `hlt`）。

這看起來很簡單，但這代表你已經成功跨越了韌體層，完全接管了硬體的控制權！

你準備好動手建立這四個檔案並執行看看了嗎？如果有遇到任何編譯或 QEMU 的錯誤訊息，隨時把錯誤 log 貼上來。
