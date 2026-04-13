還是一樣跑起來後，就 hang 著不動了，底下是 core dump and 相關 source code.

main.c 的 timer_init(100);

task.c 有加入 `extern volatile uint32_t tick;`
task.c#schedule() 前後都有加上修改的部分

這次我把 interrupts.S, task_switch.S 也附上，我猜是不是跟 task.c#schedule() 有衝突？


```bash
12: v=20 e=0000 i=0 cpl=0 IP=0008:00103151 pc=00103151 SP=0010:00112c90 env->regs[R_EAX]=ffffff50
EAX=ffffff50 EBX=c0000618 ECX=c0000618 EDX=000001f7
ESI=00000002 EDI=00000001 EBP=c0000618 ESP=00112c90
EIP=00103151 EFL=00200286 [--S--P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 00328000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     00328080 0000002f
IDT=     003280e0 000007ff
CR0=80000011 CR2=00000000 CR3=00327000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000084 CCD=ffffffc0 CCO=EFLAGS
EFER=0000000000000000
Servicing hardware INT=0x20
13: v=20 e=0000 i=0 cpl=0 IP=0008:00103151 pc=00103151 SP=0010:00112c90 env->regs[R_EAX]=ffffff50
EAX=ffffff50 EBX=00112ef8 ECX=00112ef8 EDX=000001f7
ESI=000008ec EDI=0000001e EBP=0000001f ESP=00112c90
EIP=00103151 EFL=00200286 [--S--P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 00328000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     00328080 0000002f
IDT=     003280e0 000007ff
CR0=80000011 CR2=00000000 CR3=00327000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000084 CCD=ffffffc0 CCO=EFLAGS
EFER=0000000000000000
Servicing hardware INT=0x20
14: v=20 e=0000 i=0 cpl=0 IP=0008:00103151 pc=00103151 SP=0010:00112c90 env->regs[R_EAX]=ffffff50
EAX=ffffff50 EBX=00112ef8 ECX=00112ef8 EDX=000001f7
ESI=00001a14 EDI=00000015 EBP=00000016 ESP=00112c90
EIP=00103151 EFL=00200286 [--S--P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 00328000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     00328080 0000002f
IDT=     003280e0 000007ff
CR0=80000011 CR2=00000000 CR3=00327000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000084 CCD=ffffffc0 CCO=EFLAGS
EFER=0000000000000000
Servicing hardware INT=0x20
15: v=20 e=0000 i=0 cpl=0 IP=0008:00103151 pc=00103151 SP=0010:00112c90 env->regs[R_EAX]=ffffff50
EAX=ffffff50 EBX=00112ef8 ECX=00112ef8 EDX=000001f7
ESI=000024a4 EDI=00000010 EBP=00000011 ESP=00112c90
EIP=00103151 EFL=00200286 [--S--P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 00328000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     00328080 0000002f
IDT=     003280e0 000007ff
CR0=80000011 CR2=00000000 CR3=00327000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000084 CCD=ffffffc0 CCO=EFLAGS
EFER=0000000000000000
Servicing hardware INT=0x20
16: v=20 e=0000 i=0 cpl=0 IP=0008:00103139 pc=00103139 SP=0010:00112c90 env->regs[R_EAX]=00000050
EAX=00000050 EBX=00112ef8 ECX=00112ef8 EDX=000001f7
ESI=00002ccc EDI=0000000c EBP=0000000d ESP=00112c90
EIP=00103139 EFL=00200282 [--S----] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 00328000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     00328080 0000002f
IDT=     003280e0 000007ff
CR0=80000011 CR2=00000000 CR3=00327000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000080 CCD=000000d0 CCO=EFLAGS
EFER=0000000000000000
Servicing hardware INT=0x20
17: v=20 e=0000 i=0 cpl=0 IP=0008:00103069 pc=00103069 SP=0010:00112f80 env->regs[R_EAX]=00000058
EAX=00000058 EBX=00000824 ECX=000001f7 EDX=000001f7
ESI=00000000 EDI=00003400 EBP=00000824 ESP=00112f80
EIP=00103069 EFL=00200282 [--S----] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 00328000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     00328080 0000002f
IDT=     003280e0 000007ff
CR0=80000011 CR2=00000000 CR3=00327000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000080 CCD=000000d0 CCO=EFLAGS
EFER=0000000000000000
18: v=80 e=0000 i=1 cpl=3 IP=001b:08049202 pc=08049202 SP=0023:083ffcdc env->regs[R_EAX]=00000002
EAX=00000002 EBX=083ffd41 ECX=00000000 EDX=00000000
ESI=00000000 EDI=00000000 EBP=083ffcf8 ESP=083ffcdc
EIP=08049202 EFL=00000202 [-------] CPL=3 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =001b 00000000 ffffffff 00cffa00 DPL=3 CS32 [-R-]
SS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
DS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
FS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
GS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 00328000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     00328080 0000002f
IDT=     003280e0 000007ff
CR0=80000011 CR2=00000000 CR3=00327000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000010 CCD=083ffcdc CCO=SUBL
EFER=0000000000000000
Servicing hardware INT=0x20
19: v=20 e=0000 i=0 cpl=3 IP=001b:08049204 pc=08049204 SP=0023:083ffcdc env->regs[R_EAX]=00000000
EAX=00000000 EBX=083ffd41 ECX=00000000 EDX=00000000
ESI=00000000 EDI=00000000 EBP=083ffcf8 ESP=083ffcdc
EIP=08049204 EFL=00000202 [-------] CPL=3 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =001b 00000000 ffffffff 00cffa00 DPL=3 CS32 [-R-]
SS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
DS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
FS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
GS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 00328000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     00328080 0000002f
IDT=     003280e0 000007ff
CR0=80000011 CR2=00000000 CR3=00327000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000000 CCD=c0007aa8 CCO=EFLAGS
EFER=0000000000000000
^Cqemu-system-i386: terminating on signal 2 from pid 4810 (<unknown process>)
```

---
src/kernel/arch/x86/task_switch.S
```asm
; [目的] 進程上下文切換 (Context Switch) 與位址空間切換
; [流程 - switch_task(old_esp, new_esp, next_cr3)]
;
;    Task A Stack (Kernel)             Task B Stack (Kernel)
;    +-------------------+             +-------------------+
;    | 保存 A 現場        |             |                   |
;    | (pusha, pushf)    |             |                   |
;    | [esp] -> old_esp  |             |                   |
;    +---------|---------+             +---------|---------+
;              |                                           A
;              |      [ 切換 ESP & CR3 ]                   |
;              +---------------------------------+
;              |--> mov edx, [esp+48] (取得 next_cr3)
;              |--> mov esp, [next_esp] (換堆疊)
;              |--> mov cr3, edx (換位址空間)
;                                                |
;                                      +---------V---------+
;                                      | 恢復 B 現場        |
;                                      | (popf, popa)      |
;                                      +-------------------+
[bits 32]

global switch_task
global child_ret_stub

; =========================================================
; 常數定義 (Magic Numbers 解釋)
; 堆疊偏移量計算：
; pusha (32 bytes) + pushf (4 bytes) + Return EIP (4 bytes) = 40 bytes
; 因此參數在堆疊中的偏移為：
; +40 = 第一個參數 (current_esp)
; +44 = 第二個參數 (next_esp)
; +48 = 第三個參數 (next_cr3)
; =========================================================
OFFSET_CURRENT_ESP equ 40
OFFSET_NEXT_ESP    equ 44
OFFSET_NEXT_CR3    equ 48
USER_DATA_SEG      equ 0x23

; extern void switch_task(uint32_t *current_esp, uint32_t *next_esp, uint32_t next_cr3);
switch_task:
    pusha
    pushf

    ; =========================================================
    ; 【關鍵修復：拯救 CR3】
    ; 必須在修改 ESP「之前」，先從舊堆疊拿出第三個參數 next_cr3
    ; 我們把它暫存在 EDX 裡 (稍後 popa 會從新堆疊覆蓋掉 EDX，非常安全)
    ; =========================================================
    mov edx, [esp + OFFSET_NEXT_CR3]

    ; 保存 current_esp
    mov eax, [esp + OFFSET_CURRENT_ESP]
    mov [eax], esp

    ; 載入 next_esp (這一步之後，我們就正式跨入新任務的堆疊了！)
    mov eax, [esp + OFFSET_NEXT_ESP]
    mov esp, [eax]

    ; 【跨越宇宙】堆疊切換完成，現在將剛才暫存的 CR3 寫入硬體！
    mov cr3, edx

    popf
    popa

    ret

; ==========================================
; 【終極降落傘】純組合語言實作，保證絕不弄髒暫存器！
; ==========================================
child_ret_stub:
    ; 剛從 switch_task 的 ret 跳過來。
    ; 此時 ESP 完美指向我們在 C 語言壓入的 edi！

    ; 1. 精準彈出我們手工對齊的 4 個暫存器
    pop edi
    pop esi
    pop ebx
    pop ebp       ; 這裡拿回了完美修正過的 EBP！

    ; 2. 換上平民服裝 (User Data Segment = 0x23)
    mov cx, USER_DATA_SEG
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx

    mov eax, 0    ; 【核心魔法】子行程拿到回傳值 0！

    ; 3. 完美降落！
    ; 此刻 ESP 剛好指著我們手工準備的 IRET 幀 (EIP, CS, EFLAGS, ESP, SS)
    iret
```

