
成功編譯，進入 desktop mode，在 terminal 輸入 `segfault` 後就 crash 了，底下是 core dump 和部分 source code.

感覺是 task.c 有部分判斷有問題？

```bash
   533: v=20 e=0000 i=0 cpl=0 IP=0008:00102297 pc=00102297 SP=0010:c0005f84 env->regs[R_EAX]=c0000174
EAX=c0000174 EBX=0010b138 ECX=00000000 EDX=00000008
ESI=c0005fd8 EDI=00000000 EBP=083ffd18 ESP=c0005f84
EIP=00102297 EFL=00000246 [---Z-P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
DS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
FS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
GS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 00110000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     00110080 0000002f
IDT=     002e4fa0 000007ff
CR0=80000011 CR2=00000000 CR3=0031d000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000044 CCD=00000000 CCO=EFLAGS
EFER=0000000000000000
Servicing hardware INT=0x21
   534: v=21 e=0000 i=0 cpl=0 IP=0008:00102297 pc=00102297 SP=0010:c0005f84 env->regs[R_EAX]=c0000174
EAX=c0000174 EBX=0010b138 ECX=00000000 EDX=00000008
ESI=c0005fd8 EDI=00000000 EBP=083ffd18 ESP=c0005f84
EIP=00102297 EFL=00000246 [---Z-P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
DS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
FS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
GS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 00110000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     00110080 0000002f
IDT=     002e4fa0 000007ff
CR0=80000011 CR2=00000000 CR3=0031d000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000044 CCD=00000000 CCO=EFLAGS
EFER=0000000000000000
   535: v=80 e=0000 i=1 cpl=3 IP=001b:08048af6 pc=08048af6 SP=0023:083ffd2c env->regs[R_EAX]=00000008
EAX=00000008 EBX=00000000 ECX=00000000 EDX=00000000
ESI=00000000 EDI=00000000 EBP=083ffd48 ESP=083ffd2c
EIP=08048af6 EFL=00000202 [-------] CPL=3 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =001b 00000000 ffffffff 00cffa00 DPL=3 CS32 [-R-]
SS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
DS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
FS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
GS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 00110000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     00110080 0000002f
IDT=     002e4fa0 000007ff
CR0=80000011 CR2=00000000 CR3=0031d000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000010 CCD=083ffd2c CCO=SUBL
EFER=0000000000000000
   536: v=80 e=0000 i=1 cpl=3 IP=001b:08048af6 pc=08048af6 SP=0023:083ffd1c env->regs[R_EAX]=0000000a
EAX=0000000a EBX=00000002 ECX=00000000 EDX=00000000
ESI=00000000 EDI=00000000 EBP=083ffd38 ESP=083ffd1c
EIP=08048af6 EFL=00000202 [-------] CPL=3 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =001b 00000000 ffffffff 00cffa00 DPL=3 CS32 [-R-]
SS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
DS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
FS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
GS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 00110000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     00110080 0000002f
IDT=     002e4fa0 000007ff
CR0=80000011 CR2=00000000 CR3=0031d000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000010 CCD=083ffd1c CCO=SUBL
EFER=0000000000000000
check_exception old: 0xffffffff new 0x1
   537: v=01 e=0000 i=0 cpl=3 IP=0202:0000001d pc=0000203d SP=1000:083fd023 env->regs[R_EAX]=00000000
EAX=00000000 EBX=00000000 ECX=00000023 EDX=00000000
ESI=00000000 EDI=00000000 EBP=083ffd48 ESP=083fd023
EIP=0000001d EFL=003e7506 [D----P-] CPL=3 II=0 A20=1 SMM=0 HLT=0
ES =8fdc 0008fdc0 0000ffff 0000f300 DPL=3 DS16 [-WA]
CS =0202 00002020 0000ffff 0000f300 DPL=3 DS16 [-WA]
SS =1000 00010000 0000ffff 0000f300 DPL=3 DS16 [-WA]
DS =0001 00000010 0000ffff 0000f300 DPL=3 DS16 [-WA]
FS =0000 00000000 0000ffff 0000f300 DPL=3 DS16 [-WA]
GS =0000 00000000 0000ffff 0000f300 DPL=3 DS16 [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 00110000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     00110080 0000002f
IDT=     002e4fa0 000007ff
CR0=80000011 CR2=00000000 CR3=0031d000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff4ff0 DR7=00000400
CCS=00000000 CCD=00000053 CCO=ADDB
EFER=0000000000000000
check_exception old: 0xffffffff new 0xd
   538: v=0d e=000a i=0 cpl=3 IP=0202:0000001d pc=0000203d SP=1000:083fd023 env->regs[R_EAX]=00000000
EAX=00000000 EBX=00000000 ECX=00000023 EDX=00000000
ESI=00000000 EDI=00000000 EBP=083ffd48 ESP=083fd023
EIP=0000001d EFL=003e7506 [D----P-] CPL=3 II=0 A20=1 SMM=0 HLT=0
ES =8fdc 0008fdc0 0000ffff 0000f300 DPL=3 DS16 [-WA]
CS =0202 00002020 0000ffff 0000f300 DPL=3 DS16 [-WA]
SS =1000 00010000 0000ffff 0000f300 DPL=3 DS16 [-WA]
DS =0001 00000010 0000ffff 0000f300 DPL=3 DS16 [-WA]
FS =0000 00000000 0000ffff 0000f300 DPL=3 DS16 [-WA]
GS =0000 00000000 0000ffff 0000f300 DPL=3 DS16 [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 00110000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     00110080 0000002f
IDT=     002e4fa0 000007ff
CR0=80000011 CR2=00000000 CR3=0031d000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff4ff0 DR7=00000400
CCS=00000000 CCD=00000053 CCO=ADDB
EFER=0000000000000000
check_exception old: 0xd new 0xd
   539: v=08 e=0000 i=0 cpl=3 IP=0202:0000001d pc=0000203d SP=1000:083fd023 env->regs[R_EAX]=00000000
EAX=00000000 EBX=00000000 ECX=00000023 EDX=00000000
ESI=00000000 EDI=00000000 EBP=083ffd48 ESP=083fd023
EIP=0000001d EFL=003e7506 [D----P-] CPL=3 II=0 A20=1 SMM=0 HLT=0
ES =8fdc 0008fdc0 0000ffff 0000f300 DPL=3 DS16 [-WA]
CS =0202 00002020 0000ffff 0000f300 DPL=3 DS16 [-WA]
SS =1000 00010000 0000ffff 0000f300 DPL=3 DS16 [-WA]
DS =0001 00000010 0000ffff 0000f300 DPL=3 DS16 [-WA]
FS =0000 00000000 0000ffff 0000f300 DPL=3 DS16 [-WA]
GS =0000 00000000 0000ffff 0000f300 DPL=3 DS16 [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 00110000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     00110080 0000002f
IDT=     002e4fa0 000007ff
CR0=80000011 CR2=00000000 CR3=0031d000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff4ff0 DR7=00000400
CCS=00000000 CCD=00000053 CCO=ADDB
EFER=0000000000000000
check_exception old: 0x8 new 0xd
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

    // 【Day 70 新增】掛載第 14 號中斷：Page Fault
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);


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
```

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
    call isr0_handler
    sti
    iret

