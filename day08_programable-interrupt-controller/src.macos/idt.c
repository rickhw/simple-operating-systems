// idt.c
#include "idt.h"
#include "io.h"

extern void kprintf(const char* format, ...);

// 宣告一個長度為 256 的 IDT 陣列
idt_entry_t idt_entries[256];
idt_ptr_t   idt_ptr;

// 外部組合語言函式：載入 IDT 與中斷處理入口
extern void idt_flush(uint32_t);
extern void isr0(); // 第 0 號中斷的 Assembly 進入點

// 設定單一 IDT 條目的輔助函式
static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].base_lo = base & 0xFFFF;
    idt_entries[num].base_hi = (base >> 16) & 0xFFFF;
    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    // flags 0x8E 代表：這是一個 32-bit 的中斷閘 (Interrupt Gate)，運行在 Ring 0，且此條目有效(Present)
    idt_entries[num].flags   = flags;
}

// 這是當「除以零」發生時，實際會執行的 C 語言函式
void isr0_handler(void) {
    kprintf("\n[KERNEL PANIC] Exception 0: Divide by Zero!\n");
    kprintf("System Halted.\n");
    // 發生嚴重錯誤，直接把系統凍結
    __asm__ volatile ("cli; hlt");
}

// 初始化 8259 PIC，將 IRQ 0~15 重映射到 IDT 的 32~47
void pic_remap() {
    // 儲存原本的遮罩 (Masks)
    uint8_t a1 = inb(0x21);
    uint8_t a2 = inb(0xA1);
    (void)a1; // 消除 unused variable 警告
    (void)a2; // 消除 unused variable 警告

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


extern void isr33(); // 宣告組合語言的鍵盤跳板

void init_idt(void) {
    idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
    idt_ptr.base  = (uint32_t)&idt_entries;

    // 先把 256 個中斷全部清空 (避免指到未知的記憶體)
    // 這裡我們簡單用迴圈清零 (你也可以 include 昨天寫的 memset)
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    // 掛載第 0 號中斷：除以零
    // 0x08 是我們昨天在 GDT 設定的 Kernel Code Segment
    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);

    // [新增] 重新映射 PIC
    pic_remap();

    // [新增] 掛載第 33 號中斷 (IRQ1 鍵盤)
    idt_set_gate(33, (uint32_t)isr33, 0x08, 0x8E);

    // 呼叫組合語言，正式套用新的 IDT
    idt_flush((uint32_t)&idt_ptr);
}
