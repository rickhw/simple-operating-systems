
跑起來後沒有出現 `[Net] Packet RX` 的訊息，我看了一下，現在的 code 似乎沒有觸發 rtl8139_handler() 的點，沒有頭緒。

底下是這次相關修改的 code:


---
interrupts.S

```asm
; [目的] 中斷服務例程 (ISR) 總匯與 IDT 載入
; [流程 - 以 System Call 0x128 為例]
; User App 執行 int 0x128 (eax=功能號)
;    |
;    V
; [ isr128 ]
;    |--> [ pusha ] (將 eax, ebx... 等暫存器快照壓入堆疊)
;    |--> [ push esp ] (將指向 registers_t 的指標傳給 C 函式)
;    |--> [ call syscall_handler ]
;    |      |--> C 語言修改 regs->eax 作為回傳值
;    |--> [ add esp, 4 ] (清理參數)
;    |--> [ popa ] (此時 eax 已變更為回傳值)
;    |--> [ iret ] (返回 User Mode)

global idt_flush

global isr0
extern isr0_handler

; [day70] Page Fault
global isr14
extern page_fault_handler

global isr32
extern timer_handler

global isr33
extern keyboard_handler

global isr43
extern rtl8139_handler

global isr44
extern mouse_handler

global isr128
extern syscall_handler

idt_flush:
    mov eax, [esp+4]
    lidt [eax]
    ret

isr0:
    cli
    push 0          ; [Day70] add 動推入假錯誤碼
    call isr0_handler
    add esp, 4      ; [day70] add 清除假錯誤碼
    sti
    iret

; 第 14 號中斷：Page Fault
isr14:
    ; 💡 注意：Page Fault 時，CPU「已經」自動 push error code 了！所以這裡不需要 push 0
    pusha           ; 壓入 eax, ecx, edx, ebx, esp, ebp, esi, edi
    push esp        ; 傳遞 registers_t 指標
    call page_fault_handler
    add esp, 4      ; 清除 registers_t 指標參數
    popa            ; 恢復通用暫存器
    add esp, 4      ; 【重要】丟棄 CPU 壓入的 Error Code！
    iret

isr32:
    push 0          ; 【Day 70 修復】手動推入假錯誤碼
    pusha
    call timer_handler
    popa
    add esp, 4      ; 【Day 70 修復】清除假錯誤碼
    iret

isr33:
    push 0          ; 【Day 70 修復】
    pusha
    call keyboard_handler
    popa
    add esp, 4      ; 【Day 70 修復】
    iret

isr43:
    push 0          ; 【Day 70 修復】
    pusha
    call rtl8139_handler
    popa
    add esp, 4      ; 【Day 70 修復】
    iret

isr44:
    push 0          ; 【Day 70 修復】
    pusha
    call mouse_handler
    popa
    add esp, 4      ; 【Day 70 修復】
    iret

; 第 128 號中斷 (System Call 窗口)
isr128:
    push 0          ; 【Day 70 修復】手動推入假錯誤碼，對齊 registers_t！
    pusha           ; 備份通用暫存器 (32 bytes)
    push esp        ; 把 registers_t 的指標傳給 C 語言
    call syscall_handler
    add esp, 4      ; 清除 esp 參數
    popa            ; 恢復所有暫存器
    add esp, 4      ; 【Day 70 修復】清除假錯誤碼
    iret
```

---
idt.c
```c
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
extern void isr14();    // 第 14 號中斷：Page Fault
extern void isr32();    // 第 32 號中斷：Timer IRQ 0
extern void isr33();    // 第 33 號中斷：宣告組合語言的鍵盤跳板
extern void isr43();    // 第 43 號中斷：rtl8139_handler
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
    for (uint32_t i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    // 掛載第 0 號中斷：除以零
    // 0x08 是我們昨天在 GDT 設定的 Kernel Code Segment
    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);

    // 【Day 70 新增】掛載第 14 號中斷：Page Fault
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);


    // 重新映射 PIC
    pic_remap();

    // 掛載第 32 號中斷 (IRQ0: Timer)
    idt_set_gate(32, (uint32_t)isr32, 0x08, 0x8E);

    // 掛載第 33 號中斷 (IRQ1: Keyboard)
    idt_set_gate(33, (uint32_t)isr33, 0x08, 0x8E);

    // 掛載第 43 號中斷 (IRQ11: RTL8139)
    idt_set_gate(43, (uint32_t)isr43, 0x08, 0x8E);

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
void pic_remap(void) {
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
```

