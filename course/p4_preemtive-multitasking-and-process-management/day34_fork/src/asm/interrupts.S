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
    pusha           ; 1. 備份所有通用暫存器 (32 bytes)

    ; 此刻 esp 指向暫存器結構的開頭 (也就是 registers_t 的位址)
    push esp        ; 2. 把 esp 當作參數傳給 syscall_handler!

    call syscall_handler

    add esp, 4      ; 3. 清理剛才壓入的 esp 參數

    popa            ; 4. 完美恢復所有暫存器 (包含 C 語言寫入 eax 的回傳值)
    iret
