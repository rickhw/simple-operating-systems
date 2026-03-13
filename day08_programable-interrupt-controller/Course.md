歡迎來到 **Day 8：賦予系統知覺 — 可程式化中斷控制器 (PIC) 與鍵盤輸入**！

前面我們已經把系統的「緊急聯絡簿 (IDT)」建好了，也成功攔截了 CPU 內部的除以零錯誤。今天我們要來處理**外部硬體**的訊號。

在 x86 架構中，鍵盤、滑鼠、時鐘等硬體並不會直接把中斷訊號塞給 CPU，而是統一交給一位「秘書」來排隊跟過濾，這位秘書就是 **8259 可程式化中斷控制器 (PIC, Programmable Interrupt Controller)**。

---

### Day 8 授課目標

1. **實作硬體讀取指令 (`inb`)：** 讓 CPU 能從硬體 Port 讀取資料。
2. **重映射 (Remap) PIC：** 解決早期 IBM PC 架構留下來的歷史業障。
3. **實作鍵盤中斷處理 (IRQ 1)：** 當按下鍵盤時，讀取掃描碼 (Scan Code) 並印在畫面上。
4. **開啟全域中斷 (`sti`)：** 讓系統正式開始接聽外部來電。

---

### 理論補充：PIC 的歷史業障

在原始的設計中，主機板上有兩顆 PIC 晶片（Master 與 Slave），掌管了 16 個硬體中斷 (IRQ 0 ~ 15)。

* **IRQ 0：** 系統計時器 (Timer)
* **IRQ 1：** 鍵盤 (Keyboard)
* **IRQ 2：** 連接 Slave PIC 的橋樑

**最大問題來了：** BIOS 預設把這 16 個硬體中斷，對應到 CPU 的第 8 ~ 23 號中斷。
但你還記得昨天我們說過，CPU 的 0 ~ 31 號中斷是保留給「內部例外錯誤 (Exceptions)」使用的嗎？（例如 8 號是 Double Fault）。如果我們不改設定，當你按下一鍵盤 (IRQ 1)，CPU 會以為發生了第 9 號嚴重錯誤！

所以，我們今天的第一件事，就是寫信給 PIC 秘書：**「請把你管理的硬體中斷，全部往後挪到第 32 號 (0x20) 開始！」** 這就叫做 PIC Remapping。

---

### 實作步驟

#### 1. 擴充硬體通訊能力 (`kernel.c`)

打開 `kernel.c`，在昨天寫的 `outb` 函式下面，加入用來「讀取」硬體資料的 `inb` 函式：

```c
// inb: 從指定的硬體 Port 讀取 1 byte 的資料
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

```

#### 2. PIC 重映射與鍵盤處理 (`idt.c`)

為了減少檔案切換，我們把 PIC 的初始化邏輯直接放在 `idt.c` 裡面。請打開 `idt.c` 並做以下修改：

**加入 `outb` 的宣告（因為我們要對 PIC 下指令）：**

```c
// 在 idt.c 最上方加入
extern void outb(uint16_t port, uint8_t val);
extern uint8_t inb(uint16_t port);
extern void kprintf(const char* format, ...);

```

**加入 PIC 重映射函式：**

```c
// 初始化 8259 PIC，將 IRQ 0~15 重映射到 IDT 的 32~47
void pic_remap() {
    // 儲存原本的遮罩 (Masks)
    uint8_t a1 = inb(0x21); 
    uint8_t a2 = inb(0xA1);

    // 開始初始化序列 (ICW1)
    outb(0x20, 0x11);
    outb(0xA0, 0x11);

    // ICW2: 設定 Master PIC 的偏移量為 0x20 (十進位 32)
    outb(0x21, 0x20);
    // ICW2: 設定 Slave PIC 的偏移量為 0x28 (十進位 40)
    outb(0xA1, 0x28);

    // ICW3: 告訴 Master PIC 有一個 Slave 接在 IRQ2
    outb(0x21, 0x04);
    // ICW3: 告訴 Slave PIC 它的串聯身份
    outb(0xA1, 0x02);

    // ICW4: 設定為 8086 模式
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    // [關鍵設定] 遮罩設定：0 代表開啟，1 代表屏蔽
    // 我們目前只開啟 IRQ1 (鍵盤)，關閉其他所有硬體中斷 (避免 Timer 狂噴中斷干擾我們)
    // 0xFD 的二進位是 1111 1101 (第 1 個 bit 是 0，代表開啟鍵盤)
    outb(0x21, 0xFD);
    outb(0xA1, 0xFF);
}

```