---

rtl8139.c
```c
#include "io.h"
#include "rtl8139.h"
#include "pci.h"
#include "tty.h"
#include "ethernet.h"

// 儲存網卡的 I/O 基準位址與 MAC 位址
static uint32_t rtl_iobase = 0;
static uint8_t mac_addr[6];

// ==========================================
// 【Day 83 新增】接收環狀緩衝區 (Rx Ring Buffer)
// 網卡要求最少 8K (8192) bytes，外加 16 bytes 的檔頭與 1500 bytes 容錯
// __attribute__((aligned(4096))) 確保它在實體記憶體上是 4K 對齊的
// ==========================================
static uint8_t rx_buffer[8192 + 16 + 1500] __attribute__((aligned(4096)));

// [day84] 用來追蹤目前讀取到緩衝區的哪個位置
static uint16_t rx_offset = 0;

void init_rtl8139(uint8_t bus, uint8_t slot) {
    kprintf("[RTL8139] Initializing driver...\n");

    // 1. 取得 I/O Base Address
    // 網卡的 Base Address Register 0 (BAR0) 存在 PCI 設定空間的 Offset 0x10
    // 因為 BAR0 是 32-bit，我們用兩次 16-bit 讀取來組合它
    uint32_t bar0_low = pci_read_config_word(bus, slot, 0, 0x10);
    uint32_t bar0_high = pci_read_config_word(bus, slot, 0, 0x12);

    // 組合出 32-bit 的位址，並清除最低 2 個 bit (PCI 規範，用來標示這是一塊 I/O 空間)
    rtl_iobase = (bar0_low | (bar0_high << 16)) & ~3;
    kprintf("[RTL8139] I/O Base Address found at: [%x]\n", rtl_iobase);

    // 2. 開機 (Power On)
    // RTL8139 的 CONFIG1 暫存器 (Offset 0x52)，寫入 0x00 即可喚醒
    outb(rtl_iobase + 0x52, 0x00);

    // 3. 軟體重置 (Software Reset)
    // 寫入 0x10 到 Command 暫存器 (Offset 0x37) 觸發重置
    outb(rtl_iobase + 0x37, 0x10);

    // 不斷讀取，直到 0x10 這個 bit 被硬體清空，代表重置完成
    while ((inb(rtl_iobase + 0x37) & 0x10) != 0) {
        // 等待硬體重置...
    }
    kprintf("[RTL8139] Device reset successful.\n");

    // 4. 讀取 MAC 位址
    // RTL8139 的 MAC 位址直接存在 I/O Base 的最前面 6 個 Byte (Offset 0x00 ~ 0x05)
    for (int i = 0; i < 6; i++) {
        mac_addr[i] = inb(rtl_iobase + i);
    }

    kprintf("[RTL8139] MAC Address: [%x:%x:%x:%x:%x:%x]\n",
            mac_addr[0], mac_addr[1], mac_addr[2],
            mac_addr[3], mac_addr[4], mac_addr[5]);


    // ==========================================
    // 【Day 83 新增】網卡接收引擎與中斷設定
    // ==========================================

    // 5. 將我們準備好的 Rx Buffer 實體位址告訴網卡 (寫入 RBSTART 暫存器 0x30)
    // 註：這裡假設你的 Kernel 是 Identity Mapped (虛擬位址 == 實體位址)
    outl(rtl_iobase + 0x30, (uint32_t)rx_buffer);

    // 6. 設定中斷遮罩 (IMR - Interrupt Mask Register, 0x3C)
    // 0x0005 代表我們只想監聽 "Rx OK (接收成功)" 和 "Tx OK (發送成功)" 的中斷
    outw(rtl_iobase + 0x3C, 0x0005);

    // 7. 設定接收配置暫存器 (RCR - Receive Configuration Register, 0x44)
    // (AB|AM|APM|AAP) = 0xF 代表允許接收：廣播、多播、MAC符合的封包
    // WRAP (bit 7) = 1 代表當 Buffer 滿了時，網卡會自動從頭覆寫 (環狀機制)
    outl(rtl_iobase + 0x44, 0xf | (1 << 7));

    // 8. 正式啟動接收 (Rx) 與發送 (Tx) 引擎！
    // Command Register (0x37), RE(bit 3)=1, TE(bit 2)=1 -> 組合起來就是 0x0C
    outb(rtl_iobase + 0x37, 0x0C);

    // 9. 從 PCI Config Space 讀出這張網卡被分配到的 IRQ 號碼
    uint32_t irq_info = pci_read_config_word(bus, slot, 0, 0x3C);
    uint8_t irq_line = irq_info & 0xFF; // 取出最低 8 bits

    kprintf("[RTL8139] Hardware initialized. Assigned IRQ: %d\n", irq_line);
}


// ==========================================
// 【Day 84 新增】RTL8139 中斷處理常式
// ==========================================
void rtl8139_handler(void) {
    // 1. 讀取中斷狀態暫存器 (ISR, Offset 0x3E)
    uint16_t status = inw(rtl_iobase + 0x3E);

    if (!status) return;

    // 2. 寫回 ISR 來清除中斷旗標 (寫入 1 代表清除)
    outw(rtl_iobase + 0x3E, status);

    // 3. 檢查是否為 "Receive OK" (Bit 0)
    if (status & 0x01) {

        // 不斷讀取，直到硬體告訴我們 Buffer 空了 (CR 暫存器的 Bit 0 為 1 代表空)
        while ((inb(rtl_iobase + 0x37) & 0x01) == 0) {

            // RTL8139 封包的開頭會有 4 個 bytes 的硬體檔頭：
            // [ 16-bit Rx Status ] [ 16-bit Packet Length ]
            uint16_t* rx_header = (uint16_t*)(rx_buffer + rx_offset);
            uint16_t rx_length = rx_header[1];

            // 實際的網路封包位址 (跳過那 4 bytes 硬體檔頭)
            uint8_t* packet_data = rx_buffer + rx_offset + 4;

            // 將封包轉型為乙太網路標頭！
            ethernet_header_t* eth = (ethernet_header_t*)packet_data;

            kprintf("[Net] Packet RX! Len: %d bytes\n", rx_length);
            kprintf("  -> Dest MAC: %x:%x:%x:%x:%x:%x\n",
                    eth->dest_mac[0], eth->dest_mac[1], eth->dest_mac[2],
                    eth->dest_mac[3], eth->dest_mac[4], eth->dest_mac[5]);

            uint16_t type = ntohs(eth->ethertype);
            if (type == ETHERTYPE_ARP) kprintf("  -> Protocol: ARP\n");
            else if (type == ETHERTYPE_IPv4) kprintf("  -> Protocol: IPv4\n");
            else kprintf("  -> Protocol: Unknown (0x%x)\n", type);

            // 4. 更新 Rx_offset
            // 長度包含 4 bytes CRC，且必須對齊 4 bytes 邊界
            rx_offset = (rx_offset + rx_length + 4 + 3) & ~3;

            // 若超過 8K (8192) 邊界，會繞回開頭 (Wrap around)
            if (rx_offset >= 8192) {
                rx_offset -= 8192;
            }

            // 5. 告訴網卡我們讀到哪裡了 (寫入 CAPR 暫存器 0x38)
            // (註：硬體設計怪癖，必須減掉 16)
            outw(rtl_iobase + 0x38, rx_offset - 16);
        }
    }

    // 發送 EOI 給中斷控制器 (PIC)
    // 因為 IRQ 11 接在 Slave PIC 上，所以要發給 Master & Slave
    outb(0xA0, 0x20);
    outb(0x20, 0x20);
}
```

---
rtl8139.h
```c
#ifndef RTL8139_H
#define RTL8139_H

#include <stdint.h>

void init_rtl8139(uint8_t bus, uint8_t slot);
void rtl8139_handler(void);

#endif
```
