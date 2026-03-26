; [目的] 中斷處理與 IDT 載入
; [流程 - 以 System Call 為例]
; 使用者執行 int 0x128
;    |
;    V
; [ isr128 ]
;    |--> [ pusha ] (備份所有通用暫存器)
;    |--> [ call syscall_handler ] (執行 C 處理邏輯)
;    |--> [ popa ] (恢復暫存器)
;    |--> [ iret ] (返回使用者模式)

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