; [day70] 第 14 號中斷：Page Fault (CPU 會自動壓入 Error Code)
isr14:
    pusha           ; 壓入 eax, ecx, edx, ebx, esp, ebp, esi, edi
    push esp        ; 傳遞 registers_t 指標
    call page_fault_handler
    add esp, 4      ; 清除參數
    popa            ; 恢復通用暫存器
    add esp, 4      ; 【重要】手動跳過 CPU 壓入的 Error Code！
    iret


isr32:
    pusha
    call timer_handler
    popa
    iret

isr33:
    pusha
    call keyboard_handler
    popa
    iret


isr44:
    pusha
    call mouse_handler
    popa
    iret

; 第 128 號中斷 (System Call 窗口)
isr128:
    pusha           ; 備份通用暫存器 (32 bytes)
    push esp        ; 把 registers_t 的指標傳給 C 語言
    call syscall_handler
    add esp, 4      ; 清除 esp 參數
    ; 魔法：syscall_handler 已經把回傳值寫進堆疊的 EAX 位置了
    popa            ; 恢復所有暫存器
    iret
```

---
task.h
```c
#ifndef TASK_H
#define TASK_H
#include <stdint.h>

// 行程狀態定義 (保留你原本的，並新增 ZOMBIE 狀態)
#define TASK_RUNNING 0
#define TASK_DEAD    1
#define TASK_WAITING 2
#define TASK_ZOMBIE  3  // 【新增】行程已結束，但父行程尚未 wait() 回收它


/**
 * [結構目的]：中斷上下文快照 (Interrupt Context Snapshot)
 * * 此結構體的佈局必須嚴格遵循 x86 中斷發生時的堆疊增長順序。
 * 當進入 isr128 (或其他中斷) 時：
 * 1. CPU 自動壓入: ss, esp, eflags, cs, eip (共 5 個)
 * 2. 軟體手動壓入: pusha (壓入 8 個通用暫存器)
 * * 記憶體佈局 (由低位址往高位址)：
 * [ 低位址 ] edi -> esi -> ebp -> esp -> ebx -> edx -> ecx -> eax -> eip -> cs -> eflags -> user_esp -> user_ss [ 高位址 ]
 */
