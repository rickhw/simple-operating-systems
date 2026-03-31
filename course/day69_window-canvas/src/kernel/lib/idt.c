// IDT: Interrupt Descriptor Table
// Intel 8259: https://zh.wikipedia.org/zh-tw/Intel_8259
// PIC: [Programmable Interrupt Controller](https://en.wikipedia.org/wiki/Programmable_interrupt_controller)
#include "idt.h"
#include "io.h"

extern void kprintf(const char* format, ...);

// 宣告一個長度為 256 的 IDT 陣列
idt_entry_t idt_entries[256];
idt_ptr_t   idt_ptr;

// 外部組合語言函式：載入 IDT 與中斷處理入口
extern void idt_flush(uint32_t);
extern void isr0();     // 第 0  號中斷：的 Assembly 進入點
extern void isr32();    // 第 32 號中斷：Timer IRQ 0
extern void isr33();    // 第 33 號中斷：宣告組合語言的鍵盤跳板
extern void isr44();    // 第 44 號中斷：Mouse Handler
extern void isr128();   // 第 128 號中斷：system calls

// 設定單一 IDT 條目的輔助函式
static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].base_lo = base & 0xFFFF;
    idt_entries[num].base_hi = (base >> 16) & 0xFFFF;
    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    // flags 0x8E 代表：這是一個 32-bit 的中斷閘 (Interrupt Gate)，運行在 Ring 0，且此條目有效(Present)
    idt_entries[num].flags   = flags;
}


// --- 公開 API ---

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

    // 重新映射 PIC
    pic_remap();

    // 掛載第 32 號中斷 (IRQ0: Timer)
    idt_set_gate(32, (uint32_t)isr32, 0x08, 0x8E);

    // 掛載第 33 號中斷 (IRQ1: Keyboard)
    idt_set_gate(33, (uint32_t)isr33, 0x08, 0x8E);

    // 掛載第 44 號中斷 (IRQ1: Mouse)
    idt_set_gate(44, (uint32_t)isr44, 0x08, 0x8E);

    // 掛載第 128 號中斷 (System Call)
    // 注意！旗標是 0xEE (允許 Ring 3 呼叫)
    idt_set_gate(128, (uint32_t)isr128, 0x08, 0xEE);

    // 呼叫組合語言，正式套用新的 IDT
    idt_flush((uint32_t)&idt_ptr);
}


// ---

// 這是當「除以零」發生時，實際會執行的 C 語言函式
void isr0_handler(void) {
    kprintf("\n[KERNEL PANIC] Exception 0: Divide by Zero!\n");
    kprintf("System Halted.\n");
    // 發生嚴重錯誤，直接把系統凍結
    __asm__ volatile ("cli; hlt");
}

// 初始化 8259 PIC，將 IRQ 0~15 重映射到 IDT 的 32~47
// Intel 8259: https://zh.wikipedia.org/zh-tw/Intel_8259
void pic_remap() {
    uint8_t a1 = inb(0x21);
    uint8_t a2 = inb(0xA1);
    (void)a1; (void)a2;

    // ICW1: 開始初始化序列
    outb(0x20, 0x11);
    outb(0xA0, 0x11);

    // ICW2: 設定 Master PIC 偏移量為 0x20 (32)
    outb(0x21, 0x20);
    // ICW2: 設定 Slave PIC 偏移量為 0x28 (40)
    outb(0xA1, 0x28);

    // ICW3: Master 告訴 Slave 接在 IRQ2
    outb(0x21, 0x04);
    // ICW3: Slave 確認身分
    outb(0xA1, 0x02);

    // ICW4: 設定為 8086 模式
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    // ==========================================================
    // 遮罩 (Masks)：0 代表開啟，1 代表屏蔽
    // 為了讓 IRQ 12 (滑鼠) 通過，我們必須：
    // 1. 開啟 Master PIC 的 IRQ 0(Timer), 1(Keyboard), 2(Slave連線) -> 1111 1000 = 0xF8
    // 2. 開啟 Slave PIC 的 IRQ 12 (第 4 個 bit) -> 1110 1111 = 0xEF
    // ==========================================================
    outb(0x21, 0xF8);
    outb(0xA1, 0xEF);
}
