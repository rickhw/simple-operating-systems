// GDT: Global Descriptor Table
// TSS: Task State Segment
#include <stdint.h>
#include "gdt.h"
#include "utils.h"

gdt_entry_t gdt_entries[6]; // 6 個元素：Null, KCode, KData, UCode, UData, TSS
gdt_ptr_t   gdt_ptr;
tss_entry_t tss_entry;

extern void gdt_flush(uint32_t);    // 外部組合語言函式，用來載入 GDT (Global Descriptor Table)
extern void tss_flush(void);        // 外部組合語言函式，用來載入 TSS (Task State Segment)

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

// 初始化 TSS (Task State Segment) 的輔助函式
static void write_tss(int32_t num, uint16_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t) &tss_entry;
    uint32_t limit = sizeof(tss_entry_t);

    // 把 TSS 當作一個特殊的 GDT 區段加進去 (Access: 0xE9 代表 TSS)
    gdt_set_gate(num, base, limit, 0xE9, 0x00);

    memset(&tss_entry, 0, sizeof(tss_entry_t));
    tss_entry.ss0  = ss0;   // 設定 Kernel Data Segment (0x10)
    tss_entry.esp0 = esp0;  // 設定當前 Kernel 的 Stack 頂端
}

// --- 公開 API ---

void init_gdt(void) {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 6) - 1;
    gdt_ptr.base  = (uint32_t)&gdt_entries;

    // 第一個必須是 Null
    gdt_set_gate(0, 0, 0, 0, 0);

    // 第二個是 Code Segment (位址 0~4GB)
    // Access 0x9A: Ring 0, 可執行, 可讀取
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    // 第三個是 Data Segment (位址 0~4GB)
    // Access 0x92: Ring 0, 可讀寫
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    // User Code Segment (位址 0~4GB)
    // Access 0xFA: Ring 3 (DPL=3), 可執行, 可讀取
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);

    // User Data Segment (位址 0~4GB)
    // Access 0xF2: Ring 3 (DPL=3), 可讀寫
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    // 初始化 TSS (掛在第 5 個位置，也就是 0x28)
    // 這裡的 0x10 是 Kernel Data Segment。0x0 暫時填 0，晚點會被動態更新
    write_tss(5, 0x10, 0x0);

    // 呼叫組合語言，正式套用新的 GDT
    gdt_flush((uint32_t)&gdt_ptr);
    tss_flush(); // 告訴 CPU：「逃生路線圖在這裡！」
}

// 在 lib/gdt.c 檔案的下方新增這個函式：
void set_kernel_stack(uint32_t esp) {
    // 0x10 代表 Kernel Data Segment (你的 GDT 設定中，Data 段的 offset)
    tss_entry.ss0 = 0x10;
    tss_entry.esp0 = esp;
}