/**
 * 位址方向      暫存器內容 (Data)          說明 / 來源
    --------    -----------------------    --------------------------
     高位址  ↑   [  User SS      ] 0x23     <-- CPU 自動壓入 (最後)
            |   [  User ESP     ] 0x...    <-- CPU 自動壓入
            |   [  EFLAGS       ] 0x202    <-- CPU 自動壓入 (中斷狀態)
            |   [  CS (Code Seg)] 0x1B     <-- CPU 自動壓入 (權限位元)
            |   [  EIP (Return) ] 0x...    <-- CPU 自動壓入 (第一)
            |   -----------------------    (以上為 Interrupt Frame)
            |   [  EAX          ]          <-- pusha 開始 (1st)
            |   [  ECX          ]          <-- pusha
            |   [  EDX          ]          <-- pusha
            |   [  EBX          ]          <-- pusha
            |   [  ESP (Kernel) ]          <-- pusha (在此時的 ESP)
            |   [  EBP          ]          <-- pusha
            |   [  ESI          ]          <-- pusha
     低位址  ↓   [  EDI          ]          <-- pusha 結束 (Last)
    --------    -----------------------    <-- 這裡就是 registers_t *regs 的指標位置
 */
typedef struct {
    /* 由 pusha 指令壓入的通用暫存器 (順序由後往前) */
    uint32_t edi;    // Destination Index: 常用於記憶體拷貝目的地
    uint32_t esi;    // Source Index: 常用於記憶體拷貝來源
    uint32_t ebp;    // Base Pointer: 堆疊基底指標，用於 Stack Trace
    uint32_t esp;    // Stack Pointer: 核心模式下的堆疊頂端 (pusha 壓入的值)
    uint32_t ebx;    // Base Register: 常用於存取資料段位址
    uint32_t edx;    // Data Register: 常用於 I/O 埠操作或乘除法
    uint32_t ecx;    // Counter Register: 常用於迴圈計數
    uint32_t eax;    // Accumulator Register: 函式回傳值或 Syscall 編號

    /* 【Day 70 新增】由 CPU 在某些例外發生時自動壓入的錯誤碼 */
    /* 如果是一般中斷，我們會在 ASM 裡手動補一個 0，確保結構對齊 */
    uint32_t err_code;

    /* 以下由 CPU 在發生中斷時「自動」壓入堆疊 (Interrupt Stack Frame) */
    uint32_t eip;    // Instruction Pointer: 程式被中斷時的下一條指令位址
    uint32_t cs;     // Code Segment: 包含權限等級 (CPL)，決定是 Ring 0 還是 3
    uint32_t eflags; // CPU Flags: 包含中斷開關 (IF) 與運算狀態標誌

    /* 以下兩項僅在「權限切換」(Ring 3 -> Ring 0) 時由 CPU 自動壓入 */
    uint32_t user_esp; // User Stack Pointer: 使用者原本的堆疊位址
    uint32_t user_ss;  // User Stack Segment: 使用者原本的堆疊段選擇子 (通常 0x23)
} registers_t;


typedef struct task {
    uint32_t esp;

    // ==========================================
    // 【Day 62 修改與新增】行程身分資訊 (PCB 擴充)
    // ==========================================
    uint32_t pid;               // 將原本的 id 改名為 pid (更符合 OS 慣例)
    uint32_t ppid;              // 【新增】父行程 ID (Parent PID)
    char name[32];              // 【新增】行程名稱 (例如 "shell.elf")

    // 【Day 65 新增】記錄這個行程總共跑了多少個 Ticks
    uint32_t total_ticks;

    uint32_t page_directory;    // 這個 Task 專屬的分頁目錄實體位址 (CR3)
    uint32_t kernel_stack;

    uint32_t state;
    uint32_t wait_pid;          // 保留！這是你 sys_wait 的核心機制

    uint32_t heap_start;        // 【新增】記錄 Heap 初始起點 (未來給 top 計算記憶體佔用量用)
    uint32_t heap_end;          // 記錄 User Heap 的當前頂點
    uint32_t cwd_lba;           // 當前工作目錄 (Current Working Directory, CWD) 的 LBA
    struct task *next;
} task_t;

// 【Day 63 新增】用於傳遞給 User Space 的行程資訊
typedef struct {
    uint32_t pid;
    uint32_t ppid;
    char name[32];
    uint32_t state;
    uint32_t memory_used; // 佔用記憶體大小 (Bytes)
    uint32_t total_ticks; // 【Day 65 新增】
} process_info_t;



extern volatile task_t *current_task;

void init_multitasking();

void create_user_task(uint32_t entry_point, uint32_t user_stack_top);
void exit_task();
void schedule();
int sys_fork(registers_t *regs);
int sys_exec(registers_t *regs);
int sys_wait(int pid);

int task_get_process_list(process_info_t* list, int max_count);


#endif
```

---
task.c
```c
#include <stdint.h>
#include "task.h"
#include "pmm.h"
#include "kheap.h"
#include "utils.h"
#include "tty.h"
#include "gdt.h"
#include "paging.h"
#include "simplefs.h"
#include "vfs.h"
#include "elf.h"

