; interrupts.S
global idt_flush

global isr0
extern isr0_handler ; 告訴組譯器，這個函式在 C 語言裡

global isr32
extern timer_handler

global isr33
extern keyboard_handler

global isr128
extern syscall_handler


; 載入 IDT (跟昨天的 lgdt 幾乎一樣)
idt_flush:
    mov eax, [esp+4]  ; 取出 C 語言傳進來的 idt_ptr 位址
    lidt [eax]        ; Load IDT
    ret

; 第 0 號中斷的跳板程式 (除以零)
isr0:
    ; 關閉中斷，避免在處理錯誤時又被其他中斷打擾
    cli

    ; 呼叫我們在 idt.c 裡寫的 C 語言處理函式
    call isr0_handler

    ; 恢復中斷並返回 (但因為我們在 handler 裡寫了 hlt，其實不會走到這裡)
    sti
    iret


; 第 32 號中斷 (Timer IRQ 0)
isr32:
    pusha
    call timer_handler
    popa
    iret

; 第 33 號中斷 (鍵盤)
isr33:
    ; 備份所有通用暫存器 (這是一個好習慣，避免中斷處理程式弄髒了原本正在執行的程式的狀態)
    pusha

    call keyboard_handler

    ; 恢復所有通用暫存器
    popa

    iret ; 中斷返回


; 第 128 號中斷 (System Call 窗口)
isr128:
    pusha           ; 備份目前的暫存器狀態

    ; 依照 C 語言的呼叫慣例 (cdecl)，參數要從右到左 push 進 Stack
    ; 我們假設 User App 把 syscall_num 放 eax, arg1 放 ebx, arg2 放 ecx
    push ecx        ; 參數 3 (arg2)
    push ebx        ; 參數 2 (arg1)
    push eax        ; 參數 1 (syscall_num)

    call syscall_handler

    ; 呼叫完畢後，清理剛剛 push 進去的 3 個參數 (3 * 4 = 12 bytes)
    add esp, 12

    popa            ; 恢復暫存器狀態
    iret            ; 返回 User App
