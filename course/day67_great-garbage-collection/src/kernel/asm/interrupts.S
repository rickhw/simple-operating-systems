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