volatile task_t *current_task = 0;
volatile task_t *ready_queue = 0;
uint32_t next_pid = 0; // 【Day 62 修改】將 next_task_id 改名為 next_pid
task_t *idle_task = 0;

extern uint32_t page_directory[];
extern void switch_task(uint32_t *current_esp, uint32_t *next_esp, uint32_t next_cr3);  // switch_task.S
extern void child_ret_stub();   // switch_task.S

// --- Internal API ----

void idle_loop() {
    while(1) { __asm__ volatile("sti; hlt"); }
}

// ---- Public API ----

void init_multitasking() {
    task_t *main_task = (task_t*) kmalloc(sizeof(task_t));
    main_task->pid = next_pid++; // 【Day 62 修改】
    main_task->ppid = 0;         // 【Day 62 新增】
    strcpy(main_task->name, "[kernel_main]"); // 【Day 62 新增】

    main_task->esp = 0;
    main_task->kernel_stack = 0;
    main_task->state = TASK_RUNNING;
    main_task->wait_pid = 0;
    main_task->page_directory = (uint32_t) page_directory;
    main_task->cwd_lba = 0;
    main_task->total_ticks = 0;

    main_task->next = main_task;
    current_task = main_task;
    ready_queue = main_task;

    idle_task = (task_t*) kmalloc(sizeof(task_t));
    idle_task->pid = 9999;       // 【Day 62 修改】
    idle_task->ppid = 0;         // 【Day 62 新增】
    strcpy(idle_task->name, "[kernel_idle]"); // 【Day 62 新增】
    idle_task->state = TASK_RUNNING;
    idle_task->wait_pid = 0;
    idle_task->page_directory = (uint32_t) page_directory;
    idle_task->cwd_lba = 0;
    idle_task->total_ticks = 0;

    uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
    idle_task->kernel_stack = (uint32_t) kstack;

    *(--kstack) = (uint32_t) idle_loop;
    for(int i = 0; i < 8; i++) *(--kstack) = 0;
    *(--kstack) = 0x0202;

    idle_task->esp = (uint32_t) kstack;
}

void create_user_task(uint32_t entry_point, uint32_t user_stack_top) {
    task_t *new_task = (task_t*) kmalloc(sizeof(task_t));
    new_task->pid = next_pid++; // 【Day 62 修改】
    new_task->ppid = current_task->pid; // 【Day 62 新增】
    strcpy(new_task->name, "init");     // 【Day 62 新增】初代程式命名為 init

    new_task->state = TASK_RUNNING;
    new_task->wait_pid = 0;
    new_task->page_directory = current_task->page_directory;
    new_task->cwd_lba = 0;
    new_task->total_ticks = 0;

    // 為初代老爸 (Shell) 預先分配 10 個實體分頁給 User Heap
    // 預先分配 256 個實體分頁 (1MB) 給 User Heap
    for (int i = 0; i < 256; i++) {
        uint32_t heap_phys = pmm_alloc_page();
        map_page(0x10000000 + (i * 4096), heap_phys, 7);
    }
    new_task->heap_start = 0x10000000; // 【Day 62 新增】
    new_task->heap_end = 0x10000000;

    uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
    new_task->kernel_stack = (uint32_t) kstack;

    uint32_t *ustack = (uint32_t*) (user_stack_top - 64);
    *(--ustack) = 0;
    uint32_t argv_ptr = (uint32_t) ustack;
    *(--ustack) = argv_ptr;
    *(--ustack) = 0;
    *(--ustack) = 0;

    *(--kstack) = 0x23;
    *(--kstack) = (uint32_t)ustack;
    *(--kstack) = 0x0202;
    *(--kstack) = 0x1B;
    *(--kstack) = entry_point;

    *(--kstack) = 0;
    *(--kstack) = 0;
    *(--kstack) = 0;
    *(--kstack) = 0;

    *(--kstack) = (uint32_t) child_ret_stub;

    for(int i = 0; i < 8; i++) *(--kstack) = 0;
    *(--kstack) = 0x0202;

    new_task->esp = (uint32_t) kstack;
    new_task->next = current_task->next;
    current_task->next = new_task;
}

void exit_task() {
    task_t *temp = current_task->next;
    while (temp != current_task) {
        // 【Day 62 修改】改用 pid 判斷
        if (temp->state == TASK_WAITING && temp->wait_pid == current_task->pid) {
            temp->state = TASK_RUNNING;
            temp->wait_pid = 0;
        }
        temp = temp->next;
    }

    if (current_task->page_directory != (uint32_t)page_directory) {
        free_page_directory(current_task->page_directory);
    }

    // 【修改】如果沒有老爸 (ppid == 0)，就直接 DEAD，否則變 ZOMBIE
    if (current_task->ppid == 0) {
        current_task->state = TASK_DEAD;
    } else {
        current_task->state = TASK_ZOMBIE;
    }

    schedule();
}