---
src/kernel/arch/x86/interrupts.S
```asm
; [interrupts.S] - 中斷與異常的低階處理跳板
;
; 核心任務：
; 1. 提供統一的中斷進入點。
; 2. 處理「錯誤碼」的一致性 (x86 CPU 有些中斷會自動推入錯誤碼，有些不會)。
; 3. 備份全數通用暫存器 (pusha)，確保 C 處理完後能完整恢復現場。

global idt_flush
global isr0, isr14, isr32, isr33, isr43, isr44, isr128

; 引用 C 語言實作的處理器
extern isr0_handler, page_fault_handler, timer_handler
extern keyboard_handler, rtl8139_handler, mouse_handler, syscall_handler

idt_flush:
    mov eax, [esp+4]
    lidt [eax]
    ret

; -----------------------------------------------------------------------------
; 模式 A: 無錯誤碼的中斷 (大部分中斷)
; -----------------------------------------------------------------------------
; 我們手動 push 0 作為「假錯誤碼」，確保 registers_t 結構在各中斷中都能通用。

isr0:
    cli
    push 0          ; 壓入假錯誤碼
    call isr0_handler
    add esp, 4      ; 清除假錯誤碼
    sti
    iret

isr32:
    push 0          ; 假錯誤碼
    pusha           ; 備份暫存器 (eax, ecx, edx, ebx, esp, ebp, esi, edi)
                    ; 保存現場：CPU 本來在執行的程式暫存器會被存入堆疊，以免被中斷處理程式破壞。
    call timer_handler  ; 呼叫 C 語言, 執行 call timer_handler
    popa            ; 恢復暫存器
    add esp, 4      ; 清除假錯誤碼
    iret

isr33:
    push 0
    pusha
    call keyboard_handler
    popa
    add esp, 4
    iret

isr43:
    push 0
    pusha
    call rtl8139_handler
    popa
    add esp, 4
    iret

isr44:
    push 0
    pusha
    call mouse_handler
    popa
    add esp, 4
    iret

; -----------------------------------------------------------------------------
; 模式 B: 有錯誤碼的異常 (例如 Page Fault)
; -----------------------------------------------------------------------------
; CPU 會在執行 ISR 前自動壓入一個 32-bit 的 Error Code。

isr14:
    ; 此時堆疊已有 Error Code
    pusha           ; 備份暫存器
    push esp        ; 傳遞 registers_t 指標給 C
    call page_fault_handler
    add esp, 4      ; 清除參數
    popa            ; 恢復暫存器
    add esp, 4      ; 【關鍵】手動清除 CPU 壓入的真實 Error Code！
    iret

; -----------------------------------------------------------------------------
; 模式 C: 系統呼叫 (System Call)
; -----------------------------------------------------------------------------
isr128:
    push 0          ; 假錯誤碼，對齊 registers_t
    pusha           ; 備份現場
    push esp        ; 傳遞指標
    call syscall_handler
    add esp, 4      ; 清除指標參數
    ; 注意：syscall_handler 通常會直接修改暫存器備份 (如 regs->eax) 作為傳回值
    popa            ; 恢復現場 (含已修改的 eax)
    add esp, 4      ; 清除假錯誤碼
    iret
```

---
src/kernel/arch/x86/timer.c
```c
#include <stddef.h>
#include "timer.h"
#include "io.h"
#include "utils.h"
#include "task.h"
#include "gui.h"

// 計時器晶片 8254 Programmable Interval Timer (PIT)
// 負責產生的規律電子訊號，他的基礎頻率是 1.19318 MHz (1,193,180 Hz)，
// 頻率則透過石英振蕩器生成 (Crystal Oscillator)
//
//     All PC compatibles operate the PIT at a clock rate of 105/88 = 1.19318 MHz
//
// 1,193,180 這個數字緣由是彩色電視 NTSC 的副載波頻率（14.31818 MHz）經過 12 分頻計算得來。
// 在電子工程中，「分頻 (Frequency Division)」指的是將一個高頻率的訊號，
// 轉換成較低頻率訊號的過程。簡單說「12 分頻」就是把原始頻率除以 12。
//
//     14.31818 MHz / 12 = 1,193,181 Hz
//
// 作業系統約定成俗的會使用 1,193,180 或 1,193,182 當常數
//
// 80 年代（IBM PC 誕生初期），石英震盪器（Oscillator）是很貴的零件。
// 工程師為了省錢，不想在主機板上插滿各種不同頻率的石英鐘，於是採用了「一魚多吃」
//
// - 主頻源：只買一個最便宜、大量生產的 14.31818 MHz 石英震盪器（當時因為彩色電視普及，這種零件極度便宜）。
// - 給顯卡用：這個頻率直接給彩色顯示卡使用，因為它剛好是 NTSC 電視訊號的 4 倍。
// - 給 CPU 用：經過 3 分頻 (14.318 / 3 = 4.77 MHz)，這就是初代 IBM PC 處理器的時脈。
// - 給計時器用：經過 12 分頻 (14.318 / 12 = 1.19 MHz)，這就是 8254 PIT 晶片拿到的工作頻率。
//
// @see: INTEL 8253/8254: https://en.wikipedia.org/wiki/Intel_8253
// @see: https://en.wikipedia.org/wiki/Crystal_oscillator
#define PIT_CLOCK_RATE 1193180

// tick (狀聲詞, 滴答聲)，用來記錄 作業系統 的節拍數，從開機就開始計算。
//
// 32bit 整數，在 100Hz 下頻率約 497 天會溢位。
//
// volatile 是告訴編譯器：「這個變數的值可能會在程式流程之外（被硬體中斷）改變」。
// 如果不加 volatile，編譯器可能會為了優化而把 tick 的值存在暫存器裡，導致其他程式碼讀到舊的值。
volatile uint32_t tick = 0;

// @param frequency: 每秒觸發中斷的次數
// 如果設定為 100Hz，每個 tick 的時間長就是 10ms；若改為 1000Hz，則可以精確到 1ms。
//
// 在 main.c 裡面的初始流程設定為 100
void init_timer(uint32_t frequency) {

    // divisor 表示晶片數 N 次，就發出一次中斷。
    //
    // 用 frequency 當基數，如果想要每秒中斷 100 次，frequency=100，
    // 則晶片每數 (PIT8254_BASE_FREQ / frequency = 11931 次訊號，
    // 就發出一次中斷。
    uint32_t divisor = PIT_CLOCK_RATE / frequency;

    // x43: PIT 的 模式控制暫存器 埠號。
    // 0x36 (00110110b): 這是控制字組（Control Word）。
    //  - 00: 選擇通道 0（接在 IRQ0，負責系統時鐘）。
    //  - 11: 存取模式 (LOBYTE/HIBYTE)，先寫低八位元，再寫高八位元
    //  - 011: 選擇模式 3（方波產生器，最適合當作時鐘）。
    //         它會自動重新加載計數器，適合產生週期性心跳。
    outb(0x43, 0x36);

    // 取得 divisor 的低 8 位元 (low byte) 和 高八位元 (high byte)
    uint8_t l = (uint8_t)(divisor & 0xFF);
    uint8_t h = (uint8_t)((divisor >> 8) & 0xFF);

    // 把 divisor 塞到 8254 暫存器, 但 8254 是 8bit 匯流排
    // 所以分成兩次, 把高低位原 依序「餵」進去
    //
    // 0x40: 這是通道 0 的 資料暫存器
    outb(0x40, l);
    outb(0x40, h);
}

// 中斷發生後的處理流程，由組合語言 interrupts.S#isr32 調用
//
// [Notes] 這段類似於 Game Loop 中的 Update(),
// 差別在於 GameLoop 是由 Game Engine 控制 (或我自己寫的 `while(true) {}`)
// OS 則由 PIT 晶片控制，透過中斷的方式製造出 callback 效果。
//
// @see: Game Loop https://github.com/rickhw/java-2d-rpg-game/blob/main/src/main/java/gtcafe/rpg/GamePanel.java#L217
// @see: Update() https://github.com/rickhw/java-2d-rpg-game/blob/main/src/main/java/gtcafe/rpg/GamePanel.java#L274
void timer_handler(void) {
    // 這裡的 tick++ 就像是 OS 的「虛擬時間」步進。
    // 在 x86 指令中，這是一個原子操作或受中斷保護的操作，
    // 確保所有依賴時間的 Syscall (如 sleep) 都有統一的參考系。
    //
    // [Notes]
    // 定義作業系統的節奏感，每次的累加，都代表著作業系統時間的流逝
    //
    // 概念類似於 Game Loop 裡的 FPS。但不同的是，
    // Game Loop 會因為 Update 處理的效能 (通常是 2D/3D 圖形渲染)
    // 因而影響 FPS 的大小。
    //
    // 作業系統則不能因為 task 處理慢了，tick 就停下來。
    tick++;

    // CPU Accounting (資源統計)
    // 計算 CPU 使用率核心邏輯，現在是誰在用 CPU，
    // 這個滴答的功勞就記在誰頭上！
    //
    // 作業系統無法無時無刻監控 CPU 資源。
    // 採用「抽樣法」——當中斷發生時，誰在位置上，
    // 這過去的 10ms 算費就記在誰頭上。
    //
    // 這是 top 指令顯示 CPU 佔用率的數據來源。
    if (current_task != 0) {
        current_task->total_ticks++;
    }

    // 每一滴答 (Tick)，檢查檢查滑鼠有沒有動、視窗是否需要重繪。
    // 這保證了 UI 的反應速度與 Tick 頻率同步。
    //
    // [Notes]
    // 這段呈現出經典的 Windows 95 搖動滑鼠會改善效率的問題
    // @see: https://www.techbang.com/posts/71423-this-theory-suggests-wiggling-the-mouse-cursor-actually-did-make-windows-95-run-faster
    gui_handle_timer();

    // @term: Programmable Interrupt Control (PIC), 中斷控制器, 通常是 INTEL 8259 這顆晶片
    // @term: End of Interrupt (EOI)
    // @purpose: OS 告訴 PIC
    //
    //     我處理完了，可以發下一個中斷給我了。
    //
    // PIC 介於所有硬體與 CPU 之間的「通訊官」，他像是一個警衛，他發出中斷後會擋住後續的所有訊號。
    // 如果 OS 不回覆 EOI，警衛會以為你還在忙，從此以後再也不會發出任何中斷訊號給 CPU，
    // 系統就會看起來像是「當機」了。
    //
    // 因為 schedule() 會導致上下文切換 (Context Switch)，
    // 如果不送 EOI 指令，CPU 就會被鎖死，再也不會收到下一個 tick。
    //
    // @param: 0x20 (埠號): 這是 Master PIC (中斷控制器) 的命令暫存器。
    // @value: 0x20 (數值): 這是 EOI (End of Interrupt) 指令。
    //
    // @see: INTEL 8259 https://en.wikipedia.org/wiki/Intel_8259
    outb(0x20, 0x20);

    // 實踐 搶佔式多工 (Preemptive Multitasking)，強行切換任務
    // 強制在 timer_handler 結束前切換到另一個任務，
    // 達成「每個程式都在同時執行」的假象。
    schedule();
}
```
---
src/kernel/include/timer.h
```c
#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

// extern volatile uint32_t tick;

void init_timer(uint32_t frequency);
void timer_handler(void);

#endif
```

