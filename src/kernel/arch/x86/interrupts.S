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