void schedule() {
    if (!current_task) return;

    task_t *curr = (task_t*)current_task;
    task_t *next_node = curr->next;

    // 尋找並剔除 DEAD 的任務
    while (next_node != current_task) {
        if (next_node->state == TASK_DEAD) {
            // 1. 將它從 Circular Linked List 拔除
            curr->next = next_node->next;

            // ==========================================
            // 【Day 67 核心新增】釋放記憶體 (Garbage Collection)
            // ==========================================
            if (next_node->kernel_stack != 0) {
                // 還記得我們當初是 kmalloc(4096)，然後把指標 +4096 嗎？
                // 現在要減回去才能正確 free！
                kfree((void*)(next_node->kernel_stack - 4096));
            }
            // 釋放 PCB 結構體本身
            kfree((void*)next_node);

            // 往下看下一個節點
            next_node = curr->next;
        } else {
            // 只有不是 DEAD 的任務，才繼續往下走
            curr = next_node;
            next_node = curr->next;
        }
    }

    task_t *next_run = current_task->next;
    while (next_run->state != TASK_RUNNING && next_run != current_task) {
        next_run = next_run->next;
    }

    if (next_run->state != TASK_RUNNING) {
        next_run = idle_task;
    }

    task_t *prev = (task_t*)current_task;
    current_task = next_run;

    if (current_task != prev) {
        if (current_task->kernel_stack != 0) {
            set_kernel_stack(current_task->kernel_stack);
        }
        switch_task(&prev->esp, &current_task->esp, current_task->page_directory);
    }
}

int sys_fork(registers_t *regs) {
    task_t *child = (task_t*) kmalloc(sizeof(task_t));
    child->pid = next_pid++;                 // 【Day 62 修改】
    child->ppid = current_task->pid;         // 【Day 62 新增】
    strcpy(child->name, current_task->name); // 【Day 62 新增】繼承老爸的名字

    child->state = TASK_RUNNING;
    child->wait_pid = 0;
    child->page_directory = current_task->page_directory;
    child->cwd_lba = current_task->cwd_lba;

    child->heap_start = current_task->heap_start; // 【Day 62 新增】
    child->heap_end = current_task->heap_end;
    child->total_ticks = 0; // 子行程出生時，努力值當然要歸零重新計算！

    uint32_t *kstack_mem = (uint32_t*) kmalloc(4096);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + 4096);
    child->kernel_stack = (uint32_t) kstack;

    // 【Day 62 修改】改用 child->pid 確保位址不衝突
    uint32_t child_ustack_base = 0x083FF000 - (child->pid * 4096);
    uint32_t child_ustack_phys = pmm_alloc_page();
    map_page(child_ustack_base, child_ustack_phys, 7);

    uint32_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3));

    uint32_t parent_ustack_base = regs->user_esp & 0xFFFFF000;
    uint8_t *src = (uint8_t*) parent_ustack_base;
    uint8_t *dst = (uint8_t*) child_ustack_base;
    for(int i = 0; i < 4096; i++) { dst[i] = src[i]; }

    int32_t delta = child_ustack_base - parent_ustack_base;
    uint32_t child_user_esp = regs->user_esp + delta;

    uint32_t child_ebp = regs->ebp;
    if (child_ebp >= parent_ustack_base && child_ebp < parent_ustack_base + 4096) {
        child_ebp += delta;
    }

    uint32_t curr_ebp = child_ebp;
    while (curr_ebp >= child_ustack_base && curr_ebp < child_ustack_base + 4096) {
        uint32_t *ebp_ptr = (uint32_t*) curr_ebp;
        uint32_t next_ebp = *ebp_ptr;
        if (next_ebp >= parent_ustack_base && next_ebp < parent_ustack_base + 4096) {
            *ebp_ptr = next_ebp + delta;
            curr_ebp = *ebp_ptr;
        } else {
            break;
        }
    }

    *(--kstack) = regs->user_ss;
    *(--kstack) = child_user_esp;
    *(--kstack) = regs->eflags;
    *(--kstack) = regs->cs;
    *(--kstack) = regs->eip;

    *(--kstack) = child_ebp;
    *(--kstack) = regs->ebx;
    *(--kstack) = regs->esi;
    *(--kstack) = regs->edi;

    *(--kstack) = (uint32_t) child_ret_stub;

    for(int i = 0; i < 8; i++) *(--kstack) = 0;
    *(--kstack) = 0x0202;

    child->esp = (uint32_t) kstack;
    child->next = current_task->next;
    current_task->next = child;

    return child->pid; // 【Day 62 修改】回傳 child 的 PID
}