---
src/kernel/kernel/task.h
```c
#ifndef TASK_H
#define TASK_H
#include <stdint.h>

// ==========================================
// 記憶體分佈與權限常數定義 (Memory Layout & Privileges)
// ==========================================
#define PAGE_SIZE           4096
#define USER_HEAP_START     0x10000000
#define USER_HEAP_PAGES     256
#define USER_STACK_TOP      0x083FF000
#define USER_STACK_PADDING  64
#define USER_ARGS_LIMIT     0x08000000
#define MAX_EXEC_ARGS       16

// 段選擇子 (Segment Selectors)
#define USER_DS             0x23
#define USER_CS             0x1B
#define KERNEL_DS           0x10
#define KERNEL_CS           0x08
#define EFLAGS_DEFAULT      0x0202

#define KERNEL_IDLE_PID     9999

/** 行程狀態定義 */
#define TASK_RUNNING 0
#define TASK_DEAD    1
#define TASK_WAITING 2
#define TASK_ZOMBIE  3  ///< 行程已結束，但父行程尚未 wait() 回收它
#define TASK_SLEEPING 4 ///< 睡眠中

/**
 * @brief 中斷上下文快照 (Interrupt Context Snapshot)
 * * 此結構體的佈局必須嚴格遵循 x86 中斷發生時的堆疊增長順序。
 * 記憶體佈局 (由低位址往高位址)：
 * [低] edi -> esi -> ebp -> esp -> ebx -> edx -> ecx -> eax -> eip -> cs -> eflags -> user_esp -> user_ss [高]
 */
typedef struct {
    /* 由 pusha 指令壓入的通用暫存器 (順序由後往前) */
    uint32_t edi;    ///< Destination Index: 常用於記憶體拷貝目的地
    uint32_t esi;    ///< Source Index: 常用於記憶體拷貝來源
    uint32_t ebp;    ///< Base Pointer: 堆疊基底指標，用於 Stack Trace
    uint32_t esp;    ///< Stack Pointer: 核心模式下的堆疊頂端 (pusha 壓入的值)
    uint32_t ebx;    ///< Base Register: 常用於存取資料段位址
    uint32_t edx;    ///< Data Register: 常用於 I/O 埠操作或乘除法
    uint32_t ecx;    ///< Counter Register: 常用於迴圈計數
    uint32_t eax;    ///< Accumulator Register: 函式回傳值或 Syscall 編號

    /* 由 CPU 在某些例外發生時自動壓入的錯誤碼 (一般中斷手動補 0) */
    uint32_t err_code;

    /* 以下由 CPU 在發生中斷時「自動」壓入堆疊 (Interrupt Stack Frame) */
    uint32_t eip;    ///< Instruction Pointer: 程式被中斷時的下一條指令位址
    uint32_t cs;     ///< Code Segment: 包含權限等級 (CPL)，決定是 Ring 0 還是 3
    uint32_t eflags; ///< CPU Flags: 包含中斷開關 (IF) 與運算狀態標誌

    /* 以下兩項僅在「權限切換」(Ring 3 -> Ring 0) 時由 CPU 自動壓入 */
    uint32_t user_esp; ///< User Stack Pointer: 使用者原本的堆疊位址
    uint32_t user_ss;  ///< User Stack Segment: 使用者原本的堆疊段選擇子 (通常 0x23)
} registers_t;

/**
 * @brief 任務控制區塊 (Process Control Block, PCB)
 */
typedef struct task {
    uint32_t esp;               ///< 核心堆疊指標

    // === 行程身分資訊 ===
    uint32_t pid;               ///< 行程 ID
    uint32_t ppid;              ///< 父行程 ID
    char name[32];              ///< 行程名稱

    uint32_t total_ticks;       ///< 行程總執行時間 (Ticks)

    uint32_t page_directory;    ///< 專屬的分頁目錄實體位址 (CR3)
    uint32_t kernel_stack;      ///< 核心堆疊頂部

    uint32_t state;             ///< 行程當前狀態
    uint32_t wait_pid;          ///< 正在等待的子行程 PID

    uint32_t heap_start;        ///< Heap 初始起點
    uint32_t heap_end;          ///< User Heap 的當前頂點
    uint32_t cwd_lba;           ///< 當前工作目錄 (CWD) 的 LBA

    uint32_t wake_up_tick;      /// for sleep, 預計醒來的 tick 數

    struct task *next;          ///< 環狀鏈結串列指標
} task_t;

/**
 * @brief 用於傳遞給 User Space 的行程資訊
 */
typedef struct {
    uint32_t pid;
    uint32_t ppid;
    char name[32];
    uint32_t state;
    uint32_t memory_used; ///< 佔用記憶體大小 (Bytes)
    uint32_t total_ticks; ///< 總執行時間
} process_info_t;

extern volatile task_t *current_task;

/** @brief 初始化多工排程子系統 */
void init_multitasking();

/**
 * @brief 建立第一個 User Task
 * @param entry_point 程式進入點
 * @param user_stack_top User Stack 頂部
 */
void create_user_task(uint32_t entry_point, uint32_t user_stack_top);

/** @brief 結束當前行程 */
void exit_task();

/** @brief 排程器 (Round Robin) 切換下一個行程 */
void schedule();

/**
 * @brief 複製當前行程 (Fork)
 * @param regs 當前的暫存器狀態
 * @return 子行程回傳 0，父行程回傳子行程的 PID，失敗回傳 -1
 */
int sys_fork(registers_t *regs);

/**
 * @brief 替換當前行程的執行檔影像 (Exec)
 * @param regs 包含新執行檔名稱與參數的暫存器
 * @return 成功不回傳，失敗回傳 -1
 */
int sys_exec(registers_t *regs);

/**
 * @brief 等待子行程結束
 * @param pid 要等待的子行程 PID
 * @return 成功回傳 0，失敗回傳 -1
 */
int sys_wait(uint32_t pid);

/**
 * @brief 強制終止行程
 * @param pid 要終止的行程 PID
 * @return 成功回傳 0，找不到行程回傳 -1
 */
int sys_kill(uint32_t pid);

int sys_sleep(uint32_t ms);

/**
 * @brief 取得系統中所有活躍的行程清單
 * @param list 用於儲存結果的陣列
 * @param max_count 陣列最大容量
 * @return 實際取回的行程數量
 */
int task_get_process_list(process_info_t* list, int max_count);

#endif

```

