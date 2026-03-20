// gdt.c
#include "gdt.h"

// 宣告一個長度為 3 的 GDT 陣列
gdt_entry_t gdt_entries[3];
gdt_ptr_t   gdt_ptr;

// 外部組合語言函式，用來載入 GDT
extern void gdt_flush(uint32_t);

// 設定單一 GDT 條目的輔助函式
static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;

    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access      = access;
}

void init_gdt(void) {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 3) - 1;
    gdt_ptr.base  = (uint32_t)&gdt_entries;

    // 第一個必須是 Null
    gdt_set_gate(0, 0, 0, 0, 0);

    // 第二個是 Code Segment (位址 0~4GB)
    // Access 0x9A: Ring 0, 可執行, 可讀取
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    // 第三個是 Data Segment (位址 0~4GB)
    // Access 0x92: Ring 0, 可讀寫
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    // 呼叫組合語言，正式套用新的 GDT
    gdt_flush((uint32_t)&gdt_ptr);
}