int sys_exec(registers_t *regs) {
    char* filename = (char*)regs->ebx;
    char** old_argv = (char**)regs->ecx;

    uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;
    fs_node_t* file_node = simplefs_find(current_dir, filename);

    if (file_node == 0 && current_dir != 1) {
        file_node = simplefs_find(1, filename);
    }
    if (file_node == 0) { return -1; }

    // ==========================================
    // 【Day 62 新增】靈魂轉移：更新行程名稱！
    // ==========================================
    strcpy(current_task->name, filename);

    uint8_t* buffer = (uint8_t*) kmalloc(file_node->length);
    vfs_read(file_node, 0, file_node->length, buffer);

    int argc = 0;
    char* safe_argv[16];

    if (old_argv != 0 && (uint32_t)old_argv > 0x08000000) {
        while (old_argv[argc] != 0 && argc < 15) {
            int len = strlen(old_argv[argc]) + 1;
            safe_argv[argc] = (char*) kmalloc(len);
            memcpy(safe_argv[argc], old_argv[argc], len);
            argc++;
        }
    }
    safe_argv[argc] = 0;

    uint32_t new_cr3 = create_page_directory();
    uint32_t old_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(old_cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(new_cr3));

    uint32_t entry_point = elf_load((elf32_ehdr_t*)buffer);
    kfree(buffer);

    if (entry_point == 0) {
        __asm__ volatile("mov %0, %%cr3" : : "r"(old_cr3));
        return -1;
    }

    current_task->page_directory = new_cr3;

    uint32_t clean_user_stack_top = 0x083FF000 + 4096;
    uint32_t ustack_phys = pmm_alloc_page();
    map_page(0x083FF000, ustack_phys, 7);

    // 預先分配 256 個實體分頁 (1MB) 給 User Heap
    for (int i = 0; i < 256; i++) {
        uint32_t heap_phys = pmm_alloc_page();
        map_page(0x10000000 + (i * 4096), heap_phys, 7);
    }
    current_task->heap_start = 0x10000000; // 【Day 62 新增】
    current_task->heap_end = 0x10000000;

    uint32_t stack_ptr = clean_user_stack_top - 64;

    uint32_t argv_ptrs[16] = {0};
    if (argc > 0) {
        for (int i = argc - 1; i >= 0; i--) {
            int len = strlen(safe_argv[i]) + 1;
            stack_ptr -= len;
            memcpy((void*)stack_ptr, safe_argv[i], len);
            argv_ptrs[i] = stack_ptr;
            kfree(safe_argv[i]);
        }
        stack_ptr = stack_ptr & ~3;
        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = 0;
        for (int i = argc - 1; i >= 0; i--) {
            stack_ptr -= 4;
            *(uint32_t*)stack_ptr = argv_ptrs[i];
        }
        uint32_t argv_base = stack_ptr;
        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = argv_base;
        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = argc;
    } else {
        stack_ptr -= 4; *(uint32_t*)stack_ptr = 0;
        stack_ptr -= 4; *(uint32_t*)stack_ptr = 0;
    }

    stack_ptr -= 4; *(uint32_t*)stack_ptr = 0;

    regs->eip = entry_point;
    regs->user_esp = stack_ptr;

    return 0;
}

int sys_wait(int pid) {
    task_t *temp = current_task->next;
    int found = 0;

    // 1. 先檢查兒子是不是「已經是殭屍」了？
    while (temp != current_task) {
        if (temp->pid == (uint32_t)pid) {
            found = 1;
            if (temp->state == TASK_ZOMBIE) {
                // 兒子早就死了！直接收屍並返回，不用睡覺！
                temp->state = TASK_DEAD;
                return 0;
            }
            break;
        }
        temp = temp->next;
    }

    if (!found) return -1; // 根本沒有這個兒子

    // 2. 兒子還活著，老爸只好乖乖睡覺等待
    current_task->wait_pid = pid;
    current_task->state = TASK_WAITING;
    schedule();

    // 3. 【Day 67 新增】老爸被喚醒了！代表兒子剛死，趕快去收屍！
    temp = current_task->next;
    while (temp != current_task) {
        if (temp->pid == (uint32_t)pid && temp->state == TASK_ZOMBIE) {
            temp->state = TASK_DEAD; // 正式宣告死亡！
            break;
        }
        temp = temp->next;
    }

    return 0;
}

// 【Day 66 新增】終結指定的行程
int sys_kill(int pid) {
    if (pid <= 1) return -1; // 防禦機制：絕對不准殺 Kernel Idle (0) 和 Kernel Main (1)！

    __asm__ volatile("cli");
    task_t *temp = (task_t*)current_task;
    int found = 0;

    // 遍歷所有行程尋找目標
    do {
        if (temp->pid == (uint32_t)pid && temp->state != TASK_DEAD) {
            // 【Day 67 修改】變成殭屍
            temp->state = TASK_ZOMBIE;

            // 【Day 67 新增】如果老爸正在等它死，我們要順便把老爸叫醒！
            task_t *parent = current_task;
            do {
                if (parent->pid == temp->ppid && parent->state == TASK_WAITING && parent->wait_pid == pid) {
                    parent->state = TASK_RUNNING;
                    parent->wait_pid = 0;
                }
                parent = parent->next;
            } while (parent != current_task);

            found = 1;
            break;
        }
        temp = temp->next;
    } while (temp != current_task);

    __asm__ volatile("sti");

    return found ? 0 : -1;
}