---
src/kernel/kernel/task.c
```c
/**
 * @file src/kernel/kernel/task.c
 * @brief 核心多工任務管理系統 (Kernel Multitasking System)
 *
 * 本檔案負責處理進程控制區塊 (PCB) 的管理、任務排程 (Scheduling)、
 * 垃圾回收 (GC) 以及系統呼叫如 fork, exec, wait 的核心實作。
 */

#include <stdint.h>
#include "task.h"
#include "pmm.h"
#include "kheap.h"
#include "utils.h"
#include "tty.h"
// #include "timer.h"
#include "gdt.h"
#include "paging.h"
#include "simplefs.h"
#include "vfs.h"
#include "elf.h"
#include "gui.h"

/** @brief 當前正在執行的任務指標 */
volatile task_t *current_task = 0;
/** @brief 排程器準備佇列的起點 (目前採環狀鏈結串列) */
volatile task_t *ready_queue = 0;
/** @brief 下一個可分配的 PID */
uint32_t next_pid = 0;
/** @brief 閒置任務 (Idle Task)，當系統無事可做時執行 */
task_t *idle_task = 0;

extern uint32_t page_directory[];
extern void switch_task(uint32_t *current_esp, uint32_t *next_esp, uint32_t next_cr3);
extern void child_ret_stub();
extern volatile uint32_t tick;

/**
 * @brief 根據 PID 尋找特定的任務控制區塊
 * @param pid 目標 PID
 * @return 找到回傳 task_t 指標，否則回傳 NULL
 */
static task_t* get_task_by_pid(uint32_t pid) {
    if (!current_task) return 0;
    task_t *start = (task_t*)current_task;
    task_t *temp = start;
    do {
        if (temp->pid == pid) return temp;
        temp = temp->next;
    } while (temp != start);
    return 0;
}

/**
 * @brief 垃圾回收器 (Task Garbage Collector)
 *
 * 掃描任務鏈結串列，尋找標記為 TASK_DEAD 的任務，並釋放其佔用的
 * 核心堆疊 (Kernel Stack) 與 PCB 記憶體空間。
 */
static void task_gc() {
    if (!current_task) return;

    task_t *prev = (task_t*)current_task;
    task_t *curr = prev->next;

    while (curr != current_task) {
        if (curr->state == TASK_DEAD) {
            // 從環狀鏈結串列中移除
            prev->next = curr->next;

            // 釋放核心堆疊 (kfree 回到分配時的起始點)
            if (curr->kernel_stack != 0) {
                kfree((void*)(curr->kernel_stack - PAGE_SIZE));
            }
            // 釋放 PCB 結構
            kfree((void*)curr);

            curr = prev->next;
        } else {
            prev = curr;
            curr = curr->next;
        }
    }
}

/**
 * @brief 閒置任務迴圈
 * 當系統中沒有任何可執行的任務時，CPU 將在此執行 hlt (休眠) 節省電力，
 * 直到下一次時脈中斷觸發排程。
 */
void idle_loop() {
    while(1) { __asm__ volatile("sti; hlt"); }
}

/**
 * @brief 初始化多工系統
 *
 * 將目前的執行流包裝成 PID 0 (kernel_main)，並建立閒置任務。
 */
void init_multitasking() {
    // ----------------------------------------------------
    // 1. 初始化 Kernel 主任務 (PID 0)
    // ----------------------------------------------------
    task_t *main_task = (task_t*) kmalloc(sizeof(task_t));
    main_task->pid = next_pid++;
    main_task->ppid = 0;
    strcpy(main_task->name, "[kernel_main]");

    main_task->esp = 0; // 初始值，將在第一次切換時由 ASM 寫入
    main_task->kernel_stack = 0;
    main_task->state = TASK_RUNNING;
    main_task->wait_pid = 0;
    main_task->page_directory = (uint32_t) page_directory;
    main_task->cwd_lba = 0;
    main_task->total_ticks = 0;

    // 建立環狀串列
    main_task->next = main_task;
    current_task = main_task;
    ready_queue = main_task;

    // ----------------------------------------------------
    // 2. 建立 Idle Task (PID 9999)
    // ----------------------------------------------------
    idle_task = (task_t*) kmalloc(sizeof(task_t));
    idle_task->pid = KERNEL_IDLE_PID;
    idle_task->ppid = 0;
    strcpy(idle_task->name, "[kernel_idle]");
    idle_task->state = TASK_RUNNING;
    idle_task->page_directory = (uint32_t) page_directory;

    // 為 Idle Task 配置獨立核心堆疊
    uint32_t *kstack_mem = (uint32_t*) kmalloc(PAGE_SIZE);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + PAGE_SIZE);
    idle_task->kernel_stack = (uint32_t) kstack;

    // 偽造中斷堆疊幀，模擬該任務曾被中斷
    *(--kstack) = (uint32_t) idle_loop; // EIP: 跳轉到閒置迴圈
    for(int i = 0; i < 8; i++) *(--kstack) = 0; // Pusha (暫存器清零)
    *(--kstack) = EFLAGS_DEFAULT;       // EFLAGS: 開啟中斷 (IF=1)

    idle_task->esp = (uint32_t) kstack;
}

/**
 * @brief 建立使用者模式任務
 * @param entry_point 程式起始位址
 * @param user_stack_top 使用者堆疊頂部位址
 *
 * 此函式負責建立一個 Ring 3 任務，並手動佈置堆疊以確保 IRET 後能切換到 User Mode。
 */
void create_user_task(uint32_t entry_point, uint32_t user_stack_top) {
    task_t *new_task = (task_t*) kmalloc(sizeof(task_t));
    new_task->pid = next_pid++;
    new_task->ppid = current_task->pid;
    strcpy(new_task->name, "init");

    new_task->state = TASK_RUNNING;
    new_task->page_directory = current_task->page_directory;
    new_task->cwd_lba = 0;

    // ----------------------------------------------------
    // 1. 初始化使用者堆積 (User Heap)
    // ----------------------------------------------------
    for (int i = 0; i < USER_HEAP_PAGES; i++) {
        uint32_t heap_phys = (uint32_t)pmm_alloc_page();
        map_page(USER_HEAP_START + (i * PAGE_SIZE), heap_phys, 7); // User, RW, Present
    }
    new_task->heap_start = USER_HEAP_START;
    new_task->heap_end = USER_HEAP_START;

    // ----------------------------------------------------
    // 2. 佈置使用者堆疊與核心堆疊
    // ----------------------------------------------------
    uint32_t *kstack_mem = (uint32_t*) kmalloc(PAGE_SIZE);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + PAGE_SIZE);
    new_task->kernel_stack = (uint32_t) kstack;

    // 模擬 C main(argc, argv) 參數佈局
    uint32_t *ustack = (uint32_t*) (user_stack_top - USER_STACK_PADDING);
    *(--ustack) = 0;         // NULL argv 結尾
    uint32_t argv_ptr = (uint32_t) ustack;
    *(--ustack) = argv_ptr;  // argv
    *(--ustack) = 0;         // argc
    *(--ustack) = 0;         // Dummy return address

    // ----------------------------------------------------
    // 3. 偽造 IRET 中斷幀 (切換 Ring 0 -> Ring 3)
    // ----------------------------------------------------
    *(--kstack) = USER_DS;          // User SS
    *(--kstack) = (uint32_t)ustack; // User ESP
    *(--kstack) = EFLAGS_DEFAULT;   // EFLAGS
    *(--kstack) = USER_CS;          // User CS
    *(--kstack) = entry_point;      // User EIP

    // 偽造 child_ret_stub 彈出的暫存器
    *(--kstack) = 0; // EBP
    *(--kstack) = 0; // EBX
    *(--kstack) = 0; // ESI
    *(--kstack) = 0; // EDI

    *(--kstack) = (uint32_t) child_ret_stub; // 返回位址

    // 為 switch_task 的彈出指令留空位
    for(int i = 0; i < 8; i++) *(--kstack) = 0;
    *(--kstack) = EFLAGS_DEFAULT;

    new_task->esp = (uint32_t) kstack;

    // 加入排程鏈結串列
    new_task->next = current_task->next;
    current_task->next = new_task;
}

void exit_task() {
    // 1. 關閉該行程創建的所有圖形介面視窗
    gui_close_windows_by_pid(current_task->pid);

    task_t *temp = current_task->next;
    int parent_waiting = 0;

    // 2. 喚醒可能正在 wait() 這個行程的父行程
    while (temp != current_task) {
        if (temp->state == TASK_WAITING && temp->wait_pid == current_task->pid) {
            temp->state = TASK_RUNNING;
            temp->wait_pid = 0;
            parent_waiting = 1;
        }
        temp = temp->next;
    }

    // 3. 釋放該行程專屬的分頁目錄 (如果是 Kernel 就不能清掉)
    if (current_task->page_directory != (uint32_t)page_directory) {
        free_page_directory(current_task->page_directory);
    }

    // 4. 決定行程的最終狀態
    // 如果父行程有在等待 (wait)，則變成 ZOMBIE 讓父行程來收屍並拿取退出碼
    // 否則直接標記為 DEAD，排程器稍後會直接回收記憶體
    if (current_task->ppid == 0 || !parent_waiting) {
        current_task->state = TASK_DEAD;
    } else {
        current_task->state = TASK_ZOMBIE;
    }

    // 讓出 CPU
    schedule();
}

void schedule() {
    if (!current_task) return;

    // 1. 保存進入前的 EFLAGS (包含中斷開關狀態)
    uint32_t eflags;
    __asm__ volatile("pushfl; popl %0" : "=r"(eflags));

    // 2. 核心操作期間關閉中斷
    __asm__ volatile("cli");

    // ----------------------------------------------------
    // 階段 1：垃圾回收 (Garbage Collection)
    // ----------------------------------------------------
    task_gc();

    // 階段 1.5：檢查是否有正在睡眠的任務可以喚醒
    if (ready_queue) {
        task_t *iter = (task_t*)ready_queue;
        task_t *first = iter;
        do {
            if (iter->state == TASK_SLEEPING && tick >= iter->wake_up_tick) {
                iter->state = TASK_RUNNING;
                iter->wake_up_tick = 0;
            }
            iter = iter->next;
        } while (iter != first);
    }

    // ----------------------------------------------------
    // 階段 2：尋找下一個準備就緒 (RUNNING) 的行程
    // ----------------------------------------------------
    task_t *next_run = current_task->next;
    while (next_run->state != TASK_RUNNING && next_run != current_task) {
        next_run = next_run->next;
    }

    // 如果繞了一圈發現大家都還在等 (Waiting/Zombie)，那就派 idle_task 上場
    if (next_run->state != TASK_RUNNING) {
        next_run = idle_task;
    }

    // ----------------------------------------------------
    // 階段 3：執行 Context Switch
    // ----------------------------------------------------
    task_t *prev = (task_t*)current_task;
    current_task = next_run;

    if (current_task != prev) {
        // 更新 TSS 中的 ESP0，確保下次發生中斷時，能使用正確的 Kernel Stack
        if (current_task->kernel_stack != 0) {
            set_kernel_stack(current_task->kernel_stack);
        }
        // 切換堆疊與位址空間 (CR3)
        switch_task((uint32_t*)&prev->esp, (uint32_t*)&current_task->esp, current_task->page_directory);
    }

    // // 當 switch_task 返回時，我們已經在「新任務」的環境中了
    // // 新任務可能是在不同地方被中斷的，我們開啟中斷讓系統繼續跳動
    // __asm__ volatile("sti");

    // 關鍵：恢復原本的中斷狀態，而不是盲目 sti
    // __asm__ volatile("push %0; popf" : : "r"(eflags));

    // 關鍵修復：返回前確保中斷是開啟的，否則主動呼叫 schedule 的人會卡死
    // __asm__ volatile("sti");

    // 3. 【關鍵修復】恢復進入前的狀態，而不是盲目 sti
    // 如果是從 timer_handler 進來的，這裡會保持 cli 直到退出中斷
    // 如果是從 sys_sleep 進來的，這裡會恢復為原本 syscall 開啟中斷的狀態
    __asm__ volatile("pushl %0; popfl" : : "r"(eflags));
}

int sys_fork(registers_t *regs) {
    // ----------------------------------------------------
    // 階段 1：複製父行程的 PCB
    // ----------------------------------------------------
    task_t *child = (task_t*) kmalloc(sizeof(task_t));
    child->pid = next_pid++;
    child->ppid = current_task->pid;
    strcpy(child->name, (const char*)current_task->name);

    child->state = TASK_RUNNING;
    child->wait_pid = 0;

    // 注意：這裡目前是共享 Page Directory (未來若實作寫入時複製 COW 會在這裡修改)
    child->page_directory = current_task->page_directory;
    child->cwd_lba = current_task->cwd_lba;

    child->heap_start = current_task->heap_start;
    child->heap_end = current_task->heap_end;
    child->total_ticks = 0; // 子行程的執行時間重新計算

    // 為子行程準備獨立的 Kernel Stack
    uint32_t *kstack_mem = (uint32_t*) kmalloc(PAGE_SIZE);
    uint32_t *kstack = (uint32_t*) ((uint32_t)kstack_mem + PAGE_SIZE);
    child->kernel_stack = (uint32_t) kstack;

    // ----------------------------------------------------
    // 階段 2：拷貝 User Stack
    // 為了避免多個行程在同一個位址空間中 Stack 碰撞，根據 PID 計算不同的 Base
    // ----------------------------------------------------
    uint32_t child_ustack_base = USER_STACK_TOP - (child->pid * PAGE_SIZE);
    uint32_t child_ustack_phys = (uint32_t)pmm_alloc_page();
    map_page(child_ustack_base, child_ustack_phys, 7);

    // 刷新 TLB (重新載入 CR3)
    uint32_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3));

    uint32_t parent_ustack_base = regs->user_esp & 0xFFFFF000;
    uint8_t *src = (uint8_t*) parent_ustack_base;
    uint8_t *dst = (uint8_t*) child_ustack_base;
    for(int i = 0; i < PAGE_SIZE; i++) { dst[i] = src[i]; }

    int32_t delta = child_ustack_base - parent_ustack_base;
    uint32_t child_user_esp = regs->user_esp + delta;

    // ----------------------------------------------------
    // [高難度操作] 修正 Stack Frame (EBP 鏈)
    // 因為 Stack 被搬移到新的記憶體位址，所以存放 "指向上一個 EBP 位址" 的數值必須全部加上位移量 (delta)
    // ----------------------------------------------------
    uint32_t child_ebp = regs->ebp;
    if (child_ebp >= parent_ustack_base && child_ebp < parent_ustack_base + PAGE_SIZE) {
        child_ebp += delta;
    }

    uint32_t curr_ebp = child_ebp;
    while (curr_ebp >= child_ustack_base && curr_ebp < child_ustack_base + PAGE_SIZE) {
        uint32_t *ebp_ptr = (uint32_t*) curr_ebp;
        uint32_t next_ebp = *ebp_ptr;
        // 如果這個 EBP 指向同一個 Stack page 內，就進行修正
        if (next_ebp >= parent_ustack_base && next_ebp < parent_ustack_base + PAGE_SIZE) {
            *ebp_ptr = next_ebp + delta;
            curr_ebp = *ebp_ptr;
        } else {
            break; // 到達底部或外部
        }
    }

    // ----------------------------------------------------
    // 階段 3：準備子行程被排程到的「第一次恢復狀態」
    // ----------------------------------------------------
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
    *(--kstack) = EFLAGS_DEFAULT;

    child->esp = (uint32_t) kstack;

    // 加入排程佇列
    child->next = current_task->next;
    current_task->next = child;

    // 父行程的 fork 回傳子行程的 PID
    return child->pid;
}

int sys_exec(registers_t *regs) {
    char* filename = (char*)regs->ebx;
    char** old_argv = (char**)regs->ecx;

    // ----------------------------------------------------
    // 階段 1：尋找並載入執行檔 (ELF)
    // ----------------------------------------------------
    uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;
    fs_node_t* file_node = simplefs_find(current_dir, filename);

    // 支援 PATH 的簡陋版：如果在當前目錄找不到，去根目錄 (/) 找
    if (file_node == 0 && current_dir != 1) {
        file_node = simplefs_find(1, filename);
    }
    if (file_node == 0) { return -1; }

    strcpy((char*)current_task->name, filename);

    uint8_t* buffer = (uint8_t*) kmalloc(file_node->length);
    vfs_read(file_node, 0, file_node->length, buffer);

    // ----------------------------------------------------
    // 階段 2：保存命令列參數 (Arguments)
    // 因為我們即將砍掉舊的位址空間，必須先把參數字串拷貝到核心安全區
    // ----------------------------------------------------
    int argc = 0;
    char* safe_argv[MAX_EXEC_ARGS];

    if (old_argv != 0 && (uint32_t)old_argv > USER_ARGS_LIMIT) {
        while (old_argv[argc] != 0 && argc < (MAX_EXEC_ARGS - 1)) {
            int len = strlen(old_argv[argc]) + 1;
            safe_argv[argc] = (char*) kmalloc(len);
            memcpy(safe_argv[argc], old_argv[argc], len);
            argc++;
        }
    }
    safe_argv[argc] = 0;

    // ----------------------------------------------------
    // 階段 3：建立並切換至新的分頁目錄 (Page Directory)
    // ----------------------------------------------------
    uint32_t new_cr3 = create_page_directory();
    uint32_t old_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(old_cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(new_cr3)); // 切過去以便載入 ELF

    uint32_t entry_point = elf_load((elf32_ehdr_t*)buffer);
    kfree(buffer);

    if (entry_point == 0) {
        // 如果載入失敗，切回舊的空間並報錯
        __asm__ volatile("mov %0, %%cr3" : : "r"(old_cr3));
        return -1;
    }

    current_task->page_directory = new_cr3;

    // ----------------------------------------------------
    // 階段 4：為新程式佈置全新的 User Stack 與 User Heap
    // ----------------------------------------------------
    uint32_t clean_user_stack_top = USER_STACK_TOP + PAGE_SIZE;
    uint32_t ustack_phys = (uint32_t)pmm_alloc_page();
    map_page(USER_STACK_TOP, ustack_phys, 7);

    for (int i = 0; i < USER_HEAP_PAGES; i++) {
        uint32_t heap_phys = (uint32_t)pmm_alloc_page();
        map_page(USER_HEAP_START + (i * PAGE_SIZE), heap_phys, 7);
    }
    current_task->heap_start = USER_HEAP_START;
    current_task->heap_end = USER_HEAP_START;

    uint32_t stack_ptr = clean_user_stack_top - USER_STACK_PADDING;

    // ----------------------------------------------------
    // 階段 5：將剛剛保存的命令列參數 (Args) 推入新程式的 Stack 頂部
    // 佈局結構：[字串資料] -> [argv 指標陣列] -> [argv 的雙重指標] -> [argc]
    // ----------------------------------------------------
    uint32_t argv_ptrs[MAX_EXEC_ARGS] = {0};
    if (argc > 0) {
        // a. 先把字串本身推入堆疊
        for (int i = argc - 1; i >= 0; i--) {
            int len = strlen(safe_argv[i]) + 1;
            stack_ptr -= len;
            memcpy((void*)stack_ptr, safe_argv[i], len);
            argv_ptrs[i] = stack_ptr; // 記錄字串在 Stack 中的實際位址
            kfree(safe_argv[i]);      // 釋放剛才暫存的記憶體
        }

        stack_ptr = stack_ptr & ~3;   // 確保 4-byte 對齊
        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = 0;    // argv[argc] = NULL

        // b. 推入指向那些字串的指標 (argv 陣列)
        for (int i = argc - 1; i >= 0; i--) {
            stack_ptr -= 4;
            *(uint32_t*)stack_ptr = argv_ptrs[i];
        }

        uint32_t argv_base = stack_ptr;

        // c. 推入 C 語言 main(int argc, char** argv) 的參數
        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = argv_base; // arg2: char** argv
        stack_ptr -= 4;
        *(uint32_t*)stack_ptr = argc;      // arg1: int argc
    } else {
        // 沒有參數的情況
        stack_ptr -= 4; *(uint32_t*)stack_ptr = 0;
        stack_ptr -= 4; *(uint32_t*)stack_ptr = 0;
    }

    // Return address (User mode 結束程式用的，通常是 libc 的 exit)
    stack_ptr -= 4; *(uint32_t*)stack_ptr = 0;

    // ----------------------------------------------------
    // 階段 6：修改系統呼叫返回時的暫存器
    // 這樣 IRET 回去時，CPU 就會直接開始執行新程式
    // ----------------------------------------------------
    regs->eip = entry_point;
    regs->user_esp = stack_ptr;

    return 0;
}

int sys_wait(uint32_t pid) {
    task_t *temp = current_task->next;
    int found = 0;

    // 1. 確認該子行程存在，且是否已經是 ZOMBIE (已經死掉了)
    while (temp != current_task) {
        if (temp->pid == pid) {
            found = 1;
            if (temp->state == TASK_ZOMBIE) {
                // 子行程已經結束，直接幫它收屍，無須阻塞 (Block) 父行程
                temp->state = TASK_DEAD;
                return 0;
            }
            break;
        }
        temp = temp->next;
    }

    if (!found) return -1; // 找不到該子行程

    // 2. 子行程還在執行，父行程進入 WAITING 狀態並讓出 CPU
    current_task->wait_pid = pid;
    current_task->state = TASK_WAITING;
    schedule();

    // 3. 當父行程被喚醒 (排程回到這裡)，代表子行程已經呼叫 exit() 變成 ZOMBIE 了
    // 再次掃描鏈結串列，將它設定為 DEAD 讓系統回收
    temp = current_task->next;
    while (temp != current_task) {
        if (temp->pid == pid && temp->state == TASK_ZOMBIE) {
            temp->state = TASK_DEAD;
            break;
        }
        temp = temp->next;
    }

    return 0;
}

int sys_kill(uint32_t pid) {
    // 防止自殺或誤殺系統核心 (Kernel Main PID 1 / Idle PID 9999)
    if (pid <= 1 || pid == KERNEL_IDLE_PID) return -1;

    // 因為牽涉到跨行程狀態修改，鎖定中斷避免 Race Condition
    __asm__ volatile("cli");

    task_t *target = get_task_by_pid(pid);
    if (!target || target->state == TASK_DEAD) {
        __asm__ volatile("sti");
        return -1;
    }

    // 1. 強制關閉它擁有的 GUI 視窗
    gui_close_windows_by_pid(pid);

    // 2. 檢查目標的父行程是否正在 wait() 它
    task_t *parent = get_task_by_pid(target->ppid);
    int parent_waiting = 0;
    if (parent && parent->state == TASK_WAITING && parent->wait_pid == pid) {
        parent->state = TASK_RUNNING; // 喚醒父行程
        parent->wait_pid = 0;
        parent_waiting = 1;
    }

    // 3. 根據父行程狀態決定它是變殭屍還是直接死透
    if (parent_waiting) {
        target->state = TASK_ZOMBIE;
    } else {
        target->state = TASK_DEAD;
    }

    __asm__ volatile("sti");
    return 0;
}

int task_get_process_list(process_info_t* list, int max_count) {
    if (!current_task) return 0;

    int count = 0;
    task_t* temp = (task_t*)current_task;

    // 走訪環狀鏈結串列，收集每個尚未死亡的 PCB 資訊
    do {
        if (temp->state != TASK_DEAD) {
            list[count].pid = temp->pid;
            list[count].ppid = temp->ppid;
            strncpy(list[count].name, temp->name, 32);
            list[count].state = temp->state;
            list[count].total_ticks = temp->total_ticks;

            // 估算記憶體使用量：(Heap 結尾 - Heap 開頭) + 預先分配的 1MB Heap + 4KB User Stack
            if (temp->heap_end >= temp->heap_start) {
                list[count].memory_used = (temp->heap_end - temp->heap_start) + (USER_HEAP_PAGES * PAGE_SIZE) + PAGE_SIZE;
            } else {
                // 如果是 Kernel Task，就只有 4KB Kernel Stack
                list[count].memory_used = PAGE_SIZE;
            }

            count++;
        }
        temp = temp->next;
    } while (temp != current_task && count < max_count);

    return count;
}


int sys_sleep(uint32_t ms) {
    // 假設頻率設定為 100Hz，則 1 tick = 10ms
    uint32_t ticks_to_sleep = ms / 10; // @TODO: Hardcode, 要從 timer.c 取得
    if (ticks_to_sleep == 0) ticks_to_sleep = 1; // 至少睡一個滴答

    current_task->wake_up_tick = tick + ticks_to_sleep;
    current_task->state = TASK_SLEEPING;

    // 讓出 CPU，schedule 會處理 cli/sti
    schedule();
    return 0;
}
```

