; [目的] 中斷服務例程 (ISR) 總匯與 Interrupt Descriptor Table 載入
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

;  Page Fault
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
    push 0          ;  add 動推入假錯誤碼
    call isr0_handler
    add esp, 4      ;  add 清除假錯誤碼
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
    push 0          ; 手動推入假錯誤碼
    pusha
    call timer_handler
    popa
    add esp, 4      ; 清除假錯誤碼
    iret

isr33:
    push 0          ;
    pusha
    call keyboard_handler
    popa
    add esp, 4      ;
    iret

isr43:
    push 0          ;
    pusha
    call rtl8139_handler
    popa
    add esp, 4      ;
    iret

isr44:
    push 0          ;
    pusha
    call mouse_handler
    popa
    add esp, 4      ;
    iret

; 第 128 號中斷 (System Call 窗口)
isr128:
    push 0          ; 手動推入假錯誤碼，對齊 registers_t！
    pusha           ; 備份通用暫存器 (32 bytes)
    push esp        ; 把 registers_t 的指標傳給 C 語言
    call syscall_handler
    add esp, 4      ; 清除 esp 參數
    popa            ; 恢復所有暫存器
    add esp, 4      ; 清除假錯誤碼
    iret