// 【Day 63 新增】收集所有行程資料
int task_get_process_list(process_info_t* list, int max_count) {
    if (!current_task) return 0;

    int count = 0;
    task_t* temp = (task_t*)current_task;

    // 遍歷 Circular Linked List
    do {
        // 不要回報已經徹底死亡的行程
        if (temp->state != TASK_DEAD) {
            list[count].pid = temp->pid;
            list[count].ppid = temp->ppid;
            // 【修改】使用 strncpy 防止越界，最大長度 32
            // strcpy(list[count].name, temp->name);
            strncpy(list[count].name, temp->name, 32);
            list[count].state = temp->state;

            // 在迴圈內加入這一行拷貝邏輯
            list[count].total_ticks = temp->total_ticks;

            // 計算 Heap 動態佔用的空間 (End - Start)
            if (temp->heap_end >= temp->heap_start) {
                // User Task: 動態 Heap + 預先分配的 1MB Heap + 4KB User Stack
                list[count].memory_used = (temp->heap_end - temp->heap_start) + (256 * 4096) + 4096;
            } else {
                // Kernel Task: 只有 Kernel Stack (4KB)
                list[count].memory_used = 4096;
            }

            count++;
        }
        temp = temp->next;
    } while (temp != current_task && count < max_count);

    return count; // 回傳總共找到了幾個行程
}
```

---
paging.c
```c
#include <stdint.h>
#include <stddef.h>
#include "paging.h"
#include "utils.h"
#include "pmm.h"
#include "task.h" // 確保有引入 task.h 才能知道 current_task

uint32_t page_directory[1024] __attribute__((aligned(4096)));

uint32_t first_page_table[1024] __attribute__((aligned(4096)));
uint32_t second_page_table[1024] __attribute__((aligned(4096)));
uint32_t third_page_table[1024] __attribute__((aligned(4096)));
// user page
uint32_t user_page_table[1024] __attribute__((aligned(4096)));
uint32_t user_heap_page_table[1024] __attribute__((aligned(4096)));
// vram page
uint32_t vram_page_table[1024] __attribute__((aligned(4096)));

// ====================================================================
// 預先分配 16 個宇宙的空間！
// ====================================================================
uint32_t universe_pds[16][1024] __attribute__((aligned(4096)));
uint32_t universe_pts[16][1024] __attribute__((aligned(4096)));
uint32_t universe_heap_pts[16][1024] __attribute__((aligned(4096)));    // 平行宇宙的 Heap 表陣列

int next_universe_id = 0;

// 在全域變數區新增一個陣列，記錄哪個宇宙被使用了
int universe_used[16] = {0};

extern void load_page_directory(uint32_t*);
extern void enable_paging(void);

void init_paging(void) {
    for(int i = 0; i < 1024; i++) { page_directory[i] = 0x00000002; }
    for(int i = 0; i < 1024; i++) { first_page_table[i] = (i * 0x1000) | 7; }
    for(int i = 0; i < 1024; i++) { second_page_table[i] = 0; }
    for(int i = 0; i < 1024; i++) { third_page_table[i] = 0; }
    for(int i = 0; i < 1024; i++) { user_page_table[i] = 0; }
    for(int i = 0; i < 1024; i++) { user_heap_page_table[i] = 0; } // 清空

    page_directory[0] = ((uint32_t)first_page_table) | 7;
    page_directory[32] = ((uint32_t)user_page_table) | 7;

    // [掛載 0x10000000 區域 (pd_idx = 64)
    page_directory[64] = ((uint32_t)user_heap_page_table) | 7;

    page_directory[512] = ((uint32_t)second_page_table) | 3;
    page_directory[768] = ((uint32_t)third_page_table) | 3;

    load_page_directory(page_directory);
    enable_paging();
}

void map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x03FF;

    uint32_t* page_table;

    if (pd_idx == 0) {
        page_table = first_page_table;
    } else if (pd_idx == 32) {
        // Stack & Code 區 (0x08000000)
        uint32_t cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
        uint32_t* current_pd = (uint32_t*) cr3;
        uint32_t pt_entry = current_pd[32];
        if (pt_entry & 1) {
            page_table = (uint32_t*)(pt_entry & 0xFFFFF000);
        } else {
            kprintf("Error: user page table not present in current CR3!\n");
            return;
        }
    } else if (pd_idx == 64) {
        // =========================================================
        // 【Heap 區】 (0x10000000)
        // =========================================================
        uint32_t cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
        uint32_t* current_pd = (uint32_t*) cr3;
        uint32_t pt_entry = current_pd[64]; // 檢查第 64 個目錄項

        if (pt_entry & 1) {
            page_table = (uint32_t*)(pt_entry & 0xFFFFF000);
        } else {
            kprintf("Error: heap page table not present in current CR3!\n");
            return;
        }
    } else if (pd_idx == 512) {
        page_table = second_page_table;
    } else if (pd_idx == 768) {
        page_table = third_page_table;
    } else {
        kprintf("Error: Page table not allocated for pd_idx [%d]!\n", pd_idx);
        return;
    }

    page_table[pt_idx] = (phys & 0xFFFFF000) | flags;
    __asm__ volatile("invlpg (%0)" ::"r" (virt) : "memory");
}