---
src/kernel/kernel/syscall.c
```c
/**
 * @file src/kernel/kernel/syscall.c
 * @brief 系統呼叫分派器與實作 (System Call Dispatcher & Implementation)
 *
 * 本檔案實作了 Ring 3 與 Ring 0 之間的介面。當 User 程式執行 int 0x80 時，
 * 會跳轉到這裡的 syscall_handler。我們採用分派表 (Dispatch Table) 機制
 * 以提升擴充性與可讀性。
 */

#include "syscall.h"
#include "tty.h"
#include "utils.h"
#include "simplefs.h"
#include "keyboard.h"
#include "task.h"
#include "gui.h"
#include "vfs.h"
#include "pmm.h"
#include "paging.h"
#include "icmp.h"

#define MAX_IPC_MESSAGES 16
#define MAX_FD           32
#define MAX_SYSCALLS     128

extern void read_time(int* h, int* m);

/** @brief IPC 訊息結構 */
typedef struct {
    char data[128]; // 每個訊息上限 128 位元組
} ipc_msg_t;

/** @brief 全域檔案描述符表 (目前為簡化版全域共享) */
static fs_node_t* fd_table[MAX_FD] = {0};

/** @brief IPC 信箱環狀緩衝區 */
static ipc_msg_t mailbox_queue[MAX_IPC_MESSAGES];
static int mb_head = 0;  // 讀取頭
static int mb_tail = 0;  // 寫入尾
static int mb_count = 0; // 當前訊息數

/** @brief 鎖定系統關鍵區域 (關閉中斷) */
static void syscall_lock() {
    __asm__ volatile("cli");
}

/** @brief 解除鎖定系統關鍵區域 (開啟中斷) */
static void syscall_unlock() {
    __asm__ volatile("sti");
}

// =============================================================================
// 系統呼叫處理常式 (Syscall Handlers)
// 每一個 sys_* 函式對應一個特定的系統服務
// =============================================================================

/** @brief 格式化輸出字串到終端機 (SYS_PRINT_STR_LEN) */
static int sys_print_str_len(registers_t *regs) {
    kprintf((char*)regs->ebx, regs->ecx, regs->edx, regs->esi);
    return 0;
}

/** @brief 輸出字串到終端機 (SYS_PRINT_STR) */
static int sys_print_str(registers_t *regs) {
    kprintf((char*)regs->ebx);
    return 0;
}

/** @brief 開啟檔案 (SYS_OPEN) */
static int sys_open(registers_t *regs) {
    uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;
    char* filename = (char*)regs->ebx;
    fs_node_t* node = simplefs_find(current_dir, filename);
    if (node == 0) return -1;

    // 分配 FD
    for (int i = 3; i < MAX_FD; i++) {
        if (fd_table[i] == 0) {
            fd_table[i] = node;
            return i;
        }
    }
    return -1;
}

/** @brief 讀取檔案內容 (SYS_READ) */
static int sys_read(registers_t *regs) {
    int fd = (int)regs->ebx;
    uint8_t* buffer = (uint8_t*)regs->ecx;
    uint32_t size = (uint32_t)regs->edx;

    if (fd >= 3 && fd < MAX_FD && fd_table[fd] != 0) {
        return vfs_read(fd_table[fd], 0, size, buffer);
    }
    return -1;
}

/** @brief 從鍵盤讀取一個字元 (SYS_GETCHAR) */
static int sys_getchar_handler(registers_t *regs) {
    (void)regs;
    char c = keyboard_getchar();
    char str[2] = {c, '\0'};
    kprintf(str); // 回顯到螢幕
    return (int)c;
}

/** @brief 自願放棄 CPU 執行權 (SYS_YIELD) */
static int sys_yield_handler(registers_t *regs) {
    (void)regs;
    schedule();
    return 0;
}

/** @brief 結束目前行程 (SYS_EXIT) */
static int sys_exit_handler(registers_t *regs) {
    (void)regs;
    exit_task();
    return 0;
}

/** @brief 建立子行程 (SYS_FORK) */
static int sys_fork_handler(registers_t *regs) {
    return sys_fork(regs);
}

/** @brief 載入並執行新程式 (SYS_EXEC) */
static int sys_exec_handler(registers_t *regs) {
    return sys_exec(regs);
}

/** @brief 等待子行程結束 (SYS_WAIT) */
static int sys_wait_handler(registers_t *regs) {
    return sys_wait(regs->ebx);
}

/** @brief 發送 IPC 訊息 (SYS_IPC_SEND) */
static int sys_ipc_send(registers_t *regs) {
    if (mb_count < MAX_IPC_MESSAGES) {
        strcpy(mailbox_queue[mb_tail].data, (char*)regs->ebx);
        mb_tail = (mb_tail + 1) % MAX_IPC_MESSAGES;
        mb_count++;
        return 0;
    }
    return -1;
}

/** @brief 接收 IPC 訊息 (SYS_IPC_RECV) */
static int sys_ipc_recv(registers_t *regs) {
    if (mb_count > 0) {
        strcpy((char*)regs->ebx, mailbox_queue[mb_head].data);
        mb_head = (mb_head + 1) % MAX_IPC_MESSAGES;
        mb_count--;
        return 1;
    }
    return 0;
}

/** @brief 擴充使用者堆積空間 (SYS_SBRK) */
static int sys_sbrk_handler(registers_t *regs) {
    uint32_t old_end = current_task->heap_end;
    current_task->heap_end += (int)regs->ebx;
    return old_end;
}

/** @brief 建立新檔案 (SYS_CREATE) */
static int sys_create_handler(registers_t *regs) {
    uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;
    return vfs_create_file(current_dir, (char*)regs->ebx, (char*)regs->ecx);
}

/** @brief 讀取目錄內容 (SYS_READDIR) */
static int sys_readdir_handler(registers_t *regs) {
    uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;
    return simplefs_readdir(current_dir, (int)regs->ebx, (char*)regs->ecx, (uint32_t*)regs->edx, (uint32_t*)regs->esi);
}

/** @brief 刪除檔案 (SYS_REMOVE) */
static int sys_remove_handler(registers_t *regs) {
    uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;
    return vfs_delete_file(current_dir, (char*)regs->ebx);
}

/** @brief 建立新目錄 (SYS_MKDIR) */
static int sys_mkdir_handler(registers_t *regs) {
    uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;
    return vfs_mkdir(current_dir, (char*)regs->ebx);
}

/** @brief 切換目前工作目錄 (SYS_CHDIR) */
static int sys_chdir_handler(registers_t *regs) {
    uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;
    char* dirname = (char*)regs->ebx;

    if (strcmp(dirname, "/") == 0) {
        current_task->cwd_lba = 1;
        return 0;
    } else {
        uint32_t target_lba = simplefs_get_dir_lba(current_dir, dirname);
        if (target_lba != 0) {
            current_task->cwd_lba = target_lba;
            return 0;
        }
    }
    return -1;
}

/** @brief 取得目前工作目錄路徑 (SYS_GETCWD) */
static int sys_getcwd_handler(registers_t *regs) {
    uint32_t current_dir = current_task->cwd_lba ? current_task->cwd_lba : 1;
    return vfs_getcwd(current_dir, (char*)regs->ebx);
}

/** @brief 取得當前 PID (SYS_GETPID) */
static int sys_getpid_handler(registers_t *regs) {
    (void)regs;
    return current_task->pid;
}

/** @brief 取得系統行程列表 (SYS_GETPROCS) */
static int sys_getprocs_handler(registers_t *regs) {
    return task_get_process_list((process_info_t*)regs->ebx, (int)regs->ecx);
}

/** @brief 清除畫面 (SYS_CLEAR_SCREEN) */
static int sys_clear_screen_handler(registers_t *regs) {
    (void)regs;
    terminal_initialize();
    gui_redraw();
    return 0;
}

/** @brief 殺死特定行程 (SYS_KILL) */
static int sys_kill_handler(registers_t *regs) {
    return sys_kill((uint32_t)regs->ebx);
}

/** @brief 取得記憶體使用狀態 (SYS_GET_MEM_INFO) */
static int sys_get_mem_info_handler(registers_t *regs) {
    mem_info_t* info = (mem_info_t*)regs->ebx;
    pmm_get_stats(&info->total_frames, &info->used_frames, &info->free_frames);
    info->active_universes = paging_get_active_universes();
    return 0;
}

/** @brief 建立圖形視窗 (SYS_CREATE_WINDOW) */
static int sys_create_window_handler(registers_t *regs) {
    static int win_offset = 0;
    int x = 200 + win_offset;
    int y = 150 + win_offset;
    win_offset = (win_offset + 30) % 200;

    int win_id = create_window(x, y, (int)regs->ecx, (int)regs->edx, (char*)regs->ebx, current_task->pid);
    gui_redraw();
    return win_id;
}

/** @brief 更新視窗內容 (SYS_UPDATE_WINDOW) */
static int sys_update_window_handler(registers_t *regs) {
    window_t* win = get_window((int)regs->ebx);
    if (win != 0 && win->owner_pid == current_task->pid && win->canvas != 0) {
        memcpy(win->canvas, (uint32_t*)regs->ecx, (win->width - 4) * (win->height - 24) * 4);
        gui_redraw();
    }
    return 0;
}

/** @brief 取得系統時間 (SYS_GET_TIME) */
static int sys_get_time_handler(registers_t *regs) {
    read_time((int*)regs->ebx, (int*)regs->ecx);
    return 0;
}

/** @brief 取得視窗點擊事件 (SYS_GET_WIN_EVENT) */
static int sys_get_win_event_handler(registers_t *regs) {
    int win_id = (int)regs->ebx;
    int* out_x = (int*)regs->ecx;
    int* out_y = (int*)regs->edx;

    window_t* win = get_window(win_id);
    if (win != 0 && win->owner_pid == current_task->pid && win->has_new_click) {
        *out_x = win->last_click_x;
        *out_y = win->last_click_y;
        win->has_new_click = 0;
        return 1;
    }
    return 0;
}

/** @brief 取得視窗按鍵事件 (SYS_GET_WIN_KEY) */
static int sys_get_win_key_handler(registers_t *regs) {
    int win_id = (int)regs->ebx;
    char* out_key = (char*)regs->ecx;

    window_t* win = get_window(win_id);
    if (win != 0 && win->owner_pid == current_task->pid && win->has_new_key) {
        *out_key = win->last_key;
        win->has_new_key = 0;
        return 1;
    }
    return 0;
}

/** @brief 發送 ICMP Ping 請求 (SYS_PING) */
static int sys_ping_handler(registers_t *regs) {
    ping_send_request((uint8_t*)regs->ebx);
    return 0;
}

// ==========================================
// 32 號 Syscall：UDP 發送 (ebx=IP陣列, ecx=Port, edx=字串)
// ==========================================
static int sys_net_udp_send_handler(registers_t *regs) {
    uint8_t* ip = (uint8_t*)regs->ebx;
    uint16_t port = (uint16_t)regs->ecx;
    char* msg = (char*)regs->edx;
    int len = (int)regs->esi; // 【修復】直接讀取 User 傳來的真實長度！

    extern void udp_send_packet(uint8_t* dest_ip, uint16_t src_port, uint16_t dest_port, uint8_t* data, uint32_t len);

    // 我們將來源 Port 寫死為 12345，目標 Port 由 User 決定
    udp_send_packet(ip, 12345, port, (uint8_t*)msg, len);

    regs->eax = 0;
    return 0;
}

// SYS_NET_UDP_BIND
static int sys_net_udp_bind_handler(registers_t *regs) {
    extern void udp_bind_port(uint16_t port);
    udp_bind_port((uint16_t)regs->ebx);
    regs->eax = 0;
    return 0;
}

// SYS_NET_UDP_RECV
static int sys_net_udp_recv_handler(registers_t *regs) {
    extern int udp_recv_data(char* buffer);
    return udp_recv_data((char*)regs->ebx); // 回傳 1 或 0

}

static int sys_net_tcp_connect_handler(registers_t *regs) {
    extern void tcp_send_syn(uint8_t* dest_ip, uint16_t src_port, uint16_t dest_port);
    // 使用固定的本機 Port 54321 去撞目標的 Port
    tcp_send_syn((uint8_t*)regs->ebx, TEST_TCP_PORT, (uint16_t)regs->ecx);
    regs->eax = 0;
    return 0;
}

static int sys_net_tcp_send_handler(registers_t *regs) {
    extern void tcp_send_data(uint8_t*, uint16_t, uint16_t, uint8_t*, uint32_t);
    char* msg = (char*)regs->edx;
    tcp_send_data((uint8_t*)regs->ebx, TEST_TCP_PORT, (uint16_t)regs->ecx, (uint8_t*)msg, strlen(msg));
    regs->eax = 0;
    return 0;
}

static int sys_net_tcp_close_handler(registers_t *regs) {
    extern void tcp_send_fin(uint8_t*, uint16_t, uint16_t);
    tcp_send_fin((uint8_t*)regs->ebx, TEST_TCP_PORT, (uint16_t)regs->ecx);
    regs->eax = 0;
    return 0;
}

static int sys_net_tcp_recv_handler(registers_t *regs) {
    extern int tcp_recv_data(char* buffer);
    return tcp_recv_data((char*)regs->ebx);
}

// ==========================================
// 【Day 98 新增】寫入檔案的系統呼叫
// ==========================================
static int sys_write_handler(registers_t *regs) {
    int fd = (int)regs->ebx;
    uint8_t* buffer = (uint8_t*)regs->ecx;
    uint32_t size = (uint32_t)regs->edx;

    // 確認 fd 是合法的 (3 以上，且已被 open)
    if (fd >= 3 && fd < MAX_FD && fd_table[fd] != 0) {
        // 呼叫虛擬檔案系統的寫入函式 (假設每次都從 offset 0 覆寫)
        extern uint32_t vfs_write(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
        return vfs_write(fd_table[fd], 0, size, buffer);
    }
    return -1;
}


static int sys_sleep_handler(registers_t *regs) {
    return sys_sleep((uint32_t)regs->ebx);
}

/** @brief 切換顯示模式 (SYS_SET_DISPLAY_MODE) */
static int sys_set_display_mode_handler(registers_t *regs) {
    switch_display_mode((int)regs->ebx);
    return 0;
}

// =============================================================================
// 系統呼叫分派表 (Syscall Dispatch Table)
// 將編號映射到處理函式
// =============================================================================

static syscall_t syscall_table[MAX_SYSCALLS] = {
    [SYS_PRINT_STR_LEN]   = sys_print_str_len,
    [SYS_PRINT_STR]       = sys_print_str,
    [SYS_OPEN]            = sys_open,
    [SYS_READ]            = sys_read,
    [SYS_GETCHAR]         = sys_getchar_handler,
    [SYS_YIELD]           = sys_yield_handler,
    [SYS_EXIT]            = sys_exit_handler,
    [SYS_FORK]            = sys_fork_handler,
    [SYS_EXEC]            = sys_exec_handler,
    [SYS_WAIT]            = sys_wait_handler,
    [SYS_IPC_SEND]        = sys_ipc_send,
    [SYS_IPC_RECV]        = sys_ipc_recv,
    [SYS_SBRK]            = sys_sbrk_handler,
    [SYS_CREATE]          = sys_create_handler,
    [SYS_READDIR]         = sys_readdir_handler,
    [SYS_REMOVE]          = sys_remove_handler,
    [SYS_MKDIR]           = sys_mkdir_handler,
    [SYS_CHDIR]           = sys_chdir_handler,
    [SYS_GETCWD]          = sys_getcwd_handler,
    [SYS_GETPID]          = sys_getpid_handler,
    [SYS_GETPROCS]        = sys_getprocs_handler,
    [SYS_CLEAR_SCREEN]    = sys_clear_screen_handler,
    [SYS_KILL]            = sys_kill_handler,
    [SYS_GET_MEM_INFO]    = sys_get_mem_info_handler,
    [SYS_CREATE_WINDOW]   = sys_create_window_handler,
    [SYS_UPDATE_WINDOW]   = sys_update_window_handler,
    [SYS_GET_TIME]        = sys_get_time_handler,
    [SYS_GET_WIN_EVENT]   = sys_get_win_event_handler,
    [SYS_GET_WIN_KEY]     = sys_get_win_key_handler,
    [SYS_PING]            = sys_ping_handler,
    [SYS_NET_UDP_SEND]    = sys_net_udp_send_handler,
    [SYS_NET_UDP_BIND]    = sys_net_udp_bind_handler,
    [SYS_NET_UDP_RECV]    = sys_net_udp_recv_handler,
    [SYS_NET_TCP_CONNECT] = sys_net_tcp_connect_handler,
    [SYS_NET_TCP_SEND]    = sys_net_tcp_send_handler,
    [SYS_NET_TCP_CLOSE]   = sys_net_tcp_close_handler,
    [SYS_NET_TCP_RECV]    = sys_net_tcp_recv_handler,
    [SYS_WRITE]           = sys_write_handler,
    [SYS_SLEEP]           = sys_sleep_handler,
    [SYS_SET_DISPLAY_MODE] = sys_set_display_mode_handler,
};

/**
 * @brief 系統呼叫主要入口點 (Main Entry Point)
 *
 * 當 CPU 觸發中斷時，組合語言會呼叫此函式。
 * 它會檢查編號合法性、決定是否需要鎖定、分派任務並存回結果。
 */
void syscall_handler(registers_t *regs) {
    uint32_t syscall_num = regs->eax;

    // 檢查編號範圍與是否有對應的處理常式
    if (syscall_num >= MAX_SYSCALLS || syscall_table[syscall_num] == 0) {
        regs->eax = -1;
        return;
    }

    // [效能優化] 判斷是否需要全域鎖定。
    // 部分唯讀或不涉及關鍵資料修改的呼叫可以無鎖執行。
    int need_lock = 1;
    switch(syscall_num) {
        case SYS_PRINT_STR_LEN:
        case SYS_PRINT_STR:
        case SYS_GETCHAR:
        case SYS_SBRK:
        case SYS_PING:
            need_lock = 0;
            break;
    }

    if (need_lock) syscall_lock();

    // 執行處理函式並將回傳值寫入 eax，這將成為 User 看到的 syscall 回傳值
    syscall_t handler = syscall_table[syscall_num];
    regs->eax = handler(regs);

    if (need_lock) syscall_unlock();
}

/** @brief 初始化系統呼叫子系統 (目前採靜態初始化) */
void init_syscalls(void) {
    // 未來若有動態註冊需求可在此擴充
}
```

