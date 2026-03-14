// gdt.h
#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// GDT 單一條目的結構
struct gdt_entry_struct {
    uint16_t limit_low;   // 區塊長度 (下半部)
    uint16_t base_low;    // 區塊起始位址 (下半部)
    uint8_t  base_middle; // 區塊起始位址 (中間)
    uint8_t  access;      // 存取權限 (Ring 0/3, 程式碼/資料)
    uint8_t  granularity; // 顆粒度與長度 (上半部)
    uint8_t  base_high;   // 區塊起始位址 (上半部)
} __attribute__((packed));
typedef struct gdt_entry_struct gdt_entry_t;

// 指向 GDT 陣列的指標結構 (給 lgdt 指令用的)
struct gdt_ptr_struct {
    uint16_t limit;       // GDT 陣列的總長度 - 1
    uint32_t base;        // GDT 陣列的起始位址
} __attribute__((packed));
typedef struct gdt_ptr_struct gdt_ptr_t;

// 在 gdt.h 中新增 TSS 結構
struct tss_entry_struct {
    uint32_t prev_tss;   // 舊的 TSS (不用理它)
    uint32_t esp0;       // [極重要] Ring 0 的堆疊指標
    uint32_t ss0;        // [極重要] Ring 0 的堆疊區段 (通常是 0x10)
    // ... 中間還有非常多暫存器狀態，為了簡化，我們宣告成一個大陣列填滿空間
    uint32_t unused[23];
} __attribute__((packed));
typedef struct tss_entry_struct tss_entry_t;


// 初始化 GDT 的公開函式
void init_gdt(void);

void set_kernel_stack(uint32_t stack);

#endif
