// IDT: Interrupt Descriptor Table
// ISR: Interrupt Service Routine
#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// IDT 單一條目的結構
struct idt_entry_struct {
    uint16_t base_lo; // 中斷處理程式 (ISR) 位址的下半部
    uint16_t sel;     // GDT 裡的 Kernel Code Segment 選擇子 (通常是 0x08)
    uint8_t  always0; // 必須為 0
    uint8_t  flags;   // 屬性與權限標籤
    uint16_t base_hi; // 中斷處理程式 (ISR) 位址的上半部
} __attribute__((packed));
typedef struct idt_entry_struct idt_entry_t;

// 指向 IDT 陣列的指標結構 (給 lidt 指令用的)
struct idt_ptr_struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));
typedef struct idt_ptr_struct idt_ptr_t;

void init_idt(void);

#endif