**新增鍵盤的 C 語言處理函式 (ISR 33)：**

```c
// 鍵盤按下時會觸發這個函式
void keyboard_handler() {
    // 從 Port 0x60 讀取鍵盤掃描碼 (Scan Code)
    uint8_t scancode = inb(0x60);

    // 最高位元 (第7位) 如果是 1，代表按鍵被「放開 (Release)」
    // 如果是 0，代表按鍵被「按下 (Press)」
    if (!(scancode & 0x80)) {
        // 為了驗證，我們先簡單印出十六進位的掃描碼
        kprintf("Key Pressed! Scancode: 0x%x\n", scancode);
    }

    // [非常重要] 告訴 PIC 秘書：「這個中斷我處理完了，你可以送下一個來了」
    // 也就是發送 EOI (End of Interrupt) 訊號給 Master PIC (Port 0x20)
    outb(0x20, 0x20);
}

```

**修改 `init_idt` 函式：**
把我們剛剛寫好的 PIC 初始化加進去，並把第 33 號中斷（32 + 1 鍵盤）綁定上去。

```c
extern void isr33(); // 宣告組合語言的鍵盤跳板

void init_idt(void) {
    idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
    idt_ptr.base  = (uint32_t)&idt_entries;

    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);
    
    // [新增] 重新映射 PIC
    pic_remap();
    
    // [新增] 掛載第 33 號中斷 (IRQ1 鍵盤)
    idt_set_gate(33, (uint32_t)isr33, 0x08, 0x8E);

    idt_flush((uint32_t)&idt_ptr);
}

```

#### 3. 新增鍵盤的組合語言跳板 (`interrupts.S`)

打開 `interrupts.S`，在最下方補上給鍵盤用的 33 號中斷跳板：

```assembly
global isr33
extern keyboard_handler

; 第 33 號中斷 (鍵盤)
isr33:
    ; 備份所有通用暫存器 (這是一個好習慣，避免中斷處理程式弄髒了原本正在執行的程式的狀態)
    pusha 
    
    call keyboard_handler
    
    ; 恢復所有通用暫存器
    popa
    
    iret ; 中斷返回

```

#### 4. 啟動系統神經網路 (`kernel.c`)

一切就緒！回到 `kernel.c` 的 `kernel_main` 中，移除昨天除以零的測試碼，並在最後加上開啟中斷的指令：

```c
void kernel_main(void) {
    terminal_initialize();
    kprintf("=== OS Kernel Booting ===\n");
    
    init_gdt();
    kprintf("GDT loaded successfully.\n");

    init_idt();
    kprintf("IDT and PIC loaded successfully.\n");

    // [關鍵] 執行 sti (Set Interrupt Flag) 開啟全域中斷
    // 從這行開始，CPU 會開始接聽外部硬體的呼叫！
    __asm__ volatile ("sti");

    kprintf("Interrupts Enabled. Waiting for keyboard input...\n");

    // 讓核心進入無限休眠迴圈，有中斷來才會醒來做事
    while (1) {
        __asm__ volatile ("hlt");
    }
}

```

---

### 執行與驗證

一樣的三部曲：

```bash
make clean
make
qemu-system-i386 -cdrom myos.iso

```

**預期結果：**
畫面會停在 `Waiting for keyboard input...`。
這時候，請確認滑鼠焦點在 QEMU 視窗內，然後隨便按鍵盤上的按鍵（例如按 `A`, `S`, `D`, `Space`）。
你應該會看到畫面上每按一個鍵，就會印出一行 `Key Pressed! Scancode: 0x??`。
（例如：按下 `A` 通常會印出 `0x1e`，按下 `S` 會印出 `0x1f`）。

---

### 🤔 Day 8 思考題

如果你順利看到了掃描碼印出來，恭喜你，你的系統活過來了！這是一個具有劃時代意義的里程碑，你真正打通了軟體與硬體之間的橋樑。

看著印出來的十六進位掃描碼，請思考這個問題：
**目前的鍵盤驅動只能抓到「掃描碼 (Scancode)」，也就是硬體上的鍵盤座標。如果要把它變成我們人類看得懂的 ASCII 字母 (比如按 A 印出 'A')，我們在程式碼裡該怎麼做？進一步來說，如果使用者同時按住了 `Shift` 鍵再按 `A`，我們又該怎麼把 'a' 變成大寫的 'A' 呢？**

試著敲打幾個按鍵，感受一下這個由你自己一手打造的系統神經迴路。有任何問題或想法，隨時提出來！
