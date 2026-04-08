/**
 * [idt.c] - 中斷描述符表 (Interrupt Descriptor Table) 實作
 * * 核心功能：
 * 1. 定義 IDT 結構並填充 256 個中斷閘 (Gates)。
 * 2. 重新映射可程式化中斷控制器 (PIC 8259)，避免硬體中斷與 CPU 異常衝突。
 * 3. 實作特定的中斷處理邏輯 (如 Divide by Zero)。
 */

#include "idt.h"
#include "io.h"

// 外部 C 函式宣告
extern void kprintf(const char* format, ...);

// 核心資料結構
idt_entry_t idt_entries[256]; // 存放 256 個中斷條目
idt_ptr_t   idt_ptr;          // 傳遞給 LIDT 指令的指標結構

// 宣告位於 interrupts.S 的彙編進入點 (ISRs)
extern void idt_flush(uint32_t); // 執行 lidt 指令
extern void isr0();              // 0: Divide by Zero
extern void isr14();             // 14: Page Fault
extern void isr32();             // 32: Timer (IRQ0)
extern void isr33();             // 33: Keyboard (IRQ1)
extern void isr43();             // 43: Network (IRQ11, RTL8139)
extern void isr44();             // 44: Mouse (IRQ12)
extern void isr128();            // 128: System Call (0x80)

/**
 * 設定 IDT 單一條目 (Gate)
 * @param num   中斷編號 (0-255)
 * @param base  中斷處理常式的偏移位址
 * @param sel   段選擇子 (通常指向 Kernel Code Segment 0x08)
 * @param flags 屬性位元 (包含 DPL 權限與存在位)
 */
static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].base_lo = base & 0xFFFF;         // 低 16 位元
    idt_entries[num].base_hi = (base >> 16) & 0xFFFF;  // 高 16 位元
    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    idt_entries[num].flags   = flags;
    /* Flags 常見值:
       0x8E: 10001110b (Present, Ring 0, Interrupt Gate)
       0xEE: 11101110b (Present, Ring 3, Interrupt Gate) -> 用於 Syscall
    */
}

/**
 * 初始化 IDT：清空、掛載 ISRs、重映射 PIC、正式啟用
 */
void init_idt(void) {
    idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
    idt_ptr.base  = (uint32_t)&idt_entries;

    // 1. 初始化全數條目為空
    for (uint32_t i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    // 2. 掛載 CPU 異常 (Exceptions)
    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);   // 除以零
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E); // 分頁錯誤

    // 3. 重新映射 8259 PIC (硬體中斷 IRQ 0-15 映射至 IDT 32-47)
    pic_remap();

    // 4. 掛載硬體中斷 (IRQs)
    idt_set_gate(32, (uint32_t)isr32, 0x08, 0x8E); // Timer
    idt_set_gate(33, (uint32_t)isr33, 0x08, 0x8E); // Keyboard
    idt_set_gate(43, (uint32_t)isr43, 0x08, 0x8E); // Network
    idt_set_gate(44, (uint32_t)isr44, 0x08, 0x8E); // Mouse

    // 5. 掛載系統呼叫 (System Call)
    // 必須使用 0xEE (DPL=3)，否則 User Mode 執行 int 0x80 會觸發 General Protection Fault
    idt_set_gate(128, (uint32_t)isr128, 0x08, 0xEE);

    // 6. 將 IDT 指標載入 CPU
    idt_flush((uint32_t)&idt_ptr);
}

/**
 * [PIC 重映射]
 * 預設 BIOS 將 IRQ 0-7 映射到 IDT 8-15，這與 CPU 內建異常衝突。
 * 故將其改為 IRQ 0-7 -> IDT 32-39, IRQ 8-15 -> IDT 40-47。
 */
void pic_remap(void) {
    // ICW1: 開始初始化
    outb(0x20, 0x11);
    outb(0xA0, 0x11);

    // ICW2: 設定向量偏移 (Vector Offset)
    outb(0x21, 0x20); // Master -> 0x20 (32)
    outb(0xA1, 0x28); // Slave  -> 0x28 (40)

    // ICW3: 級聯 (Cascading)
    outb(0x21, 0x04); // Master 的 IRQ2 接 Slave
    outb(0xA1, 0x02); // Slave 接在 Master 的第 2 號

    // ICW4: 環境設定
    outb(0x21, 0x01); // 8086 模式
    outb(0xA1, 0x01);

    // [中斷遮罩設定] 0 為開啟，1 為屏蔽
    // Master: 開啟 IRQ0(Timer), IRQ1(KB), IRQ2(Slave) -> 11111000 (0xF8)
    outb(0x21, 0xF8);
    // Slave: 開啟 IRQ11(Net), IRQ12(Mouse) -> 11100111 (0xE7)
    outb(0xA1, 0xE7);
}

// 範例異常處理函式
void isr0_handler(void) {
    kprintf("\n[KERNEL PANIC] Exception 0: Divide by Zero!\n");
    __asm__ volatile ("cli; hlt");
}