---
src/kernel/init/main.c
```c
#include "tty.h"
#include "utils.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "timer.h"
#include "pmm.h"
#include "kheap.h"
#include "ata.h"
#include "mbr.h"
#include "vfs.h"
#include "simplefs.h"
#include "elf.h"
#include "task.h"
#include "multiboot.h"
#include "gfx.h"
#include "mouse.h"
#include "gui.h"
#include "config.h"
#include "pci.h"
#include "ethernet.h"
#include "arp.h"
#include "icmp.h"

void setup_filesystem(uint32_t part_lba, multiboot_info_t* mbd) {
    kprintf("[Kernel] Setting up SimpleFS environment...\n");
    simplefs_mount(part_lba);
    simplefs_format(part_lba, 10000);
    simplefs_create_file(1, "hello.txt", "This is the content of the very first file ever created on Simple OS!\n", 70);

    // 【修改】自動巡覽所有模組並寫入 HDD
    if (mbd->mods_count > 0) {
        multiboot_module_t* mod = (multiboot_module_t*)mbd->mods_addr;

        // 這個次序跟 GRUB 列出來的次序要對齊
        char* filenames[] = {
            "shell.elf",
            "echo.elf", "ping2.elf", "pong.elf", "sleep.elf",
            // file/directory
            "touch.elf", "cat.elf", "ls.elf", "rm.elf", "mkdir.elf",
            // process
            "ps.elf", "top.elf", "kill.elf",
            // memory
            "free.elf", "segfault.elf",
            // network
            "ping.elf",
            "udpsend.elf", "udprecv.elf", "tcptest.elf",
            "host.elf", "wget.elf",
            // window app
            "status.elf", "paint.elf",
            "viewer.elf", "logo.bmp",
            "clock.elf", "explorer.elf",
            "taskmgr.elf", "notepad.elf",
            "tictactoe.elf"
        };
        uint32_t expected_mods = sizeof(filenames) / sizeof(filenames[0]);

        for(uint32_t i = 0; i < mbd->mods_count && i < expected_mods; i++) {
            uint32_t size = mod[i].mod_end - mod[i].mod_start;
            // kprintf("[Kernel] Installing [%s] to HDD (Size: %d bytes)...\n", filenames[i], size);
            simplefs_create_file(1, filenames[i], (char*)mod[i].mod_start, size);
        }
    }

    // simplefs_list_files();

}

/**
 * [Kernel Entry Point]
 *
 * ASCII Flow:
 * GRUB -> boot.S -> kernel_main()
 *                      |
 *                      +--> terminal_initialize()
 *                      +--> init_gdt() / init_idt()
 *                      +--> init_paging() / init_pmm() / init_kheap()
 *                      +--> init_gfx()
 *                      +--> parse_cmdline() -> GUI or CLI?
 *                      +--> init_mouse() / sti
 *                      +--> setup_filesystem()
 *                      +--> init_multitasking()
 *                      +--> create_user_task("shell.elf")
 *                      +--> exit_task() (Kernel becomes idle)
 */
void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
    (void)magic; // 忽略未使用的警告

    terminal_initialize();
    kprintf("=== Simple OS Booting ===\n");

    // --- 系統底層基礎建設 ---
    init_gdt();
    init_idt();
    init_paging();
    init_timer(100);
    init_pmm(INITIAL_PMM_SIZE);
    init_kheap();
    init_gfx(mbd);

    // ==========================================
    // 2. 解析 GRUB 傳遞的 Cmdline
    // ==========================================
    int is_gui = 1; // 預設為 GUI 模式

    // 檢查 mbd 的 bit 2，確認 cmdline 是否有效
    if (mbd->flags & (1 << 2)) {
        char* cmdline = (char*) mbd->cmdline;
        // 如果 GRUB 參數包含 mode=cli，就切換到 CLI 模式
        if (strstr(cmdline, "mode=cli") != 0) {
            is_gui = 0;
        }
    }

    // 3. 根據模式初始化系統介面
    enable_gui_mode(is_gui);
    terminal_set_mode(is_gui);

    if (is_gui) {
        init_gui();
        int term_win = create_window(50, 50, 600, 300, "Simple OS Terminal", 1); // 預設 PID(1) 建立
        terminal_bind_window(term_win);
        gui_render(400, 300);

    } else {
        // CLI 模式只綁定一個無效的視窗 ID，並強制重繪全螢幕
        terminal_bind_window(-1);
        gui_redraw();
    }

    init_mouse();

    __asm__ volatile ("sti");

    init_pci();


    // 左上角的終端機文字會自然地印在藍綠色的桌面上
    kprintf("=== OS Subsystems Ready ===\n\n");
    kprintf("Welcome to Simple OS Desktop Environment!\n");

    // --- 儲存與檔案系統 ---
    uint32_t part_lba = parse_mbr();
    if (part_lba == 0) {
        kprintf("Disk Error: No partition found.\n");
        while(1) __asm__ volatile("hlt");
    }
    // 初始化 file system: mount, format, copy external applications
    setup_filesystem(part_lba, mbd);

    // ==========================================
    // 應用程式載入與排程 (Ring 0 -> Ring 3)
    // ==========================================
    kprintf("[Kernel] Loading 'shell' from Virtual File System...\n");
    fs_node_t* app_node = simplefs_find(1, "shell.elf");

    if (app_node != 0) {
        uint8_t* app_buffer = (uint8_t*) kmalloc(app_node->length);
        vfs_read(app_node, 0, app_node->length, app_buffer);
        uint32_t entry_point = elf_load((elf32_ehdr_t*)app_buffer);

        if (entry_point != 0) {
            kprintf("Creating Initial Process (Shell)...\n\n");

            // 啟動排程器 (Timer IRQ0 開始跳動)
            init_multitasking();

            // 為 Shell 分配專屬的 User Stack
            uint32_t ustack1_phys = (uint32_t) pmm_alloc_page();
            map_page(0x083FF000, ustack1_phys, 7);

            // 建立 Ring 3 主任務
            create_user_task(entry_point, 0x083FF000 + 4096);

            kprintf("[Kernel] Dropping to idle state. Enjoy your GUI!\n");

            // Kernel 功成身退，把自己宣告死亡！
            exit_task();
        }
    } else {
        kprintf("[Kernel] Error: Shell (shell.elf) not found on disk.\n");
    }

    while (1) { __asm__ volatile ("hlt"); }
}
```