uint32_t create_page_directory() {

    int id = -1;
    // 尋找空閒的宇宙
    for (int i = 0; i < 16; i++) {
        if (!universe_used[i]) { id = i; break; }
    }
    if (id == -1) {
        kprintf("Error: Max universes reached!\n");
        while(1) __asm__ volatile("hlt");
    }

    universe_used[id] = 1; // 標記為使用中

    uint32_t* new_pd = universe_pds[id];
    uint32_t* new_pt = universe_pts[id];
    uint32_t* new_heap_pt = universe_heap_pts[id]; // 拿出這個宇宙專屬的 Heap 表

    for(int i = 0; i < 1024; i++) {
        new_pd[i] = page_directory[i];
    }

    for(int i = 0; i < 1024; i++) {
        new_pt[i] = 0;
        new_heap_pt[i] = 0; // 初始化清空
    }

    new_pd[32] = ((uint32_t)new_pt) | 7;
    // 將這張全新的 Heap 表掛載到新宇宙的 0x10000000 區段
    new_pd[64] = ((uint32_t)new_heap_pt) | 7;

    return (uint32_t)new_pd;
}


// 提供給 sys_exit 呼叫的回收函式
// 提供給 sys_exit 呼叫的回收函式
void free_page_directory(uint32_t pd_phys) {
    for (int i = 0; i < 16; i++) {
        if ((uint32_t)universe_pds[i] == pd_phys) {

            // 1. 釋放 Code & Stack 區段的物理分頁
            for (int j = 0; j < 1024; j++) {
                if (universe_pts[i][j] & 1) { // 檢查 Present Bit 是否為 1
                    uint32_t phys_addr = universe_pts[i][j] & 0xFFFFF000;
                    pmm_free_page((void*)phys_addr); // 還給實體記憶體管理器！
                    universe_pts[i][j] = 0;          // 清空分頁表紀錄
                }
            }

            // 2. 釋放 Heap 區段的物理分頁
            for (int j = 0; j < 1024; j++) {
                if (universe_heap_pts[i][j] & 1) {
                    uint32_t phys_addr = universe_heap_pts[i][j] & 0xFFFFF000;
                    pmm_free_page((void*)phys_addr);
                    universe_heap_pts[i][j] = 0;
                }
            }

            universe_used[i] = 0; // 解除佔用，讓給下一個程式！
            return;
        }
    }
}

// 專門用來映射 Framebuffer (MMIO, Memory-Mapped I/O) 的安全函數
void map_vram(uint32_t virt, uint32_t phys) {
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x03FF;

    if ((page_directory[pd_idx] & 1) == 0) {
        uint32_t pt_phys = (uint32_t)vram_page_table;
        for (int i = 0; i < 1024; i++) vram_page_table[i] = 0;
        page_directory[pd_idx] = pt_phys | 7;
    }
    vram_page_table[pt_idx] = phys | 7;
}

// 【新增】取得活躍的 Paging 宇宙數量
uint32_t paging_get_active_universes(void) {
    uint32_t count = 0;
    for (int i = 0; i < 16; i++) {
        if (universe_used[i]) count++;
    }
    return count;
}




// 【Day 70 新增】Page Fault 專屬的攔截與處理中心
void page_fault_handler(registers_t *regs) {
    uint32_t faulting_address;
    __asm__ volatile("mov %%cr2, %0" : "=r" (faulting_address));

    // 檢查錯誤碼的 bit 0：0 代表頁面不存在，1 代表權限違規
    int present = !(regs->err_code & 0x1);
    int rw = regs->err_code & 0x2;           // 0 為讀取，1 為寫入
    int us = regs->err_code & 0x4;           // 0 為核心模式，1 為使用者模式

    if (us) {
        kprintf("\n[Kernel] Segmentation Fault at 0x%x!\n", faulting_address);
        kprintf("  -> Cause: %s in %s mode\n",
                present ? "Page not present" : "Protection violation",
                rw ? "Write" : "Read");

        // 殺掉兇手！
        extern int sys_kill(int pid);
        sys_kill(current_task->pid);

        // 切換任務，永不回頭
        schedule();
    } else {
        // Kernel 自己崩潰了
        kprintf("\nKERNEL PANIC: Page Fault at 0x%x\n", faulting_address);
        while(1) __asm__ volatile("cli; hlt");
    }
}
```
